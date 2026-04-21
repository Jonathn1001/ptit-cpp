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
