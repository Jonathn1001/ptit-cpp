#include "doctest.h"
#include "Account.h"
#include "Currency.h"
#include "Errors.h"

namespace {
/** Test-only concrete Account that always permits any withdrawal. */
class PermissiveAccount : public Account {
public:
    PermissiveAccount(std::string id, std::string owner, std::string currency,
                      Money opening, const CurrencyRegistry* reg)
        : Account(std::move(id), std::move(owner), std::move(currency),
                  std::move(opening), reg) {}
    std::string kind() const override { return "Permissive"; }
    bool canWithdraw(double /*amt*/) const override { return true; }
};

CurrencyRegistry makeRegistry() {
    CurrencyRegistry r;
    r.setRate("USD", 1.0);
    r.setRate("VND", 24500.0);
    return r;
}
}

TEST_CASE("operator+ deposits same-currency money") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    a + Money{50.0, "USD"};
    CHECK(a.getBalance() == doctest::Approx(150.0));
}

TEST_CASE("operator+ converts foreign currency to native") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    a + Money{24500.0, "VND"};  // == 1 USD
    CHECK(a.getBalance() == doctest::Approx(101.0));
}

TEST_CASE("operator+ on negative amount throws BadInput") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    Money neg{-1.0, "USD"};
    CHECK_THROWS_AS(a + neg, BadInput);
}

TEST_CASE("operator- withdraws same-currency money when canWithdraw allows") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    a - Money{30.0, "USD"};
    CHECK(a.getBalance() == doctest::Approx(70.0));
}

TEST_CASE("operator- throws InsufficientFunds when policy rejects") {
    class StrictAccount : public Account {
    public:
        using Account::Account;
        std::string kind() const override { return "Strict"; }
        bool canWithdraw(double amt) const override { return amt <= balance_(); }
    private:
        double balance_() const { return getBalance(); }
    };
    auto reg = makeRegistry();
    StrictAccount a("acc_1", "A", "USD", Money{10.0, "USD"}, &reg);
    Money over{50.0, "USD"};
    CHECK_THROWS_AS(a - over, InsufficientFunds);
    CHECK(a.getBalance() == doctest::Approx(10.0));  // unchanged
}

TEST_CASE("operator+ propagates CurrencyUnknown from registry") {
    auto reg = makeRegistry();
    PermissiveAccount a("acc_1", "A", "USD", Money{100.0, "USD"}, &reg);
    Money unknown{1.0, "ZZZ"};
    CHECK_THROWS_AS(a + unknown, CurrencyUnknown);
}
