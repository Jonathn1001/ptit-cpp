#pragma once
#include "Money.h"
#include <iosfwd>
#include <map>
#include <string>

/**
 * Stores per-currency exchange rates and converts between them.
 * Rates are expressed as "units of X per 1 base unit" where the base is
 * implicit USD (i.e. rates["USD"] is always 1.0 by convention). The class
 * does not enforce a base — any currency whose rate is 1.0 works as base.
 */
class CurrencyRegistry {
    std::map<std::string, double> rates;
public:
    /**
     * Register or update a currency's rate.
     * @throws InvalidRate if rate <= 0.
     */
    void setRate(const std::string& code, double rate);

    /** @throws CurrencyUnknown if code is not registered. */
    double getRate(const std::string& code) const;

    /** @throws CurrencyUnknown on either side. */
    Money convert(const Money& from, const std::string& toCurrency) const;

    bool has(const std::string& code) const;
    /** Prints all registered currencies and their rates to `os`. */
    void list(std::ostream&) const;
};
