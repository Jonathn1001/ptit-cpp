#include "doctest.h"
#include "Currency.h"
#include "Errors.h"

TEST_CASE("setRate rejects zero and negative rates") {
    CurrencyRegistry reg;
    CHECK_THROWS_AS(reg.setRate("EUR", 0.0), InvalidRate);
    CHECK_THROWS_AS(reg.setRate("EUR", -1.5), InvalidRate);
    CHECK_FALSE(reg.has("EUR"));
}

TEST_CASE("setRate accepts positive rates and getRate echoes them back") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    reg.setRate("VND", 24500.0);
    CHECK(reg.has("USD"));
    CHECK(reg.getRate("USD") == doctest::Approx(1.0));
    CHECK(reg.getRate("VND") == doctest::Approx(24500.0));
}

TEST_CASE("getRate throws CurrencyUnknown for missing currency") {
    CurrencyRegistry reg;
    CHECK_THROWS_AS(reg.getRate("XXX"), CurrencyUnknown);
}

TEST_CASE("convert: same currency is identity") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    Money m{100.0, "USD"};
    Money out = reg.convert(m, "USD");
    CHECK(out.amount == doctest::Approx(100.0));
    CHECK(out.currency == "USD");
}

TEST_CASE("convert: USD to VND uses rates correctly") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    reg.setRate("VND", 24500.0);
    Money out = reg.convert(Money{10.0, "USD"}, "VND");
    CHECK(out.amount == doctest::Approx(245000.0));
    CHECK(out.currency == "VND");
}

TEST_CASE("convert: VND to USD inverse direction") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    reg.setRate("VND", 24500.0);
    Money out = reg.convert(Money{245000.0, "VND"}, "USD");
    CHECK(out.amount == doctest::Approx(10.0));
    CHECK(out.currency == "USD");
}

TEST_CASE("convert: unknown source or target throws CurrencyUnknown") {
    CurrencyRegistry reg;
    reg.setRate("USD", 1.0);
    CHECK_THROWS_AS(reg.convert(Money{1.0, "XXX"}, "USD"), CurrencyUnknown);
    CHECK_THROWS_AS(reg.convert(Money{1.0, "USD"}, "XXX"), CurrencyUnknown);
}
