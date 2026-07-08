#include "Bank.h"
#include "Checking.h"
#include "Errors.h"
#include "Savings.h"
#include <ctime>
#include <iomanip>
#include <iostream>
#include <mutex>
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
void Bank::applyInterest(const std::string& id) {
    try {
        Account& a = require(id);
        Savings* s = dynamic_cast<Savings*>(&a);

        if (!s) {
            throw BadInput(id + " is not a Savings account");
        }

        double before = s->getBalance();

        s->applyInterest();

        double interestGenerated = s->getBalance() - before;

        log(Transaction(
            allocTxId(),
            TxType::APPLY_INTEREST,
            TxStatus::SUCCESS,
            "",
            id,
            Money{interestGenerated, s->getCurrency()},
            "interest generated",
            now()
        ));
    } catch (const BankError& e) {
        log(Transaction(
            allocTxId(),
            TxType::APPLY_INTEREST,
            TxStatus::FAILED,
            "",
            id,
            Money{},
            e.what(),
            now()
        ));

        throw;
    }
}
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
    std::lock_guard<std::mutex> guard(bankMutex);

    try {
        Account* p = find(id);
        if (!p) {
            throw AccountNotFound("no such account: " + id);
        }

        Account& a = *p;
        a - m;

        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS, id, "", m, "", now()));
    } catch (const BankError& e) {
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED, id, "", m, e.what(), now()));
        throw;
    }
}

void Bank::transfer(const std::string& fromId, const std::string& toId, const Money& m) {
    std::lock_guard<std::mutex> guard(bankMutex);

    try {
        Account* fromPtr = find(fromId);
        Account* toPtr = find(toId);

        if (!fromPtr) {
            throw AccountNotFound("no such account: " + fromId);
        }

        if (!toPtr) {
            throw AccountNotFound("no such account: " + toId);
        }

        Account& from = *fromPtr;
        Account& to = *toPtr;

        from - m;

        try {
            to + m;
        } catch (...) {
            from + m;
            throw;
        }

        log(Transaction(allocTxId(), TxType::TRANSFER, TxStatus::SUCCESS, fromId, toId, m, "", now()));
    } catch (const BankError& e) {
        log(Transaction(allocTxId(), TxType::TRANSFER, TxStatus::FAILED, fromId, toId, m, e.what(), now()));
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

// ----- demos -----
void Bank::simulateDoubleSpend(const std::string& accountId, double amount) {
    Account& a = require(accountId);
    std::ostream& os = std::cout;
    const double startBalance = a.getBalance();

    // ----------- PHASE 1: narrate the race (NO state mutation) ----------
    // A naive implementation would read the balance, check, then write —
    // without serializing those three steps, two concurrent writers both see
    // the original balance and both succeed, overspending the account.
    // We narrate what that would look like without actually corrupting state,
    // so Phase 2 can demonstrate the fix against an untouched account.
    os << "\n--- Phase 1: BUGGY (no locks) — narrated, no state changes ---\n";
    os << "Starting balance: " << startBalance << " " << a.getCurrency() << "\n";
    double snapshotA = startBalance;
    double snapshotB = startBalance;
    os << "Step 1. Terminal A reads balance -> " << snapshotA << "\n";
    os << "Step 2. Terminal B reads balance -> " << snapshotB
       << "   (before A writes — the race begins)\n";
    bool okA = amount <= snapshotA;
    bool okB = amount <= snapshotB;
    os << "Step 3. Terminal A checks " << amount << " <= " << snapshotA
       << " ? " << (okA ? "YES" : "NO") << "\n";
    os << "Step 4. Terminal B checks " << amount << " <= " << snapshotB
       << " ? " << (okB ? "YES" : "NO") << "\n";
    if (okA && okB) {
        double projA = snapshotA - amount;
        double projB = snapshotB - amount;
        os << "Step 5. If both write based on their own snapshot:\n";
        os << "        Terminal A writes new balance = " << projA << "\n";
        os << "        Terminal B writes new balance = " << projB
           << "   (expected " << (startBalance - 2 * amount) << " — one write is lost!)\n";
        os << "Phase 1 conclusion: bank would release " << (2 * amount)
           << " but only one withdrawal is recorded. The naive design is unsafe.\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                        accountId, "", Money{amount, a.getCurrency()},
                        "doublespend/buggy/race-projected", now()));
    } else {
        os << "Step 5. At least one terminal legitimately can't afford "
           << amount << "; no race to demonstrate at this amount.\n";
    }
    os << "Balance after Phase 1 (unchanged): " << a.getBalance() << "\n";

    // ----------- PHASE 2: the fix (real lock/unlock) --------------------
    os << "\n--- Phase 2: FIXED (with locks) ---\n";
    os << "Starting balance: " << a.getBalance() << " " << a.getCurrency() << "\n";
    Money m{amount, a.getCurrency()};

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
                        accountId, "", m, e.what(), now()));
    }
    try {
        a - m;
        os << "Step 3. Terminal A withdraw " << amount << " OK, balance="
           << a.getBalance() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS,
                        accountId, "", m, "doublespend/fixed/TerminalA", now()));
    } catch (const BankError& e) {
        os << "Step 3. Terminal A withdraw FAILED: " << e.what() << "\n";
    }
    unlock(accountId);
    os << "Step 4. Terminal A unlock(" << accountId << ")\n";

    // Terminal B retries after unlock — should succeed IF balance still covers amount
    try {
        lock(accountId);
        a - m;
        unlock(accountId);
        os << "Step 5. Terminal B retry succeeded, balance=" << a.getBalance() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS,
                        accountId, "", m, "doublespend/fixed/TerminalB", now()));
    } catch (const BankError& e) {
        unlock(accountId);
        os << "Step 5. Terminal B retry FAILED: " << e.what() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                        accountId, "", m,
                        std::string("doublespend/fixed/B/") + e.what(), now()));
    }
    os << "Phase 2 final balance: " << a.getBalance() << "\n\n";
}
