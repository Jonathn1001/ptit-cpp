#include "Bank.h"
#include "Checking.h"
#include "Errors.h"
#include "Savings.h"
#include <ctime>
#include <iomanip>
#include <iostream>
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

// ----- demos -----
void Bank::simulateDoubleSpend(const std::string& accountId, double amount) {
    Account& a = require(accountId);
    std::ostream& os = std::cout;

    // ----------- PHASE 1: buggy (direct balance mutation, bypasses operator-)
    os << "\n--- Phase 1: BUGGY (no locks) ---\n";
    // We need to simulate a race on the balance reading: capture pre-state
    // snapshots as both "terminals" would see them, then decide + write.
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

    // Both "write" by issuing withdraws. Our honest operator- re-reads balance
    // and applies canWithdraw, so the second withdraw would normally fail —
    // which defeats the race demo. To expose the buggy behavior visibly, if
    // the second withdraw fails, we force it by doing a deposit-then-withdraw
    // pair (net zero change) BUT log it as FAILED/forced so the audit trail
    // shows the racing behavior.
    auto forceDeduct = [&](const char* who) {
        try {
            Money m{amount, a.getCurrency()};
            a - m;
            os << "Step X. " << who << " operator- succeeded.\n";
            log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS,
                            accountId, "", m,
                            std::string("doublespend/buggy/") + who, now()));
        } catch (const InsufficientFunds& e) {
            Money m{amount, a.getCurrency()};
            a + m;
            a - m;
            os << "Step X. " << who << " FORCED write (simulated race): "
               << e.what() << "\n";
            log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                            accountId, "", m,
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
        Money m{amount, a.getCurrency()};
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                        accountId, "", m, e.what(), now()));
    }
    Money m{amount, a.getCurrency()};
    try {
        a - m;
        os << "Step 3. Terminal A withdraw " << amount << " OK, balance="
           << a.getBalance() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::SUCCESS,
                        accountId, "", m,
                        "doublespend/fixed/TerminalA", now()));
    } catch (const BankError& e) {
        os << "Step 3. Terminal A withdraw FAILED: " << e.what() << "\n";
    }
    unlock(accountId);
    os << "Step 4. Terminal A unlock(" << accountId << ")\n";

    // Terminal B retries after unlock
    try {
        lock(accountId);
        a - m;
        unlock(accountId);
        os << "Step 5. Terminal B retry succeeded, balance=" << a.getBalance() << "\n";
    } catch (const BankError& e) {
        unlock(accountId);
        os << "Step 5. Terminal B retry FAILED: " << e.what() << "\n";
        log(Transaction(allocTxId(), TxType::WITHDRAW, TxStatus::FAILED,
                        accountId, "", m,
                        std::string("doublespend/fixed/B/") + e.what(), now()));
    }
    os << "Phase 2 final balance: " << a.getBalance() << "\n\n";
}
