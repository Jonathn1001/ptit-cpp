#include "Savings.h"
#include "Errors.h"
#include <ostream>
#include <utility>
#include <cmath>

Savings::Savings(std::string id, std::string owner, std::string currency,
                 Money opening, const CurrencyRegistry* reg, double interestRate)
    : Account(std::move(id), std::move(owner), std::move(currency),
              std::move(opening), reg),
      interestRate(interestRate) {
    if (!std::isfinite(interestRate) || interestRate < 0.0) {
        throw BadInput("interest rate must be finite and >= 0");
    }
}

bool Savings::canWithdraw(double amtNative) const {
    return std::isfinite(amtNative) && amtNative <= balance;
}

void Savings::applyInterest() {
    double nextBalance = balance * (1.0 + interestRate);

    if (!std::isfinite(nextBalance)) {
        throw BadInput("interest result must be finite");
    }

    balance = nextBalance;
}

void Savings::print(std::ostream& os) const {
    Account::print(os);
    os << "  interest=" << (interestRate * 100.0) << "%";
}
