#include "Checking.h"
#include "Errors.h"
#include <iomanip>
#include <ostream>
#include <utility>
#include <cmath>

Checking::Checking(std::string id, std::string owner, std::string currency,
                   Money opening, const CurrencyRegistry* reg, double overdraftLimit)
    : Account(std::move(id), std::move(owner), std::move(currency),
              std::move(opening), reg),
      overdraftLimit(overdraftLimit) {
    if (!std::isfinite(overdraftLimit) || overdraftLimit < 0.0) {
        throw BadInput("overdraft limit must be finite and >= 0");
    }
}
bool Checking::canWithdraw(double amtNative) const {
    return amtNative <= balance + overdraftLimit;
}

void Checking::print(std::ostream& os) const {
    Account::print(os);
    // Fixed 2-decimal like Money; save/restore so we don't leak format state
    // into whatever prints next (e.g. the next account in a list).
    std::ios::fmtflags f = os.flags();
    std::streamsize    p = os.precision();
    os << "  overdraft=" << std::fixed << std::setprecision(2) << overdraftLimit;
    os.flags(f);
    os.precision(p);
}
