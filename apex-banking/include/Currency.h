#pragma once

#include "Money.h"

#include <iosfwd>
#include <map>
#include <string>

/**
 * Stores per-currency exchange rates and converts between them.
 * Rates are expressed as "units of X per 1 base unit" where the base is
 * implicit USD (i.e. rates["USD"] is always 1.0 by convention).
 */
class CurrencyRegistry {
    std::map<std::string, double> rates;

public:
    void setRate(const std::string& code, double rate);
    double getRate(const std::string& code) const;
    Money convert(const Money& from, const std::string& toCurrency) const;
    bool has(const std::string& code) const;
    void list(std::ostream& os) const;

    // Web/dashboard helper: expose read-only rates for tables and charts.
    const std::map<std::string, double>& allRates() const { return rates; }
};
