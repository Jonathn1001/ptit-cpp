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
