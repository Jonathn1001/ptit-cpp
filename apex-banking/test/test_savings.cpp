#include "doctest.h"
#include "Savings.h"
#include "Currency.h"
#include "Errors.h"

namespace {
CurrencyRegistry usdOnly() {
    CurrencyRegistry r;
    r.setRate("USD", 1.0);
    return r;
}
}

TEST_CASE("Savings.kind() returns 'Savings'") {
    auto reg = usdOnly();
    Savings s("s1", "A", "USD", Money{100.0, "USD"}, &reg, 0.05);
    CHECK(s.kind() == "Savings");
}

TEST_CASE("Savings rejects withdrawal beyond balance (no overdraft)") {
    auto reg = usdOnly();
    Savings s("s1", "A", "USD", Money{100.0, "USD"}, &reg, 0.05);
    CHECK(s.canWithdraw(100.0));
    CHECK(s.canWithdraw(99.99));
    CHECK_FALSE(s.canWithdraw(100.01));
    Money bigWithdraw{101.0, "USD"};
    CHECK_THROWS_AS(s - bigWithdraw, InsufficientFunds);
}

TEST_CASE("Savings applyInterest multiplies balance by (1 + rate)") {
    auto reg = usdOnly();
    Savings s("s1", "A", "USD", Money{100.0, "USD"}, &reg, 0.05);
    s.applyInterest();
    CHECK(s.getBalance() == doctest::Approx(105.0));
    s.applyInterest();
    CHECK(s.getBalance() == doctest::Approx(110.25));
}
