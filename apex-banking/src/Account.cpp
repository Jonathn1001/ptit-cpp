#include "Account.h"
#include "Currency.h"
#include "Errors.h"
#include <ostream>
#include <utility>
#include <cmath>

Account::Account(std::string id, std::string owner, std::string currency,
                 Money opening, const CurrencyRegistry* reg)
    : id(std::move(id)),
      owner(std::move(owner)),
      nativeCurrency(std::move(currency)),
      balance(0.0),
      reg(reg) {
     if (!std::isfinite(opening.amount)) {
        throw BadInput("opening balance must be finite");
    }

    if (opening.amount < 0.0) {
        throw BadInput("opening balance must be >= 0");
    }

    if (!reg->has(nativeCurrency)) {
        throw CurrencyUnknown("unknown currency: " + nativeCurrency);
    }

    if (opening.currency == nativeCurrency) {
        balance = opening.amount;
    } else {
        balance = reg->convert(opening, nativeCurrency).amount;
    }
 }

Account& Account::operator+(const Money& m) {
    if (!std::isfinite(m.amount)) {
        throw BadInput("deposit amount must be finite");
    }

    if (m.amount <= 0.0) {
        throw BadInput("deposit amount must be > 0");
    }

    Money converted = (m.currency == nativeCurrency)
        ? m
        : reg->convert(m, nativeCurrency);

    balance += converted.amount;
    return *this;
}

Account& Account::operator-(const Money& m) {
    if (!std::isfinite(m.amount)) {
        throw BadInput("withdraw amount must be finite");
    }

    if (m.amount <= 0.0) {
        throw BadInput("withdraw amount must be > 0");
    }

    Money converted = (m.currency == nativeCurrency)
        ? m
        : reg->convert(m, nativeCurrency);

    if (!canWithdraw(converted.amount)) {
        throw InsufficientFunds("insufficient funds in " + id);
    }

    balance -= converted.amount;
    return *this;
}

void Account::print(std::ostream& os) const {
    os << "[" << kind() << "] " << id
       << "  owner=" << owner
       << "  balance=" << Money{balance, nativeCurrency};
}
