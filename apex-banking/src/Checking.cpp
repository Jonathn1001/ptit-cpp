#include "Checking.h"
#include <ostream>
#include <utility>
#include <iomanip>

Checking::Checking(std::string id, std::string owner, std::string currency,
                   Money opening, const CurrencyRegistry* reg, double overdraftLimit)
    : Account(std::move(id), std::move(owner), std::move(currency),
              std::move(opening), reg),
      overdraftLimit(overdraftLimit) {}

bool Checking::canWithdraw(double amtNative) const {
    return amtNative <= balance + overdraftLimit;
}

void Checking::print(std::ostream& os) const {
    Account::print(os);
    os << "  overdraft="<< std::fixed << std::setprecision(0) << overdraftLimit;
}
