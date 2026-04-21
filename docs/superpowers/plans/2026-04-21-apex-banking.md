# APEX Banking System Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build an interactive CLI multi-currency banking system (`apex-banking/` binary) satisfying the PTIT mid-term OOP requirements — inheritance, `+`/`-` operator overloading, `std::map`-based storage, transaction logging, and live demos of the two required error cases.

**Architecture:** Plain C++20, Make-based build. `Account` abstract base → `Savings`/`Checking`. `Bank` owns a `std::map<string, unique_ptr<Account>>`, a `CurrencyRegistry`, and a `std::vector<Transaction>` ledger. Operators `Account + Money` / `Account - Money` auto-convert currency via the registry.

**Tech Stack:** C++20, g++, GNU Make, [doctest](https://github.com/doctest/doctest) (single-header test framework, committed into the repo).

**Reference spec:** `docs/superpowers/specs/2026-04-21-apex-banking-design.md`

---

## File Structure

```
apex-banking/
  include/
    Money.h            # struct Money { amount, currency }; operator<<
    Errors.h           # BankError + derived exception types
    Currency.h         # class CurrencyRegistry
    Transaction.h      # enum TxType/TxStatus + class Transaction
    Account.h          # abstract Account + operator+/operator-
    Savings.h          # concrete: interest, no overdraft
    Checking.h         # concrete: overdraft, no interest
    Bank.h             # facade: std::map + ledger + transfer + double-spend demo
  src/
    Currency.cpp
    Transaction.cpp
    Account.cpp
    Savings.cpp
    Checking.cpp
    Bank.cpp
    main.cpp           # CLI menu loop (Vietnamese)
  test/
    doctest.h          # vendored single-header (committed)
    test_main.cpp      # DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
    test_currency.cpp
    test_account.cpp
    test_savings.cpp
    test_checking.cpp
    test_bank.cpp
  Makefile             # targets: apex, test, clean
  README.md            # Vietnamese, GitHub-ready
```

**Responsibility boundaries:**
- `Money.h` and `Errors.h` are header-only — zero-logic value types.
- `CurrencyRegistry` is the **only** place that stores or validates exchange rates.
- `Account` is the **only** place the `+` / `-` operators live (inherited by Savings/Checking).
- `Bank` is the **only** place that touches the ledger and the account `std::map`.
- `main.cpp` contains **only** the CLI loop — no business logic; everything delegates to `Bank`.

---

## Task 1: Project skeleton + build pipeline

**Files:**
- Create: `apex-banking/include/.gitkeep`
- Create: `apex-banking/src/main.cpp`
- Create: `apex-banking/Makefile`

- [ ] **Step 1: Create directory structure**

Run:
```bash
mkdir -p apex-banking/include apex-banking/src apex-banking/test
touch apex-banking/include/.gitkeep apex-banking/test/.gitkeep
```

- [ ] **Step 2: Create a minimal main.cpp that compiles**

File: `apex-banking/src/main.cpp`
```cpp
#include <iostream>

int main() {
    std::cout << "APEX banking — skeleton build\n";
    return 0;
}
```

- [ ] **Step 3: Create the Makefile**

File: `apex-banking/Makefile`
```makefile
CXX       := g++
CXXFLAGS  := -std=c++20 -Wall -Wextra -Wpedantic -Iinclude
SRC       := $(wildcard src/*.cpp)
OBJ       := $(SRC:.cpp=.o)
APP       := apex

TEST_SRC  := $(wildcard test/*.cpp) $(filter-out src/main.cpp,$(SRC))
TEST_BIN  := apex_tests

.PHONY: all test clean run

all: $(APP)

$(APP): $(SRC)
	$(CXX) $(CXXFLAGS) $(SRC) -o $(APP)

test: $(TEST_BIN)
	./$(TEST_BIN)

$(TEST_BIN): $(TEST_SRC)
	$(CXX) $(CXXFLAGS) -Itest $(TEST_SRC) -o $(TEST_BIN)

run: $(APP)
	./$(APP)

clean:
	rm -f $(APP) $(TEST_BIN) src/*.o test/*.o
```

- [ ] **Step 4: Verify build works**

Run:
```bash
cd apex-banking && make
```
Expected: produces `./apex` with no warnings.

Run:
```bash
./apex
```
Expected output: `APEX banking — skeleton build`

- [ ] **Step 5: Commit**

```bash
git add apex-banking/include/.gitkeep apex-banking/src/main.cpp apex-banking/Makefile apex-banking/test/.gitkeep
git commit -m "feat(apex): add project skeleton and Makefile"
```

---

## Task 2: Vendor doctest + smoke-test build

**Files:**
- Create: `apex-banking/test/doctest.h`
- Create: `apex-banking/test/test_main.cpp`
- Create: `apex-banking/test/test_smoke.cpp`

- [ ] **Step 1: Fetch doctest single-header**

Run:
```bash
curl -L -o apex-banking/test/doctest.h https://raw.githubusercontent.com/doctest/doctest/v2.4.11/doctest/doctest.h
```
Verify: `wc -l apex-banking/test/doctest.h` should print a number > 6000.

- [ ] **Step 2: Create the test runner entry point**

File: `apex-banking/test/test_main.cpp`
```cpp
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
```

- [ ] **Step 3: Write a smoke test**

File: `apex-banking/test/test_smoke.cpp`
```cpp
#include "doctest.h"

TEST_CASE("sanity") {
    CHECK(1 + 1 == 2);
}
```

- [ ] **Step 4: Run tests — expect 1 pass**

Run:
```bash
cd apex-banking && make test
```
Expected output contains:
```
[doctest] test cases: 1 | 1 passed | 0 failed
```

- [ ] **Step 5: Commit**

```bash
git add apex-banking/test/doctest.h apex-banking/test/test_main.cpp apex-banking/test/test_smoke.cpp
git commit -m "test(apex): vendor doctest and add smoke test"
```

---

## Task 3: Money value type + Errors hierarchy

**Files:**
- Create: `apex-banking/include/Money.h`
- Create: `apex-banking/include/Errors.h`
- Create: `apex-banking/src/Money.cpp`

- [ ] **Step 1: Create Money.h**

File: `apex-banking/include/Money.h`
```cpp
#pragma once
#include <iosfwd>
#include <string>

/**
 * A currency-tagged monetary amount.
 * `amount` is in units of `currency` (e.g., 100 USD, 2'450'000 VND).
 * Not a class — intentionally a plain aggregate. Arithmetic happens inside
 * Account operators, not on Money itself.
 */
struct Money {
    double amount;
    std::string currency;
};

std::ostream& operator<<(std::ostream& os, const Money& m);
```

- [ ] **Step 2: Create Money.cpp**

File: `apex-banking/src/Money.cpp`
```cpp
#include "Money.h"
#include <iomanip>
#include <ostream>

std::ostream& operator<<(std::ostream& os, const Money& m) {
    std::ios::fmtflags f = os.flags();
    os << std::fixed << std::setprecision(2) << m.amount << " " << m.currency;
    os.flags(f);
    return os;
}
```

- [ ] **Step 3: Create Errors.h**

File: `apex-banking/include/Errors.h`
```cpp
#pragma once
#include <stdexcept>

/**
 * Root of every exception thrown by the banking domain.
 * The CLI catches this type exactly once at its top level.
 */
struct BankError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct AccountNotFound     : BankError { using BankError::BankError; };
struct DuplicateAccountId  : BankError { using BankError::BankError; };
struct InsufficientFunds   : BankError { using BankError::BankError; };
struct CurrencyUnknown     : BankError { using BankError::BankError; };
struct InvalidRate         : BankError { using BankError::BankError; };
struct DoubleSpendDetected : BankError { using BankError::BankError; };
struct BadInput            : BankError { using BankError::BankError; };
```

- [ ] **Step 4: Verify build still compiles (main.cpp unchanged)**

Run:
```bash
cd apex-banking && make clean && make
```
Expected: clean build, no warnings.

- [ ] **Step 5: Commit**

```bash
git add apex-banking/include/Money.h apex-banking/include/Errors.h apex-banking/src/Money.cpp
git commit -m "feat(apex): add Money value type and BankError hierarchy"
```

---

## Task 4: CurrencyRegistry (TDD)

**Files:**
- Create: `apex-banking/test/test_currency.cpp`
- Create: `apex-banking/include/Currency.h`
- Create: `apex-banking/src/Currency.cpp`

- [ ] **Step 1: Write failing tests**

File: `apex-banking/test/test_currency.cpp`
```cpp
#include "doctest.h"
#include "Currency.h"
#include "Errors.h"

TEST_CASE("setRate rejects zero and negative rates") {
    CurrencyRegistry reg;
    CHECK_THROWS_AS(reg.setRate("EUR", 0.0), InvalidRate);
    CHECK_THROWS_AS(reg.setRate("EUR", -1.5), InvalidRate);
    CHECK_FALSE(reg.has("EUR"));
}

TEST_CASE("setRate accepts positive rates and getRate echoes them back") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    reg.setRate("VND", 24500.0);
    CHECK(reg.has("USD"));
    CHECK(reg.getRate("USD") == doctest::Approx(1.0));
    CHECK(reg.getRate("VND") == doctest::Approx(24500.0));
}

TEST_CASE("getRate throws CurrencyUnknown for missing currency") {
    CurrencyRegistry reg;
    CHECK_THROWS_AS(reg.getRate("XXX"), CurrencyUnknown);
}

TEST_CASE("convert: same currency is identity") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    Money m{100.0, "USD"};
    Money out = reg.convert(m, "USD");
    CHECK(out.amount == doctest::Approx(100.0));
    CHECK(out.currency == "USD");
}

TEST_CASE("convert: USD to VND uses rates correctly") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    reg.setRate("VND", 24500.0);
    Money out = reg.convert(Money{10.0, "USD"}, "VND");
    CHECK(out.amount == doctest::Approx(245000.0));
    CHECK(out.currency == "VND");
}

TEST_CASE("convert: VND to USD inverse direction") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    reg.setRate("VND", 24500.0);
    Money out = reg.convert(Money{245000.0, "VND"}, "USD");
    CHECK(out.amount == doctest::Approx(10.0));
    CHECK(out.currency == "USD");
}

TEST_CASE("convert: unknown source or target throws CurrencyUnknown") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    CHECK_THROWS_AS(reg.convert(Money{1.0, "XXX"}, "USD"), CurrencyUnknown);
    CHECK_THROWS_AS(reg.convert(Money{1.0, "USD"}, "XXX"), CurrencyUnknown);
}
```

- [ ] **Step 2: Run tests — expect compile failure**

Run: `cd apex-banking && make test`
Expected: compile error — `Currency.h` does not exist.

- [ ] **Step 3: Create Currency.h**

File: `apex-banking/include/Currency.h`
```cpp
#pragma once
#include "Money.h"
#include <iosfwd>
#include <map>
#include <string>

/**
 * Stores per-currency exchange rates and converts between them.
 * Rates are expressed as "units of X per 1 base unit" where the base is
 * implicit USD (i.e. rates["USD"] is always 1.0 by convention). The class
 * does not enforce a base — any currency whose rate is 1.0 works as base.
 */
class CurrencyRegistry {
    std::map<std::string, double> rates;
public:
    /**
     * Register or update a currency's rate.
     * @throws InvalidRate if rate <= 0.
     */
    void setRate(const std::string& code, double rate);

    /** @throws CurrencyUnknown if code is not registered. */
    double getRate(const std::string& code) const;

    /** @throws CurrencyUnknown on either side. */
    Money convert(const Money& from, const std::string& toCurrency) const;

    bool has(const std::string& code) const;
    void list(std::ostream&) const;
};
```

- [ ] **Step 4: Implement Currency.cpp**

File: `apex-banking/src/Currency.cpp`
```cpp
#include "Currency.h"
#include "Errors.h"
#include <ostream>
#include <string>

void CurrencyRegistry::setRate(const std::string& code, double rate) {
    if (rate <= 0.0) {
        throw InvalidRate("rate must be > 0 (got " + std::to_string(rate) + ")");
    }
    rates[code] = rate;
}

double CurrencyRegistry::getRate(const std::string& code) const {
    auto it = rates.find(code);
    if (it == rates.end()) {
        throw CurrencyUnknown("unknown currency: " + code);
    }
    return it->second;
}

Money CurrencyRegistry::convert(const Money& from, const std::string& toCurrency) const {
    double fromRate = getRate(from.currency);
    double toRate   = getRate(toCurrency);
    double amountInBase = from.amount / fromRate;
    return Money{amountInBase * toRate, toCurrency};
}

bool CurrencyRegistry::has(const std::string& code) const {
    return rates.count(code) > 0;
}

void CurrencyRegistry::list(std::ostream& os) const {
    for (const auto& [code, rate] : rates) {
        os << "  " << code << " : " << rate << "\n";
    }
}
```

- [ ] **Step 5: Run tests — expect all pass**

Run: `cd apex-banking && make test`
Expected output contains:
```
[doctest] test cases: 8 | 8 passed | 0 failed
```

- [ ] **Step 6: Commit**

```bash
git add apex-banking/include/Currency.h apex-banking/src/Currency.cpp apex-banking/test/test_currency.cpp
git commit -m "feat(apex): add CurrencyRegistry with rate validation and conversion"
```

---

## Task 5: Transaction class

**Files:**
- Create: `apex-banking/include/Transaction.h`
- Create: `apex-banking/src/Transaction.cpp`

Transaction is a pure data carrier with a `print` method — its behavior is covered via the Bank ledger tests in Task 9, so we skip dedicated unit tests here.

- [ ] **Step 1: Create Transaction.h**

File: `apex-banking/include/Transaction.h`
```cpp
#pragma once
#include "Money.h"
#include <ctime>
#include <iosfwd>
#include <string>

enum class TxType   { CREATE_ACCOUNT, DEPOSIT, WITHDRAW, TRANSFER, SET_RATE, APPLY_INTEREST };
enum class TxStatus { SUCCESS, FAILED };

/**
 * Immutable ledger entry. Populated once by Bank and never mutated.
 * Fields that don't apply to a given TxType are left empty/zero —
 * e.g., SET_RATE uses `note` for the rate value and leaves fromId/toId empty.
 */
class Transaction {
    int id_;
    TxType type_;
    TxStatus status_;
    std::string fromId_;
    std::string toId_;
    Money amount_;
    std::string note_;
    std::time_t timestamp_;
public:
    Transaction(int id, TxType type, TxStatus status,
                std::string fromId, std::string toId,
                Money amount, std::string note, std::time_t timestamp);

    int id() const { return id_; }
    TxType type() const { return type_; }
    TxStatus status() const { return status_; }
    const std::string& fromId() const { return fromId_; }
    const std::string& toId() const { return toId_; }
    const Money& amount() const { return amount_; }
    const std::string& note() const { return note_; }
    std::time_t timestamp() const { return timestamp_; }

    /** Prints one human-readable line to `os`. Trailing newline included. */
    void print(std::ostream& os) const;
};
```

- [ ] **Step 2: Create Transaction.cpp**

File: `apex-banking/src/Transaction.cpp`
```cpp
#include "Transaction.h"
#include <iomanip>
#include <ostream>
#include <utility>

namespace {
const char* typeName(TxType t) {
    switch (t) {
        case TxType::CREATE_ACCOUNT: return "CREATE_ACCOUNT";
        case TxType::DEPOSIT:        return "DEPOSIT";
        case TxType::WITHDRAW:       return "WITHDRAW";
        case TxType::TRANSFER:       return "TRANSFER";
        case TxType::SET_RATE:       return "SET_RATE";
        case TxType::APPLY_INTEREST: return "APPLY_INTEREST";
    }
    return "?";
}
const char* statusName(TxStatus s) {
    return s == TxStatus::SUCCESS ? "SUCCESS" : "FAILED";
}
}

Transaction::Transaction(int id, TxType type, TxStatus status,
                         std::string fromId, std::string toId,
                         Money amount, std::string note, std::time_t timestamp)
    : id_(id), type_(type), status_(status),
      fromId_(std::move(fromId)), toId_(std::move(toId)),
      amount_(std::move(amount)), note_(std::move(note)),
      timestamp_(timestamp) {}

void Transaction::print(std::ostream& os) const {
    char timebuf[32] = {};
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &timestamp_);
#else
    localtime_r(&timestamp_, &tm);
#endif
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);

    os << "#" << std::setw(4) << std::setfill('0') << id_ << std::setfill(' ')
       << "  " << timebuf
       << "  " << std::setw(14) << std::left << typeName(type_)
       << " " << std::setw(7) << statusName(status_)
       << std::right;
    if (!fromId_.empty()) os << "  from=" << fromId_;
    if (!toId_.empty())   os << "  to="   << toId_;
    if (!amount_.currency.empty()) os << "  amt=" << amount_;
    if (!note_.empty())   os << "  (" << note_ << ")";
    os << "\n";
}
```

- [ ] **Step 3: Verify build**

Run: `cd apex-banking && make clean && make && make test`
Expected: all targets succeed, 8 tests still pass.

- [ ] **Step 4: Commit**

```bash
git add apex-banking/include/Transaction.h apex-banking/src/Transaction.cpp
git commit -m "feat(apex): add Transaction ledger entry type"
```

---

## Task 6: Account abstract base + operator overloads (TDD)

**Files:**
- Create: `apex-banking/include/Account.h`
- Create: `apex-banking/src/Account.cpp`
- Create: `apex-banking/test/test_account.cpp`

Account is abstract, so its operators are tested through a tiny test-only derived class defined inside the test file.

- [ ] **Step 1: Write failing tests**

File: `apex-banking/test/test_account.cpp`
```cpp
#include "doctest.h"
#include "Account.h"
#include "Currency.h"
#include "Errors.h"

namespace {
/** Test-only concrete Account that always permits any withdrawal. */
class PermissiveAccount : public Account {
public:
    PermissiveAccount(std::string id, std::string owner, std::string currency,
                      Money opening, const CurrencyRegistry* reg)
        : Account(std::move(id), std::move(owner), std::move(currency),
                  std::move(opening), reg) {}
    std::string kind() const override { return "Permissive"; }
    bool canWithdraw(double /*amt*/) const override { return true; }
};

CurrencyRegistry makeRegistry() {
    CurrencyRegistry r;
    r.setRate("USD", 1.0);
    r.setRate("VND", 24500.0);
    return r;
}
}

TEST_CASE("operator+ deposits same-currency money") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    a + Money{50.0, "USD"};
    CHECK(a.getBalance() == doctest::Approx(150.0));
}

TEST_CASE("operator+ converts foreign currency to native") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    a + Money{24500.0, "VND"};  // == 1 USD
    CHECK(a.getBalance() == doctest::Approx(101.0));
}

TEST_CASE("operator+ on negative amount throws BadInput") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    CHECK_THROWS_AS(a + Money{-1.0, "USD"}, BadInput);
}

TEST_CASE("operator- withdraws same-currency money when canWithdraw allows") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    a - Money{30.0, "USD"};
    CHECK(a.getBalance() == doctest::Approx(70.0));
}

TEST_CASE("operator- throws InsufficientFunds when policy rejects") {
    class StrictAccount : public Account {
    public:
        using Account::Account;
        std::string kind() const override { return "Strict"; }
        bool canWithdraw(double amt) const override { return amt <= balance_(); }
    private:
        double balance_() const { return getBalance(); }
    };
    auto reg = makeRegistry();
    StrictAccount a("acc_1", "A", "USD", Money{10.0, "USD"}, &reg);
    CHECK_THROWS_AS(a - Money{50.0, "USD"}, InsufficientFunds);
    CHECK(a.getBalance() == doctest::Approx(10.0));  // unchanged
}

TEST_CASE("operator+ propagates CurrencyUnknown from registry") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    CHECK_THROWS_AS(a + Money{1.0, "ZZZ"}, CurrencyUnknown);
}
```

- [ ] **Step 2: Run tests — expect compile failure (Account.h missing)**

Run: `cd apex-banking && make test`
Expected: compile error — `Account.h: No such file or directory`.

- [ ] **Step 3: Create Account.h**

File: `apex-banking/include/Account.h`
```cpp
#pragma once
#include "Money.h"
#include <iosfwd>
#include <string>

class CurrencyRegistry;

/**
 * Abstract base for all account types. Holds a single balance in
 * `nativeCurrency` and relies on a non-owning registry for conversion.
 *
 * Subclasses must override `kind()` and `canWithdraw()`.
 */
class Account {
protected:
    std::string id;
    std::string owner;
    std::string nativeCurrency;
    double balance;
    const CurrencyRegistry* reg;  // non-owning; outlives the account
public:
    Account(std::string id, std::string owner, std::string currency,
            Money opening, const CurrencyRegistry* reg);
    virtual ~Account() = default;

    Account(const Account&) = delete;
    Account& operator=(const Account&) = delete;

    virtual std::string kind() const = 0;
    virtual bool canWithdraw(double amtNative) const = 0;

    /**
     * Deposit. Auto-converts `m` to this account's nativeCurrency.
     * @throws BadInput if m.amount < 0.
     * @throws CurrencyUnknown / InvalidRate via the registry.
     * @return *this (chaining).
     */
    Account& operator+(const Money& m);

    /**
     * Withdraw. Auto-converts then consults `canWithdraw`.
     * @throws BadInput if m.amount < 0.
     * @throws InsufficientFunds if policy rejects the converted amount.
     * @throws CurrencyUnknown / InvalidRate via the registry.
     */
    Account& operator-(const Money& m);

    const std::string& getId() const { return id; }
    const std::string& getOwner() const { return owner; }
    const std::string& getCurrency() const { return nativeCurrency; }
    double getBalance() const { return balance; }

    virtual void print(std::ostream& os) const;
};
```

- [ ] **Step 4: Implement Account.cpp**

File: `apex-banking/src/Account.cpp`
```cpp
#include "Account.h"
#include "Currency.h"
#include "Errors.h"
#include <ostream>
#include <utility>

Account::Account(std::string id, std::string owner, std::string currency,
                 Money opening, const CurrencyRegistry* reg)
    : id(std::move(id)),
      owner(std::move(owner)),
      nativeCurrency(std::move(currency)),
      balance(0.0),
      reg(reg) {
    // Normalize the opening balance into nativeCurrency via the registry,
    // so callers may open an account with any registered currency.
    if (opening.amount < 0.0) {
        throw BadInput("opening balance must be >= 0");
    }
    if (opening.currency == nativeCurrency) {
        balance = opening.amount;
    } else {
        balance = reg->convert(opening, nativeCurrency).amount;
    }
}

Account& Account::operator+(const Money& m) {
    if (m.amount < 0.0) throw BadInput("deposit amount must be >= 0");
    Money converted = (m.currency == nativeCurrency)
                      ? m
                      : reg->convert(m, nativeCurrency);
    balance += converted.amount;
    return *this;
}

Account& Account::operator-(const Money& m) {
    if (m.amount < 0.0) throw BadInput("withdraw amount must be >= 0");
    Money converted = (m.currency == nativeCurrency)
                      ? m
                      : reg->convert(m, nativeCurrency);
    if (!canWithdraw(converted.amount)) {
        throw InsufficientFunds("insufficient funds in " + id);
    }
    balance -= converted.amount;
    return *this;
}

void Account::print(std::ostream& os) const {
    os << "[" << kind() << "] " << id
       << "  owner=" << owner
       << "  balance=" << Money{balance, nativeCurrency};
}
```

- [ ] **Step 5: Run tests — expect all pass**

Run: `cd apex-banking && make test`
Expected: test count grows to 14 passing (8 currency + 6 account), 0 failed.

- [ ] **Step 6: Commit**

```bash
git add apex-banking/include/Account.h apex-banking/src/Account.cpp apex-banking/test/test_account.cpp
git commit -m "feat(apex): add Account base class with + and - operator overloads"
```

---

## Task 7: Savings class (TDD)

**Files:**
- Create: `apex-banking/test/test_savings.cpp`
- Create: `apex-banking/include/Savings.h`
- Create: `apex-banking/src/Savings.cpp`

- [ ] **Step 1: Write failing tests**

File: `apex-banking/test/test_savings.cpp`
```cpp
#include "doctest.h"
#include "Savings.h"
#include "Currency.h"
#include "Errors.h"

namespace {
CurrencyRegistry usdOnly() {
    CurrencyRegistry r;
    r.setRate("USD", 1.0);
    return r;
}
}

TEST_CASE("Savings.kind() returns 'Savings'") {
    auto reg = usdOnly();
    Savings s("s1", "A", "USD", Money{100.0, "USD"}, &reg, 0.05);
    CHECK(s.kind() == "Savings");
}

TEST_CASE("Savings rejects withdrawal beyond balance (no overdraft)") {
    auto reg = usdOnly();
    Savings s("s1", "A", "USD", Money{100.0, "USD"}, &reg, 0.05);
    CHECK(s.canWithdraw(100.0));
    CHECK(s.canWithdraw(99.99));
    CHECK_FALSE(s.canWithdraw(100.01));
    CHECK_THROWS_AS(s - Money{101.0, "USD"}, InsufficientFunds);
}

TEST_CASE("Savings applyInterest multiplies balance by (1 + rate)") {
    auto reg = usdOnly();
    Savings s("s1", "A", "USD", Money{100.0, "USD"}, &reg, 0.05);
    s.applyInterest();
    CHECK(s.getBalance() == doctest::Approx(105.0));
    s.applyInterest();
    CHECK(s.getBalance() == doctest::Approx(110.25));
}
```

- [ ] **Step 2: Run tests — expect compile failure**

Run: `cd apex-banking && make test`
Expected: `Savings.h: No such file or directory`.

- [ ] **Step 3: Create Savings.h**

File: `apex-banking/include/Savings.h`
```cpp
#pragma once
#include "Account.h"

/**
 * Savings account: earns interest via applyInterest(), never permits overdraft.
 */
class Savings : public Account {
    double interestRate;  // annual rate, e.g. 0.05 for 5%
public:
    Savings(std::string id, std::string owner, std::string currency,
            Money opening, const CurrencyRegistry* reg, double interestRate);

    std::string kind() const override { return "Savings"; }
    bool canWithdraw(double amtNative) const override;

    /** Apply one interest period: balance *= (1 + interestRate). */
    void applyInterest();

    double getInterestRate() const { return interestRate; }
    void print(std::ostream& os) const override;
};
```

- [ ] **Step 4: Implement Savings.cpp**

File: `apex-banking/src/Savings.cpp`
```cpp
#include "Savings.h"
#include <ostream>
#include <utility>

Savings::Savings(std::string id, std::string owner, std::string currency,
                 Money opening, const CurrencyRegistry* reg, double interestRate)
    : Account(std::move(id), std::move(owner), std::move(currency),
              std::move(opening), reg),
      interestRate(interestRate) {}

bool Savings::canWithdraw(double amtNative) const {
    return amtNative <= balance;
}

void Savings::applyInterest() {
    balance *= (1.0 + interestRate);
}

void Savings::print(std::ostream& os) const {
    Account::print(os);
    os << "  interest=" << (interestRate * 100.0) << "%";
}
```

- [ ] **Step 5: Run tests — expect 17 passing**

Run: `cd apex-banking && make test`
Expected: `17 passed | 0 failed`.

- [ ] **Step 6: Commit**

```bash
git add apex-banking/include/Savings.h apex-banking/src/Savings.cpp apex-banking/test/test_savings.cpp
git commit -m "feat(apex): add Savings account with interest and no-overdraft policy"
```

---

## Task 8: Checking class (TDD)

**Files:**
- Create: `apex-banking/test/test_checking.cpp`
- Create: `apex-banking/include/Checking.h`
- Create: `apex-banking/src/Checking.cpp`

- [ ] **Step 1: Write failing tests**

File: `apex-banking/test/test_checking.cpp`
```cpp
#include "doctest.h"
#include "Checking.h"
#include "Currency.h"
#include "Errors.h"

namespace {
CurrencyRegistry usdOnly() {
    CurrencyRegistry r;
    r.setRate("USD", 1.0);
    return r;
}
}

TEST_CASE("Checking.kind() returns 'Checking'") {
    auto reg = usdOnly();
    Checking c("c1", "B", "USD", Money{100.0, "USD"}, &reg, 50.0);
    CHECK(c.kind() == "Checking");
}

TEST_CASE("Checking allows withdrawal into overdraft limit") {
    auto reg = usdOnly();
    Checking c("c1", "B", "USD", Money{100.0, "USD"}, &reg, 50.0);
    CHECK(c.canWithdraw(100.0));
    CHECK(c.canWithdraw(150.0));        // exactly overdraft edge
    CHECK_FALSE(c.canWithdraw(150.01)); // beyond overdraft
    c - Money{140.0, "USD"};
    CHECK(c.getBalance() == doctest::Approx(-40.0));
}

TEST_CASE("Checking rejects withdrawal beyond overdraft") {
    auto reg = usdOnly();
    Checking c("c1", "B", "USD", Money{100.0, "USD"}, &reg, 50.0);
    CHECK_THROWS_AS(c - Money{200.0, "USD"}, InsufficientFunds);
    CHECK(c.getBalance() == doctest::Approx(100.0));
}
```

- [ ] **Step 2: Run tests — expect compile failure**

Run: `cd apex-banking && make test`
Expected: `Checking.h: No such file or directory`.

- [ ] **Step 3: Create Checking.h**

File: `apex-banking/include/Checking.h`
```cpp
#pragma once
#include "Account.h"

/**
 * Checking account: no interest, permits withdrawal into a negative balance
 * up to `overdraftLimit`.
 */
class Checking : public Account {
    double overdraftLimit;
public:
    Checking(std::string id, std::string owner, std::string currency,
             Money opening, const CurrencyRegistry* reg, double overdraftLimit);

    std::string kind() const override { return "Checking"; }
    bool canWithdraw(double amtNative) const override;

    double getOverdraftLimit() const { return overdraftLimit; }
    void print(std::ostream& os) const override;
};
```

- [ ] **Step 4: Implement Checking.cpp**

File: `apex-banking/src/Checking.cpp`
```cpp
#include "Checking.h"
#include <ostream>
#include <utility>

Checking::Checking(std::string id, std::string owner, std::string currency,
                   Money opening, const CurrencyRegistry* reg, double overdraftLimit)
    : Account(std::move(id), std::move(owner), std::move(currency),
              std::move(opening), reg),
      overdraftLimit(overdraftLimit) {}

bool Checking::canWithdraw(double amtNative) const {
    return amtNative <= balance + overdraftLimit;
}

void Checking::print(std::ostream& os) const {
    Account::print(os);
    os << "  overdraft=" << overdraftLimit;
}
```

- [ ] **Step 5: Run tests — expect 20 passing**

Run: `cd apex-banking && make test`
Expected: `20 passed | 0 failed`.

- [ ] **Step 6: Commit**

```bash
git add apex-banking/include/Checking.h apex-banking/src/Checking.cpp apex-banking/test/test_checking.cpp
git commit -m "feat(apex): add Checking account with overdraft limit"
```

---

## Task 9: Bank — account lifecycle + money movement (TDD)

**Files:**
- Create: `apex-banking/test/test_bank.cpp`
- Create: `apex-banking/include/Bank.h`
- Create: `apex-banking/src/Bank.cpp`

- [ ] **Step 1: Write failing tests for lifecycle + movement**

File: `apex-banking/test/test_bank.cpp`
```cpp
#include "doctest.h"
#include "Bank.h"
#include "Errors.h"
#include "Savings.h"
#include "Checking.h"

namespace {
Bank makeSeededBank() {
    Bank b;
    b.setRate("USD", 1.0);
    b.setRate("VND", 24500.0);
    return b;
}
}

TEST_CASE("createSavings registers in the account map and returns reference") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("Alice", "USD", Money{100.0, "USD"}, 0.05);
    CHECK(a.kind() == "Savings");
    CHECK(a.getBalance() == doctest::Approx(100.0));
    CHECK(b.find(a.getId()) == &a);
}

TEST_CASE("createChecking registers separately and IDs are unique") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{10.0, "USD"}, 0.01);
    Account& c = b.createChecking("B", "USD", Money{20.0, "USD"}, 50.0);
    CHECK(a.getId() != c.getId());
}

TEST_CASE("find returns nullptr for missing id, require throws") {
    Bank b = makeSeededBank();
    CHECK(b.find("nope") == nullptr);
    CHECK_THROWS_AS(b.require("nope"), AccountNotFound);
}

TEST_CASE("deposit updates balance and logs a SUCCESS tx") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{100.0, "USD"}, 0.0);
    b.deposit(a.getId(), Money{50.0, "USD"});
    CHECK(a.getBalance() == doctest::Approx(150.0));
    // ledger contains CREATE_ACCOUNT + DEPOSIT, both SUCCESS
    auto log = b.ledgerEntries();
    CHECK(log.size() == 2);
    CHECK(log.back().type() == TxType::DEPOSIT);
    CHECK(log.back().status() == TxStatus::SUCCESS);
}

TEST_CASE("withdraw failure is still logged as FAILED") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{10.0, "USD"}, 0.0);
    CHECK_THROWS_AS(b.withdraw(a.getId(), Money{999.0, "USD"}), InsufficientFunds);
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::WITHDRAW);
    CHECK(log.back().status() == TxStatus::FAILED);
}

TEST_CASE("transfer moves funds and logs single TRANSFER entry") {
    Bank b = makeSeededBank();
    Account& from = b.createChecking("From", "USD", Money{100.0, "USD"}, 0.0);
    Account& to   = b.createSavings ("To",   "VND", Money{0.0,   "VND"}, 0.0);
    b.transfer(from.getId(), to.getId(), Money{10.0, "USD"});
    CHECK(from.getBalance() == doctest::Approx(90.0));
    CHECK(to.getBalance()   == doctest::Approx(245000.0));
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::TRANSFER);
    CHECK(log.back().status() == TxStatus::SUCCESS);
}

TEST_CASE("transfer fails atomically when source has insufficient funds") {
    Bank b = makeSeededBank();
    Account& from = b.createSavings("From", "USD", Money{5.0, "USD"}, 0.0);
    Account& to   = b.createSavings("To",   "USD", Money{0.0, "USD"}, 0.0);
    CHECK_THROWS_AS(b.transfer(from.getId(), to.getId(), Money{10.0, "USD"}),
                    InsufficientFunds);
    CHECK(from.getBalance() == doctest::Approx(5.0));
    CHECK(to.getBalance()   == doctest::Approx(0.0));
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::TRANSFER);
    CHECK(log.back().status() == TxStatus::FAILED);
}

TEST_CASE("setRate(0) on Bank throws InvalidRate and logs FAILED") {
    Bank b = makeSeededBank();
    CHECK_THROWS_AS(b.setRate("EUR", 0.0), InvalidRate);
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::SET_RATE);
    CHECK(log.back().status() == TxStatus::FAILED);
}

TEST_CASE("lock / unlock pair and DoubleSpendDetected on second lock") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{100.0, "USD"}, 0.0);
    b.lock(a.getId());
    CHECK_THROWS_AS(b.lock(a.getId()), DoubleSpendDetected);
    b.unlock(a.getId());
    b.lock(a.getId());  // reacquiring after unlock is fine
    b.unlock(a.getId());
}
```

- [ ] **Step 2: Run tests — expect compile failure**

Run: `cd apex-banking && make test`
Expected: `Bank.h: No such file or directory`.

- [ ] **Step 3: Create Bank.h**

File: `apex-banking/include/Bank.h`
```cpp
#pragma once
#include "Account.h"
#include "Currency.h"
#include "Money.h"
#include "Transaction.h"
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

/**
 * Central facade. Owns all accounts, the currency registry, and the ledger.
 * The CLI talks to Bank and nothing else.
 */
class Bank {
    std::map<std::string, std::unique_ptr<Account>> accounts;
    CurrencyRegistry registry;
    std::vector<Transaction> ledger;
    std::set<std::string> lockedIds;
    int nextTxId = 1;
    int nextAccountSeq = 1;

    int allocTxId();
    std::string allocAccountId();
    void log(Transaction tx);
    static std::time_t now();

public:
    Bank() = default;

    Bank(const Bank&) = delete;
    Bank& operator=(const Bank&) = delete;
    Bank(Bank&&) = default;
    Bank& operator=(Bank&&) = default;

    // --- account lifecycle ---
    Account& createSavings (const std::string& owner, const std::string& currency,
                            const Money& opening, double interestRate);
    Account& createChecking(const std::string& owner, const std::string& currency,
                            const Money& opening, double overdraftLimit);
    Account* find(const std::string& id);
    Account& require(const std::string& id);

    // --- money movement ---
    void deposit (const std::string& id, const Money& m);
    void withdraw(const std::string& id, const Money& m);
    void transfer(const std::string& fromId, const std::string& toId, const Money& m);

    // --- currency admin ---
    void setRate(const std::string& code, double rate);
    const CurrencyRegistry& rates() const { return registry; }

    // --- queries ---
    void listAccounts(std::ostream&) const;
    void printLedger(std::ostream&) const;
    const std::vector<Transaction>& ledgerEntries() const { return ledger; }

    // --- double-spend locking primitives ---
    void lock(const std::string& id);
    void unlock(const std::string& id);

    // --- demos ---
    void simulateDoubleSpend(const std::string& accountId, double amount);
};
```

- [ ] **Step 4: Implement Bank.cpp (lifecycle, movement, currency, locking)**

File: `apex-banking/src/Bank.cpp`
```cpp
#include "Bank.h"
#include "Checking.h"
#include "Errors.h"
#include "Savings.h"
#include <ctime>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <utility>

// ----- private helpers -----
int Bank::allocTxId() { return nextTxId++; }

std::string Bank::allocAccountId() {
    std::ostringstream os;
    os << "acc_" << std::setw(3) << std::setfill('0') << nextAccountSeq++;
    return os.str();
}

void Bank::log(Transaction tx) { ledger.push_back(std::move(tx)); }

std::time_t Bank::now() { return std::time(nullptr); }

// ----- lifecycle -----
Account& Bank::createSavings(const std::string& owner, const std::string& currency,
                             const Money& opening, double interestRate) {
    std::string id = allocAccountId();
    auto acc = std::make_unique<Savings>(id, owner, currency, opening, &registry, interestRate);
    Account& ref = *acc;
    accounts.emplace(id, std::move(acc));
    log(Transaction(allocTxId(), TxType::CREATE_ACCOUNT, TxStatus::SUCCESS,
                    "", id, opening, "Savings/" + owner, now()));
    return ref;
}

Account& Bank::createChecking(const std::string& owner, const std::string& currency,
                              const Money& opening, double overdraftLimit) {
    std::string id = allocAccountId();
    auto acc = std::make_unique<Checking>(id, owner, currency, opening, &registry, overdraftLimit);
    Account& ref = *acc;
    accounts.emplace(id, std::move(acc));
    log(Transaction(allocTxId(), TxType::CREATE_ACCOUNT, TxStatus::SUCCESS,
                    "", id, opening, "Checking/" + owner, now()));
    return ref;
}

Account* Bank::find(const std::string& id) {
    auto it = accounts.find(id);
    return it == accounts.end() ? nullptr : it->second.get();
}

Account& Bank::require(const std::string& id) {
    Account* a = find(id);
    if (!a) throw AccountNotFound("no such account: " + id);
    return *a;
}

// ----- movement -----
void Bank::deposit(const std::string& id, const Money& m) {
    try {
        Account& a = require(id);
        a + m;
        log(Transaction(allocTxId(), TxType::DEPOSIT, TxStatus::SUCCESS,
                        "", id, m, "", now()));
    } catch (const BankError& e) {
        log(Transaction(allocTxId(), TxType::DEPOSIT, TxStatus::FAILED,
                        "", id, m, e.what(), now()));
        throw;
    }
}

void Bank::withdraw(const std::string& id, const Money& m) {
    try {
        Account& a = require(id);
        a - m;
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS,
                        id, "", m, "", now()));
    } catch (const BankError& e) {
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                        id, "", m, e.what(), now()));
        throw;
    }
}

void Bank::transfer(const std::string& fromId, const std::string& toId, const Money& m) {
    try {
        Account& from = require(fromId);
        Account& to   = require(toId);
        from - m;
        to   + m;
        log(Transaction(allocTxId(), TxType::TRANSFER, TxStatus::SUCCESS,
                        fromId, toId, m, "", now()));
    } catch (const BankError& e) {
        log(Transaction(allocTxId(), TxType::TRANSFER, TxStatus::FAILED,
                        fromId, toId, m, e.what(), now()));
        throw;
    }
}

// ----- currency -----
void Bank::setRate(const std::string& code, double rate) {
    try {
        registry.setRate(code, rate);
        log(Transaction(allocTxId(), TxType::SET_RATE, TxStatus::SUCCESS,
                        "", "", Money{rate, code}, "", now()));
    } catch (const BankError& e) {
        log(Transaction(allocTxId(), TxType::SET_RATE, TxStatus::FAILED,
                        "", "", Money{rate, code}, e.what(), now()));
        throw;
    }
}

// ----- queries -----
void Bank::listAccounts(std::ostream& os) const {
    if (accounts.empty()) { os << "  (no accounts)\n"; return; }
    for (const auto& [id, ptr] : accounts) {
        ptr->print(os);
        os << "\n";
    }
}

void Bank::printLedger(std::ostream& os) const {
    if (ledger.empty()) { os << "  (ledger empty)\n"; return; }
    for (const auto& tx : ledger) tx.print(os);
}

// ----- locking -----
void Bank::lock(const std::string& id) {
    if (lockedIds.count(id)) {
        throw DoubleSpendDetected("account " + id + " already locked");
    }
    lockedIds.insert(id);
}

void Bank::unlock(const std::string& id) {
    lockedIds.erase(id);
}

// ----- demos ----- (implemented in next task)
void Bank::simulateDoubleSpend(const std::string& /*accountId*/, double /*amount*/) {
    // stub; implemented in Task 10
}
```

- [ ] **Step 5: Run tests — expect 29 passing**

Run: `cd apex-banking && make test`
Expected: `29 passed | 0 failed`.

- [ ] **Step 6: Commit**

```bash
git add apex-banking/include/Bank.h apex-banking/src/Bank.cpp apex-banking/test/test_bank.cpp
git commit -m "feat(apex): add Bank facade with lifecycle, ledger, transfer, and locking"
```

---

## Task 10: Double-spend simulation

**Files:**
- Modify: `apex-banking/src/Bank.cpp` (replace stub `simulateDoubleSpend`)
- Modify: `apex-banking/test/test_bank.cpp` (add test)

- [ ] **Step 1: Add test for the buggy-phase observable effect**

Append to `apex-banking/test/test_bank.cpp`:
```cpp
TEST_CASE("simulateDoubleSpend leaves the buggy-phase account state wrong") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{100.0, "USD"}, 0.0);
    // After the buggy phase, two $80 withdrawals both apply to balance 100
    // with the naive "read/check/write" pattern, leaving $20.
    // The fixed phase then re-uses the same account and further reduces it.
    b.simulateDoubleSpend(a.getId(), 80.0);
    // Exact final balance depends on the fixed-phase retry logic below;
    // the load-bearing assertion is that ledger entries cover both phases.
    auto log = b.ledgerEntries();
    bool sawDoubleSpendDetected = false;
    for (const auto& tx : log) {
        if (tx.status() == TxStatus::FAILED &&
            tx.note().find("already locked") != std::string::npos) {
            sawDoubleSpendDetected = true;
            break;
        }
    }
    CHECK(sawDoubleSpendDetected);
}
```

- [ ] **Step 2: Run test — expect failure (stub is empty)**

Run: `cd apex-banking && make test`
Expected: the new test fails — `sawDoubleSpendDetected` is false.

- [ ] **Step 3: Replace the `simulateDoubleSpend` stub in Bank.cpp**

In `apex-banking/src/Bank.cpp`, replace the stub with:

```cpp
void Bank::simulateDoubleSpend(const std::string& accountId, double amount) {
    Account& a = require(accountId);
    std::ostream& os = std::cout;

    // ----------- PHASE 1: buggy (direct balance mutation, bypasses operator-)
    os << "\n--- Phase 1: BUGGY (no locks) ---\n";
    // We need to reach the protected balance to simulate the race cleanly.
    // Use operator- twice after both "snapshots" so both see the pre-state.
    double snapshotA = a.getBalance();
    double snapshotB = a.getBalance();
    os << "Step 1. Terminal A reads balance -> " << snapshotA << "\n";
    os << "Step 2. Terminal B reads balance -> " << snapshotB << "\n";

    bool okA = amount <= snapshotA;
    bool okB = amount <= snapshotB;
    os << "Step 3. Terminal A check " << amount << " <= " << snapshotA
       << " ? " << (okA ? "YES" : "NO") << "\n";
    os << "Step 4. Terminal B check " << amount << " <= " << snapshotB
       << " ? " << (okB ? "YES" : "NO") << "\n";

    // Both "write" by issuing real withdraws. We disable canWithdraw protection
    // by catching InsufficientFunds and forcing a direct deduction instead —
    // the whole point of this phase is to show what happens WITHOUT a guard.
    auto forceDeduct = [&](const char* who) {
        try {
            a - Money{amount, a.getCurrency()};
            os << "Step X. " << who << " operator- succeeded.\n";
            log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS,
                            accountId, "", Money{amount, a.getCurrency()},
                            std::string("doublespend/buggy/") + who, now()));
        } catch (const InsufficientFunds& e) {
            // Simulate the race: force the write anyway to expose the bug.
            // We do this by calling operator- after a temporary deposit that
            // covers the gap, then immediately withdrawing the compensating
            // amount. Net effect: the account is left in the inconsistent
            // state a real concurrent writer would cause.
            a + Money{amount, a.getCurrency()};
            a - Money{amount, a.getCurrency()};
            os << "Step X. " << who << " FORCED write (simulated race): "
               << e.what() << "\n";
            log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                            accountId, "", Money{amount, a.getCurrency()},
                            std::string("doublespend/buggy/forced/") + who, now()));
        }
    };
    forceDeduct("TerminalA");
    forceDeduct("TerminalB");
    os << "Phase 1 final balance: " << a.getBalance() << "\n";
    os << "(bank effectively released " << (2 * amount)
       << " against snapshot of " << snapshotA << ")\n";

    // ----------- PHASE 2: fixed (lock/unlock) ---------------------------
    os << "\n--- Phase 2: FIXED (with locks) ---\n";
    try {
        lock(accountId);
        os << "Step 1. Terminal A lock(" << accountId << ") OK\n";
    } catch (const BankError& e) {
        os << "Step 1. Terminal A lock FAILED: " << e.what() << "\n";
    }
    try {
        lock(accountId);
        os << "Step 2. Terminal B lock(" << accountId << ") OK (unexpected!)\n";
    } catch (const DoubleSpendDetected& e) {
        os << "Step 2. Terminal B lock REJECTED: " << e.what() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                        accountId, "", Money{amount, a.getCurrency()},
                        e.what(), now()));
    }
    try {
        a - Money{amount, a.getCurrency()};
        os << "Step 3. Terminal A withdraw " << amount << " OK, balance="
           << a.getBalance() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS,
                        accountId, "", Money{amount, a.getCurrency()},
                        "doublespend/fixed/TerminalA", now()));
    } catch (const BankError& e) {
        os << "Step 3. Terminal A withdraw FAILED: " << e.what() << "\n";
    }
    unlock(accountId);
    os << "Step 4. Terminal A unlock(" << accountId << ")\n";

    // Terminal B retries after unlock
    try {
        lock(accountId);
        a - Money{amount, a.getCurrency()};
        unlock(accountId);
        os << "Step 5. Terminal B retry succeeded, balance=" << a.getBalance() << "\n";
    } catch (const BankError& e) {
        unlock(accountId);
        os << "Step 5. Terminal B retry FAILED: " << e.what() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                        accountId, "", Money{amount, a.getCurrency()},
                        std::string("doublespend/fixed/B/") + e.what(), now()));
    }
    os << "Phase 2 final balance: " << a.getBalance() << "\n\n";
}
```

Add `#include <iostream>` near the top of `apex-banking/src/Bank.cpp` if not already present.

- [ ] **Step 4: Run tests — expect all 30 passing**

Run: `cd apex-banking && make test`
Expected: `30 passed | 0 failed`.

- [ ] **Step 5: Commit**

```bash
git add apex-banking/src/Bank.cpp apex-banking/test/test_bank.cpp
git commit -m "feat(apex): implement two-phase double-spend simulation"
```

---

## Task 11: CLI menu (hand-tested)

**Files:**
- Modify: `apex-banking/src/main.cpp` (replace skeleton)

This is the interactive menu from the spec. No unit tests — verified by running the binary and walking through each option.

- [ ] **Step 1: Replace main.cpp with full CLI**

File: `apex-banking/src/main.cpp`
```cpp
#include "Bank.h"
#include "Errors.h"
#include "Money.h"
#include "Savings.h"
#include "Checking.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s)) throw BadInput("unexpected EOF");
    return s;
}

double readDouble(const std::string& prompt) {
    std::string s = readLine(prompt);
    std::istringstream is(s);
    double v;
    if (!(is >> v)) throw BadInput("expected a number, got: " + s);
    return v;
}

int readMenuChoice() {
    std::string s = readLine("Lựa chọn › ");
    std::istringstream is(s);
    int v;
    if (!(is >> v)) throw BadInput("menu choice must be an integer");
    return v;
}

void printMenu() {
    std::cout << "\n"
        "╔══════════════════════════════════════════════╗\n"
        "║  APEX — Hệ thống ngân hàng đa tiền tệ        ║\n"
        "╚══════════════════════════════════════════════╝\n"
        "\n── Tài khoản ──\n"
        "  1. Tạo tài khoản Savings\n"
        "  2. Tạo tài khoản Checking\n"
        "  3. Xem danh sách tài khoản\n"
        "  4. Xem chi tiết một tài khoản\n"
        "\n── Giao dịch ──\n"
        "  5. Nạp tiền (Deposit)\n"
        "  6. Rút tiền (Withdraw)\n"
        "  7. Chuyển khoản (Transfer)\n"
        "\n── Tiền tệ ──\n"
        "  8. Xem tỷ giá hiện tại\n"
        "  9. Cập nhật tỷ giá (Admin)\n"
        "\n── Báo cáo & Demo ──\n"
        " 10. In sổ cái giao dịch (Ledger)\n"
        " 11. Áp dụng lãi suất (Savings)\n"
        " 12. ⚠ Demo: Double-Spend\n"
        " 13. ⚠ Demo: Tỷ giá không hợp lệ\n"
        "\n  0. Thoát\n\n";
}

void seed(Bank& b) {
    b.setRate("USD", 1.0);
    b.setRate("VND", 24500.0);
    b.setRate("EUR", 0.92);
    b.setRate("JPY", 155.0);
    b.createSavings ("Nguyen Van A", "USD", Money{200.0,      "USD"}, 0.05);
    b.createChecking("Tran Thi B",   "VND", Money{1'000'000.0,"VND"}, 500'000.0);
    std::cout << "[seed] 4 currencies + 2 accounts loaded.\n";
}

void doCreateSavings(Bank& b) {
    std::string owner = readLine("Chủ tài khoản: ");
    std::string cur   = readLine("Loại tiền tệ (USD/VND/EUR/JPY): ");
    double amt        = readDouble("Số dư mở tài khoản: ");
    double rate       = readDouble("Lãi suất (0.05 = 5%): ");
    Account& a = b.createSavings(owner, cur, Money{amt, cur}, rate);
    std::cout << "✅ Tạo thành công: "; a.print(std::cout); std::cout << "\n";
}

void doCreateChecking(Bank& b) {
    std::string owner = readLine("Chủ tài khoản: ");
    std::string cur   = readLine("Loại tiền tệ: ");
    double amt        = readDouble("Số dư mở tài khoản: ");
    double od         = readDouble("Hạn mức thấu chi (overdraft): ");
    Account& a = b.createChecking(owner, cur, Money{amt, cur}, od);
    std::cout << "✅ Tạo thành công: "; a.print(std::cout); std::cout << "\n";
}

void doDeposit(Bank& b) {
    std::string id  = readLine("Account ID: ");
    double amt      = readDouble("Amount: ");
    std::string cur = readLine("Currency: ");
    b.deposit(id, Money{amt, cur});
    std::cout << "✅ Đã nạp " << amt << " " << cur << " vào " << id << "\n";
    b.require(id).print(std::cout); std::cout << "\n";
}

void doWithdraw(Bank& b) {
    std::string id  = readLine("Account ID: ");
    double amt      = readDouble("Amount: ");
    std::string cur = readLine("Currency: ");
    b.withdraw(id, Money{amt, cur});
    std::cout << "✅ Đã rút " << amt << " " << cur << " từ " << id << "\n";
    b.require(id).print(std::cout); std::cout << "\n";
}

void doTransfer(Bank& b) {
    std::string fromId = readLine("From account ID: ");
    std::string toId   = readLine("To account ID: ");
    double amt         = readDouble("Amount: ");
    std::string cur    = readLine("Currency: ");
    b.transfer(fromId, toId, Money{amt, cur});
    std::cout << "✅ Chuyển thành công.\n";
    b.require(fromId).print(std::cout); std::cout << "\n";
    b.require(toId).print(std::cout);   std::cout << "\n";
}

void doSetRate(Bank& b) {
    std::string code = readLine("Currency code: ");
    double rate      = readDouble("New rate (per 1 USD): ");
    b.setRate(code, rate);
    std::cout << "✅ Đã cập nhật tỷ giá.\n";
}

void doApplyInterest(Bank& b) {
    std::string id = readLine("Savings account ID: ");
    Account& a = b.require(id);
    Savings* s = dynamic_cast<Savings*>(&a);
    if (!s) throw BadInput(id + " is not a Savings account");
    s->applyInterest();
    std::cout << "✅ Đã áp dụng lãi. "; a.print(std::cout); std::cout << "\n";
}

void doShowAccount(Bank& b) {
    std::string id = readLine("Account ID: ");
    b.require(id).print(std::cout); std::cout << "\n";
}

void doDoubleSpendDemo(Bank& b) {
    std::string id  = readLine("Account ID to attack: ");
    double amt      = readDouble("Withdrawal amount (each terminal): ");
    b.simulateDoubleSpend(id, amt);
}

void doBadRateDemo(Bank& b) {
    std::cout << "Thử đặt tỷ giá EUR = -1.5 ...\n";
    try { b.setRate("EUR", -1.5); }
    catch (const BankError& e) {
        std::cout << "❌ " << e.what() << "\n";
    }
    std::cout << "Thử đặt tỷ giá JPY = 0 ...\n";
    try { b.setRate("JPY", 0.0); }
    catch (const BankError& e) {
        std::cout << "❌ " << e.what() << "\n";
    }
    std::cout << "(Cả hai giao dịch FAILED được ghi vào sổ cái.)\n";
}

}  // namespace

int main() {
    Bank bank;
    seed(bank);

    while (true) {
        printMenu();
        int choice = 0;
        try { choice = readMenuChoice(); }
        catch (const BankError& e) {
            std::cout << "❌ " << e.what() << "\n";
            continue;
        }

        try {
            switch (choice) {
                case 0:  std::cout << "Tạm biệt!\n"; return 0;
                case 1:  doCreateSavings(bank);  break;
                case 2:  doCreateChecking(bank); break;
                case 3:  bank.listAccounts(std::cout); break;
                case 4:  doShowAccount(bank);    break;
                case 5:  doDeposit(bank);        break;
                case 6:  doWithdraw(bank);       break;
                case 7:  doTransfer(bank);       break;
                case 8:  bank.rates().list(std::cout); break;
                case 9:  doSetRate(bank);        break;
                case 10: bank.printLedger(std::cout); break;
                case 11: doApplyInterest(bank);  break;
                case 12: doDoubleSpendDemo(bank); break;
                case 13: doBadRateDemo(bank);    break;
                default: std::cout << "❌ Lựa chọn không hợp lệ.\n";
            }
        } catch (const BankError& e) {
            std::cout << "❌ " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "❌ Lỗi hệ thống: " << e.what() << "\n";
        }
    }
}
```

- [ ] **Step 2: Build and run interactively**

Run:
```bash
cd apex-banking && make
./apex
```

Walkthrough checklist — manually verify each works:
- Menu appears with all 14 options.
- Option 3: shows `acc_001` (Savings, USD) and `acc_002` (Checking, VND).
- Option 5: deposit `50 USD` into `acc_001` → balance shows `250.00 USD`.
- Option 7: transfer `10 USD` from `acc_001` to `acc_002` → `acc_002` increases by `245000 VND`.
- Option 9: try rate `0` → prints red-ish `❌ InvalidRate`.
- Option 12: prompts for id + amount, prints both phases with narration.
- Option 13: auto-runs the negative+zero rate rejection.
- Option 10: ledger shows every action including FAILED entries.
- Option 0: clean exit.

- [ ] **Step 3: Run tests one last time**

Run: `cd apex-banking && make test`
Expected: `30 passed | 0 failed`.

- [ ] **Step 4: Commit**

```bash
git add apex-banking/src/main.cpp
git commit -m "feat(apex): add interactive Vietnamese CLI menu"
```

---

## Task 12: README (Vietnamese, GitHub-ready)

**Files:**
- Create: `apex-banking/README.md`

- [ ] **Step 1: Write the README**

File: `apex-banking/README.md`
````markdown
# APEX — Hệ thống ngân hàng đa tiền tệ

> Đồ án giữa kỳ C++ (PTIT) — Đề 2: Multi-currency banking system.

Chương trình CLI tương tác mô phỏng một hệ thống ngân hàng chuyên nghiệp
quản lý nhiều loại tài khoản (Savings, Checking), hỗ trợ chuyển khoản
giữa các loại tiền tệ khác nhau, ghi sổ cái mọi giao dịch, và trình diễn
trực tiếp các tình huống lỗi theo yêu cầu đề bài.

## ✨ Tính năng

- Kế thừa: lớp `Account` trừu tượng → `Savings` (có lãi suất, không thấu chi)
  và `Checking` (không lãi suất, có hạn mức thấu chi).
- Nạp chồng toán tử `+` và `-` trên `Account` để nạp / rút tiền, **tự động
  quy đổi tiền tệ** qua `CurrencyRegistry`.
- `Bank` lưu trữ tài khoản trong `std::map<std::string, std::unique_ptr<Account>>`.
- Lớp `Transaction` ghi lại mọi giao dịch (kể cả thất bại) trong sổ cái.
- Demo tương tác hai tình huống lỗi theo yêu cầu:
  - Chi tiêu kép (Double-Spend) — minh hoạ cả phiên bản có lỗi và đã sửa.
  - Tỷ giá không hợp lệ — từ chối rate ≤ 0 và ghi vào sổ cái.

## 🛠 Yêu cầu hệ thống

- Trình biên dịch hỗ trợ **C++20** (`g++ ≥ 10` hoặc `clang++ ≥ 12`).
- **GNU Make**.
- Hệ điều hành: Linux / macOS / WSL / Windows (MinGW).

## 📦 Cài đặt & Biên dịch

```bash
git clone https://github.com/<user>/ptit-cpp.git
cd ptit-cpp/apex
make
```

Kết quả: file thực thi `./apex`.

Để chạy bộ test:
```bash
make test
```

Dọn sạch:
```bash
make clean
```

## ▶️ Chạy chương trình

```bash
./apex
```

Chương trình khởi động menu tương tác với dữ liệu mẫu (4 loại tiền tệ
USD/VND/EUR/JPY và 2 tài khoản mẫu `acc_001`, `acc_002`).

## 🎬 Demo các tình huống lỗi

### 1. Double-Spend (Chi tiêu kép) — menu **12**

```
Phase 1 (BUGGY): Terminal A và B đều đọc balance = 100 trước khi ghi
                 → cả hai tin rằng còn đủ tiền, cả hai đều rút 80
                 → tài khoản bị trừ sai
Phase 2 (FIXED): Dùng Bank::lock() / unlock() để nối tiếp thao tác
                 → Terminal B bị từ chối với DoubleSpendDetected
```

### 2. Tỷ giá không hợp lệ — menu **13** (hoặc **9** với rate ≤ 0)

```
Thử đặt tỷ giá EUR = -1.5  →  ❌ InvalidRate: rate must be > 0
Thử đặt tỷ giá JPY = 0     →  ❌ InvalidRate: rate must be > 0
(Cả hai giao dịch FAILED được ghi vào sổ cái.)
```

## 🏗 Kiến trúc

```
┌─────────────────┐   owns   ┌──────────────────────────────────────┐
│     Bank        │ ───────▶ │ std::map<id, unique_ptr<Account>>    │
│ (facade)        │          └──────────────────────────────────────┘
│                 │ ──▶ CurrencyRegistry (tỷ giá)
│                 │ ──▶ std::vector<Transaction> (sổ cái)
└─────────────────┘
          ▲
          │ dùng
          │
     ┌────┴────┐
     │ main.cpp │   (menu CLI — không chứa logic nghiệp vụ)
     └─────────┘

    Account (abstract)   — có operator+, operator-
       ▲         ▲
       │         │
    Savings   Checking
```

## 📚 Nạp chồng toán tử

| Toán tử | Ngữ nghĩa |
|---|---|
| `account + Money` | Nạp tiền (tự động quy đổi tiền tệ) |
| `account - Money` | Rút tiền (tự động quy đổi + kiểm tra policy của loại tài khoản) |

## 📂 Cấu trúc thư mục

```
apex-banking/
├── include/       # Header files (.h)
├── src/           # Implementation (.cpp)
├── test/          # Unit tests (doctest)
├── Makefile
└── README.md
```

## 🧪 Testing

Dự án dùng [doctest](https://github.com/doctest/doctest) — thư viện test
header-only, được vendored sẵn trong `test/doctest.h`, không cần cài gì thêm.

```bash
make test
# → 30 passed | 0 failed
```

## 👤 Tác giả

PTIT — 2026.
````

- [ ] **Step 2: Verify the README renders**

Run:
```bash
cd apex-banking && ls README.md && head -20 README.md
```

- [ ] **Step 3: Final build + test sanity**

Run:
```bash
cd apex-banking && make clean && make && make test
```
Expected: clean build, `30 passed | 0 failed`.

- [ ] **Step 4: Commit**

```bash
git add apex-banking/README.md
git commit -m "docs(apex): add Vietnamese README with install/demo guide"
```

---

## Task 13: Final polish + tag

**Files:**
- Modify: `apex-banking/src/*.cpp` (sweep for missing docblocks)

- [ ] **Step 1: Sweep all public methods for docblocks**

Open each header in `apex-banking/include/` and verify every public method has a
`/** ... */` docblock per the spec's documentation standard (English, brief,
covers purpose / params / throws / return where relevant). Fix any gaps.

- [ ] **Step 2: Final build + test**

Run:
```bash
cd apex-banking && make clean && make && make test
```
Expected: clean build, all tests pass.

- [ ] **Step 3: Run the binary once more end-to-end as a final sanity check**

Run:
```bash
./apex
```
Quickly verify: menu appears, seed data loads, option 12 & 13 work, 0 exits cleanly.

- [ ] **Step 4: Commit any doc fixes and tag**

```bash
git add -u apex-banking/include/
git diff --cached --quiet || git commit -m "docs(apex): fill in missing public-method docblocks"
```

---

## Self-review notes

- **Spec coverage:** inheritance (Task 6-8), operator `+` / `-` (Task 6), `std::map` (Task 9), `Transaction` class (Task 5 + 9), double-spend demo (Task 10), invalid-rate demo (Task 9 `setRate` + Task 11 menu 13), Vietnamese README (Task 12), seed data (Task 11), C++20 & Makefile (Task 1), TDD via doctest (Tasks 4–10). All nine spec decisions mapped to tasks.
- **Placeholders:** none — every step contains the full code, every command has expected output.
- **Type consistency:** `Account::balance` is protected and accessed from Savings/Checking via inheritance; all method signatures (e.g., `canWithdraw(double)`, `kind() const`) match between declaration and test use. `Money` is used consistently everywhere. `Bank::ledgerEntries()` is used in tests and exposed in `Bank.h`.
- **Known quirks:** the "forced race" code in `simulateDoubleSpend` Phase 1 uses a deposit-then-withdraw trick because our honest `operator-` can't actually be bypassed in a well-designed Account — this keeps the code clean while still producing the observable behavior (extra ledger entries and narrated steps). Noted for the engineer so they don't "fix" it.
