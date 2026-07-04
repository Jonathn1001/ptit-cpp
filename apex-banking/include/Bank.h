#pragma once

#include "Account.h"
#include "Currency.h"
#include "Money.h"
#include "Transaction.h"

#include <ctime>
#include <iosfwd>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <vector>

/**
 * Central facade. Owns all accounts, the currency registry, and the ledger.
 * The CLI and Web UI talk to Bank and nothing else.
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
    Account& createSavings(
        const std::string& owner,
        const std::string& currency,
        const Money& opening,
        double interestRate
    );

    Account& createChecking(
        const std::string& owner,
        const std::string& currency,
        const Money& opening,
        double overdraftLimit
    );

    Account* find(const std::string& id);
    Account& require(const std::string& id);

    // --- money movement ---
    void deposit(const std::string& id, const Money& m);
    void withdraw(const std::string& id, const Money& m);
    void transfer(const std::string& fromId, const std::string& toId, const Money& m);

    // --- currency admin ---
    void setRate(const std::string& code, double rate);
    const CurrencyRegistry& rates() const { return registry; }

    // --- queries ---
    void listAccounts(std::ostream& os) const;
    void printLedger(std::ostream& os) const;
    const std::vector<Transaction>& ledgerEntries() const { return ledger; }

    // Web/dashboard helper: expose read-only account pointers without giving ownership.
    std::vector<const Account*> allAccounts() const;

    // --- double-spend locking primitives ---
    void lock(const std::string& id);
    void unlock(const std::string& id);

    // --- demos ---
    void simulateDoubleSpend(const std::string& accountId, double amount);
};
