#include "Currency.h"
#include "Errors.h"
#include <ostream>
#include <string>

void CurrencyRegistry::setRate(const std::string& code, double rate) {
    if (rate <= 0.0) {
        throw InvalidRate("rate must be > 0 (got " + std::to_string(rate) + ")");
    }
    rates[code] = rate;
}

double CurrencyRegistry::getRate(const std::string& code) const {
    auto it = rates.find(code);
    if (it == rates.end()) {
        throw CurrencyUnknown("unknown currency: " + code);
    }
    return it->second;
}

Money CurrencyRegistry::convert(const Money& from, const std::string& toCurrency) const {
    double fromRate = getRate(from.currency);
    double toRate   = getRate(toCurrency);
    double amountInBase = from.amount / fromRate;
    return Money{amountInBase * toRate, toCurrency};
}

bool CurrencyRegistry::has(const std::string& code) const {
    return rates.count(code) > 0;
}

void CurrencyRegistry::list(std::ostream& os) const {
    for (const auto& [code, rate] : rates) {
        os << "  " << code << " : " << rate << "\n";
    }
}
