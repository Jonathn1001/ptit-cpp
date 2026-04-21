#pragma once
#include "Account.h"

/**
 * Savings account: earns interest via applyInterest(), never permits overdraft.
 */
class Savings : public Account {
    double interestRate;  // annual rate, e.g. 0.05 for 5%
public:
    /** @param interestRate Annual rate as a decimal fraction (e.g. 0.05 for 5%). */
    Savings(std::string id, std::string owner, std::string currency,
            Money opening, const CurrencyRegistry* reg, double interestRate);

    std::string kind() const override { return "Savings"; }
    bool canWithdraw(double amtNative) const override;

    /** Apply one interest period: balance *= (1 + interestRate). */
    void applyInterest();

    double getInterestRate() const { return interestRate; }
    void print(std::ostream& os) const override;
};
