#include "doctest.h"
#include "Checking.h"
#include "Currency.h"
#include "Errors.h"

namespace {
CurrencyRegistry usdOnly() {
    CurrencyRegistry r;
    r.setRate("USD", 1.0);
    return r;
}
}

TEST_CASE("Checking.kind() returns 'Checking'") {
    auto reg = usdOnly();
    Checking c("c1", "B", "USD", Money{100.0, "USD"}, &reg, 50.0);
    CHECK(c.kind() == "Checking");
}

TEST_CASE("Checking allows withdrawal into overdraft limit") {
    auto reg = usdOnly();
    Checking c("c1", "B", "USD", Money{100.0, "USD"}, &reg, 50.0);
    CHECK(c.canWithdraw(100.0));
    CHECK(c.canWithdraw(150.0));        // exactly overdraft edge
    CHECK_FALSE(c.canWithdraw(150.01)); // beyond overdraft
    Money withdraw140{140.0, "USD"};
    c - withdraw140;
    CHECK(c.getBalance() == doctest::Approx(-40.0));
}

TEST_CASE("Checking rejects withdrawal beyond overdraft") {
    auto reg = usdOnly();
    Checking c("c1", "B", "USD", Money{100.0, "USD"}, &reg, 50.0);
    Money huge{200.0, "USD"};
    CHECK_THROWS_AS(c - huge, InsufficientFunds);
    CHECK(c.getBalance() == doctest::Approx(100.0));
}
