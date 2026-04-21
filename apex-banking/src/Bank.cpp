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
        // Successful rate changes are not journaled to keep the ledger
        // focused on account-level events; only failures are recorded.
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
