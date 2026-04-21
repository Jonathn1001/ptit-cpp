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
