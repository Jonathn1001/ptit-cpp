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
    /** Creates a new Savings account and records a CREATE_ACCOUNT transaction. */
    Account& createSavings (const std::string& owner, const std::string& currency,
                            const Money& opening, double interestRate);
    /** Creates a new Checking account and records a CREATE_ACCOUNT transaction. */
    Account& createChecking(const std::string& owner, const std::string& currency,
                            const Money& opening, double overdraftLimit);
    /** @return Pointer to the account, or nullptr if not found. */
    Account* find(const std::string& id);
    /**
     * Like find(), but throws on missing account.
     * @throws AccountNotFound if `id` does not exist.
     */
    Account& require(const std::string& id);

    // --- money movement ---
    /** Deposits `m` into account `id`, recording a DEPOSIT transaction. */
    void deposit (const std::string& id, const Money& m);
    /** Withdraws `m` from account `id`, recording a WITHDRAW transaction. */
    void withdraw(const std::string& id, const Money& m);
    /** Transfers `m` from `fromId` to `toId`, recording a TRANSFER transaction. */
    void transfer(const std::string& fromId, const std::string& toId, const Money& m);

    /**
     * Applies one interest period to Savings account `id`; records an
     * APPLY_INTEREST transaction (SUCCESS or FAILED).
     * @throws BadInput if `id` is not a Savings account.
     * @throws AccountNotFound if `id` does not exist.
     */
    void applyInterest(const std::string& id);

    // --- currency admin ---
    /** Registers or updates a currency rate; records a SET_RATE transaction. */
    void setRate(const std::string& code, double rate);
    const CurrencyRegistry& rates() const { return registry; }

    // --- queries ---
    /** Prints a summary line for every account to `os`. */
    void listAccounts(std::ostream&) const;
    /** Prints every ledger entry to `os`. */
    void printLedger(std::ostream&) const;
    const std::vector<Transaction>& ledgerEntries() const { return ledger; }

    // --- double-spend locking primitives ---
    /** Marks account `id` as locked; locked accounts cannot be debited. */
    void lock(const std::string& id);
    /** Releases the lock on account `id`. */
    void unlock(const std::string& id);

    // --- demos ---
    /** Demonstrates double-spend prevention by attempting two concurrent withdrawals. */
    void simulateDoubleSpend(const std::string& accountId, double amount);
};
