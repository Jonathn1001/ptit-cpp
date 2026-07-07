# APEX — Multi-Currency Banking System — Design

> PTIT mid-term project (Đồ án giữa kỳ), Đề #2.
> Date: 2026-04-21

## 1. Goal

Build an interactive CLI banking system that manages multiple accounts in
multiple currencies, performs cross-currency transfers, logs every action in a
ledger, and demonstrates two specific failure scenarios on demand (double
spend, invalid exchange rate).

The assignment grades three OOP concepts: inheritance, operator overloading,
and use of `std::map`. Every design choice below is oriented toward making
those three concepts load-bearing rather than decorative.

## 2. Scope & decisions (brainstorming outcomes)

| # | Decision | Rationale |
|---|---|---|
| 1 | Interactive CLI menu | Easiest to live-demo error cases on video |
| 2 | Runtime-configurable currencies & rates | Lets the "set rate = 0" error case be demoed live |
| 3 | Single-threaded, scripted double-spend simulation | Deterministic, narratable on camera; threads add complexity without teaching the graded OOP concepts |
| 4 | In-memory only (no persistence) | File I/O is not graded; keeps focus on OOP |
| 5 | Header + implementation per class + Makefile | Mid-term project expects proper translation-unit separation |
| 6 | Vietnamese README, English in-code comments | README is what the professor reads; English comments are idiomatic |
| 7 | C++20 | Modern default; gives `std::optional`, structured bindings, cleaner `std::map` usage |
| 8 | Savings = interest, no overdraft; Checking = no interest, overdraft limit | Standard textbook distinction |
| 9 | Operator `+` / `-` take a `Money` and auto-convert currency | Makes the overloads multi-currency-aware, not a side-car |

## 3. File layout

```
ptit-cpp/
  apex-banking/
    include/
      Money.h
      Errors.h
      Currency.h
      Transaction.h
      Account.h
      Savings.h
      Checking.h
      Bank.h
    src/
      Currency.cpp
      Transaction.cpp
      Account.cpp
      Savings.cpp
      Checking.cpp
      Bank.cpp
      main.cpp
    Makefile
    README.md
```

Build: `cd apex && make` → `./apex`. Makefile rule:

```makefile
CXX      := g++
CXXFLAGS := -std=c++20 -Wall -Wextra -Iinclude
SRC      := $(wildcard src/*.cpp)
apex: $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o apex
clean:
	rm -f apex
```

`Money.h` and `Errors.h` are header-only (small value types / exception
classes). All other classes split declaration and definition.

## 4. Core types

### 4.1 `Money` (header-only)

```cpp
struct Money {
    double amount;
    std::string currency;   // e.g. "USD", "VND"
};
std::ostream& operator<<(std::ostream&, const Money&);
```

### 4.2 `Errors.h` — exception hierarchy

```cpp
struct BankError : std::runtime_error {
    using std::runtime_error::runtime_error;
};
struct AccountNotFound     : BankError { using BankError::BankError; };
struct DuplicateAccountId  : BankError { using BankError::BankError; };
struct InsufficientFunds   : BankError { using BankError::BankError; };
struct CurrencyUnknown     : BankError { using BankError::BankError; };
struct InvalidRate         : BankError { using BankError::BankError; };   // rate <= 0
struct DoubleSpendDetected : BankError { using BankError::BankError; };   // lock contention
struct BadInput            : BankError { using BankError::BankError; };   // malformed CLI input
```

All carry a human-readable message. The CLI main loop has a single top-level
`catch (const BankError&)` that prints the message and returns to the menu —
no crash ever escapes to the user.

### 4.3 `CurrencyRegistry`

Stores each currency's rate relative to a base (USD by convention).

```cpp
class CurrencyRegistry {
    std::map<std::string, double> rates;  // code -> rate per 1 USD
public:
    void setRate(const std::string& code, double rate);   // throws InvalidRate if <= 0
    double getRate(const std::string& code) const;        // throws CurrencyUnknown
    Money convert(const Money& from, const std::string& toCurrency) const;
    bool has(const std::string& code) const;
    void list(std::ostream&) const;
};
```

`convert` logic: `amountInBase = from.amount / rate(from.currency);
result = amountInBase * rate(toCurrency);`

### 4.4 `Transaction` (immutable ledger entry)

```cpp
enum class TxType   { CREATE_ACCOUNT, DEPOSIT, WITHDRAW, TRANSFER, SET_RATE, APPLY_INTEREST };
enum class TxStatus { SUCCESS, FAILED };

class Transaction {
    int id;
    TxType type;
    TxStatus status;
    std::string fromId;       // empty when not applicable
    std::string toId;          // empty when not applicable
    Money amount;              // {0, ""} when not applicable (e.g. SET_RATE)
    std::string note;          // failure reason / rate value / etc.
    std::time_t timestamp;
public:
    // full ctor, getters, print(std::ostream&) method
};
```

### 4.5 `Account` (abstract base)

```cpp
class Account {
protected:
    std::string id;
    std::string owner;
    std::string nativeCurrency;
    double balance;                    // always stored in nativeCurrency
    const CurrencyRegistry* reg;       // non-owning pointer
public:
    Account(std::string id, std::string owner, std::string currency,
            Money opening, const CurrencyRegistry* reg);
    virtual ~Account() = default;

    virtual std::string kind() const = 0;                   // "Savings" / "Checking"
    virtual bool canWithdraw(double amtNative) const = 0;   // policy hook

    // required operator overloads
    Account& operator+(const Money& m);    // deposit (auto-convert)
    Account& operator-(const Money& m);    // withdraw (auto-convert + policy)

    // getters
    const std::string& getId() const;
    double getBalance() const;
    const std::string& getCurrency() const;
    const std::string& getOwner() const;

    virtual void print(std::ostream&) const;
};
```

### 4.6 `Savings`

```cpp
class Savings : public Account {
    double interestRate;   // e.g. 0.05 for 5%
public:
    Savings(std::string id, std::string owner, std::string currency,
            Money opening, const CurrencyRegistry* reg, double interestRate);
    std::string kind() const override { return "Savings"; }
    bool canWithdraw(double amt) const override;    // amt <= balance
    void applyInterest();                           // balance *= (1 + interestRate)
    void print(std::ostream&) const override;
};
```

### 4.7 `Checking`

```cpp
class Checking : public Account {
    double overdraftLimit;   // native-currency allowance below zero
public:
    Checking(std::string id, std::string owner, std::string currency,
             Money opening, const CurrencyRegistry* reg, double overdraftLimit);
    std::string kind() const override { return "Checking"; }
    bool canWithdraw(double amt) const override;    // amt <= balance + overdraftLimit
    void print(std::ostream&) const override;
};
```

### 4.8 `Bank` (facade / orchestrator)

```cpp
class Bank {
    std::map<std::string, std::unique_ptr<Account>> accounts;   // REQUIRED by spec
    CurrencyRegistry registry;
    std::vector<Transaction> ledger;
    std::set<std::string> lockedIds;                            // for double-spend fix phase
    int nextTxId = 1;
    int nextAccountSeq = 1;

    int allocTxId();
    std::string allocAccountId();   // e.g. "acc_001"
    void log(Transaction tx);
    static std::time_t now();       // wraps std::time(nullptr); private helper
public:
    // lifecycle
    Account& createSavings(const std::string& owner, const std::string& currency,
                           const Money& opening, double interestRate);
    Account& createChecking(const std::string& owner, const std::string& currency,
                            const Money& opening, double overdraftLimit);
    Account* find(const std::string& id);               // nullptr if missing
    Account& require(const std::string& id);            // throws AccountNotFound

    // money movement
    void deposit (const std::string& id, const Money& m);
    void withdraw(const std::string& id, const Money& m);
    void transfer(const std::string& fromId, const std::string& toId, const Money& m);

    // currency admin
    void setRate(const std::string& code, double rate);

    // queries
    void listAccounts(std::ostream&) const;
    void printLedger (std::ostream&) const;
    const CurrencyRegistry& rates() const { return registry; }

    // locking helpers (used only by the double-spend fix demo)
    void lock  (const std::string& id);   // throws DoubleSpendDetected if already locked
    void unlock(const std::string& id);

    // demo
    void simulateDoubleSpend(const std::string& accountId, double amt);
};
```

## 5. Operator semantics (the graded bit)

`operator+` and `operator-` on `Account` mutate `*this` and return a reference
to allow chaining. They auto-convert the supplied `Money` to the account's
native currency via the registry.

```cpp
Account& Account::operator+(const Money& m) {
    if (m.amount < 0) throw BadInput("amount must be >= 0");
    Money converted = reg->convert(m, nativeCurrency);   // may throw
    balance += converted.amount;
    return *this;
}

Account& Account::operator-(const Money& m) {
    if (m.amount < 0) throw BadInput("amount must be >= 0");
    Money converted = reg->convert(m, nativeCurrency);
    if (!canWithdraw(converted.amount))
        throw InsufficientFunds("insufficient funds in " + id);
    balance -= converted.amount;
    return *this;
}
```

`Bank::transfer` composes them atomically:

```cpp
void Bank::transfer(const std::string& fromId, const std::string& toId, const Money& m) {
    Account& from = require(fromId);
    Account& to   = require(toId);
    try {
        from - m;                     // may throw; no state mutated beyond from
        to   + m;
        log({allocTxId(), TxType::TRANSFER, TxStatus::SUCCESS, fromId, toId, m, "", now()});
    } catch (const BankError& e) {
        log({allocTxId(), TxType::TRANSFER, TxStatus::FAILED,  fromId, toId, m, e.what(), now()});
        throw;
    }
}
```

Note on atomicity: if `from - m` succeeds but `to + m` throws (e.g. because
`to` uses an unknown currency), the money has vanished from `from`. For a
mid-term the design accepts this risk — currency validation happens at the CLI
boundary before `transfer` is called, so this path is not reachable in normal
use. A production design would stage the withdrawal.

## 6. Error cases (required by assignment)

### 6.1 Invalid exchange rate (zero or negative)

Guarded at the single point of entry in `CurrencyRegistry::setRate`:

```cpp
void CurrencyRegistry::setRate(const std::string& code, double rate) {
    if (rate <= 0.0)
        throw InvalidRate("rate must be > 0 (got " + std::to_string(rate) + ")");
    rates[code] = rate;
}
```

The CLI wraps the call in a try/catch, prints the error, and logs a
`SET_RATE` transaction with `TxStatus::FAILED` and the offending rate in
`note`.

Demo flow (menu item 9, or dedicated menu item 13):
```
Currency code: EUR
New rate: -1.5
❌ InvalidRate: rate must be > 0 (got -1.500000)
[tx #14 FAILED logged]
```

### 6.2 Double-spend simulation

Menu item 12. `Bank::simulateDoubleSpend(id, amt)` runs a scripted two-phase
scenario on a target account, printing narration at each step.

**Phase 1 — buggy (direct balance manipulation):**

```
Step 1. Terminal A reads balance → sees $100
Step 2. Terminal B reads balance → sees $100    (both snapshots say "affordable")
Step 3. Terminal A check: 80 <= 100  ✓
Step 4. Terminal B check: 80 <= 100  ✓
Step 5. Terminal A writes balance = 100 - 80 = 20
Step 6. Terminal B writes balance = 100 - 80 = 20  (expected -60!)
❌ Final balance: $20. Withdrawn: $160. The bank lost $60.
```

This is implemented with raw `double` snapshots bypassing the normal
`operator-` path; the point is to show why the naive approach is unsafe.

**Phase 2 — fixed (`Bank::lock` / `unlock`):**

```
Step 1. Terminal A bank.lock(acc_001)  ✓
Step 2. Terminal B bank.lock(acc_001)  ❌ DoubleSpendDetected
Step 3. Terminal A operator-(80)       ✓   balance = 20
Step 4. Terminal A bank.unlock(acc_001)
Step 5. Terminal B retry → lock ✓, canWithdraw(80) false → InsufficientFunds
✅ Final balance: $20.
```

Both phases write ledger entries so `printLedger` afterwards shows the full
sequence (successes and failures).

### 6.3 Additional defensive checks (not required but free)

- Duplicate account ID (`createSavings`/`createChecking` with conflicting id)
  → `DuplicateAccountId`.
- Negative deposit / withdraw amount → `BadInput`.
- Unknown currency in a `Money` value → `CurrencyUnknown`.
- `canWithdraw` violation → `InsufficientFunds`.
- Malformed CLI numeric input → caught at the CLI boundary, `BadInput`.

## 7. CLI

```
╔══════════════════════════════════════════════╗
║  APEX — Hệ thống ngân hàng đa tiền tệ        ║
╚══════════════════════════════════════════════╝

── Tài khoản ──
  1. Tạo tài khoản Savings
  2. Tạo tài khoản Checking
  3. Xem danh sách tài khoản
  4. Xem chi tiết một tài khoản

── Giao dịch ──
  5. Nạp tiền (Deposit)
  6. Rút tiền (Withdraw)
  7. Chuyển khoản (Transfer)

── Tiền tệ ──
  8. Xem tỷ giá hiện tại
  9. Cập nhật tỷ giá (Admin)

── Báo cáo & Demo ──
 10. In sổ cái giao dịch (Ledger)
 11. Áp dụng lãi suất (Savings)
 12. ⚠ Demo: Double-Spend
 13. ⚠ Demo: Tỷ giá không hợp lệ

  0. Thoát

Lựa chọn › _
```

**Startup seed data** (in `main()` before the menu loop):
- Currencies: `USD` 1.0, `VND` 24500.0, `EUR` 0.92, `JPY` 155.0.
- Accounts:
  - `acc_001` — Savings, owner "Nguyễn Văn A", USD, opening 200.0, interest 5%.
  - `acc_002` — Checking, owner "Trần Thị B", VND, opening 1_000_000, overdraft 500_000.

**Input robustness:** every numeric read clears and rethrows `BadInput` on
failure so the user never sees garbage values; menu option is read as
`std::string` then parsed, so empty-enter or letters don't break parsing.

## 8. Documentation

### 8.1 README (Vietnamese, GitHub-ready)

Sections: Mô tả, Tính năng, Yêu cầu (g++ ≥ 10, Make), Cài đặt & Biên dịch
(`git clone` → `cd apex` → `make`), Chạy (`./apex`), Demo các tình huống lỗi
(menu 12 & 13 with ASCII output samples), Kiến trúc (class diagram, either
mermaid or ASCII), Nạp chồng toán tử (table explaining `+` and `-`), Cấu
trúc thư mục (tree), Tác giả.

### 8.2 In-code comments (English)

Every public method gets a Javadoc-style docblock stating purpose, params,
exceptions, and return. Private helpers get one-line `//` comments.
Example:

```cpp
/**
 * Deposit money into this account. Automatically converts the given
 * Money to the account's native currency via the CurrencyRegistry.
 *
 * @param m   Amount + source currency. Currency must be registered.
 * @throws CurrencyUnknown if m.currency is not in the registry.
 * @throws InvalidRate     if the conversion rate is <= 0.
 * @throws BadInput        if m.amount is negative.
 * @return reference to *this to allow chaining.
 */
Account& operator+(const Money& m);
```

## 9. Summary

| Area | Decision |
|---|---|
| Layout | `apex-banking/{include,src}/`, Makefile, C++20 |
| Core classes | `Money`, `CurrencyRegistry`, `Transaction`, `Account`(abstract) + `Savings` / `Checking`, `Bank` |
| Storage | `std::map<string, unique_ptr<Account>>` in `Bank` |
| Operators | `Account + Money` = deposit, `Account - Money` = withdraw, both auto-convert |
| Transfer | `Bank::transfer` composes `-` then `+`, logged atomically |
| Errors | Exception hierarchy rooted at `BankError`; every failed op still hits the ledger |
| Error demos | Menu 12 (interleaved double-spend, buggy → fixed) + Menu 9/13 (reject rate ≤ 0) |
| Docs | Vietnamese README (install / build / run / demos) + English docblock comments |
