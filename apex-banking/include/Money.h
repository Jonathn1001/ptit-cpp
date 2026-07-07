#pragma once
#include <iosfwd>
#include <string>

/**
 * A currency-tagged monetary amount.
 * `amount` is in units of `currency` (e.g., 100 USD, 2'450'000 VND).
 * Not a class — intentionally a plain aggregate. Arithmetic happens inside
 * Account operators, not on Money itself.
 */
struct Money {
    double amount;
    std::string currency;
};

/** Writes "amount CURRENCY" (e.g. "100.00 USD") to `os`. */
std::ostream& operator<<(std::ostream& os, const Money& m);
