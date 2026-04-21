#include "doctest.h"
#include "Bank.h"
#include "Errors.h"
#include "Savings.h"
#include "Checking.h"

namespace {
Bank makeSeededBank() {
    Bank b;
    b.setRate("USD", 1.0);
    b.setRate("VND", 24500.0);
    return b;
}
}

TEST_CASE("createSavings registers in the account map and returns reference") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("Alice", "USD", Money{100.0, "USD"}, 0.05);
    CHECK(a.kind() == "Savings");
    CHECK(a.getBalance() == doctest::Approx(100.0));
    CHECK(b.find(a.getId()) == &a);
}

TEST_CASE("createChecking registers separately and IDs are unique") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{10.0, "USD"}, 0.01);
    Account& c = b.createChecking("B", "USD", Money{20.0, "USD"}, 50.0);
    CHECK(a.getId() != c.getId());
}

TEST_CASE("find returns nullptr for missing id, require throws") {
    Bank b = makeSeededBank();
    CHECK(b.find("nope") == nullptr);
    CHECK_THROWS_AS(b.require("nope"), AccountNotFound);
}

TEST_CASE("deposit updates balance and logs a SUCCESS tx") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{100.0, "USD"}, 0.0);
    Money m{50.0, "USD"};
    b.deposit(a.getId(), m);
    CHECK(a.getBalance() == doctest::Approx(150.0));
    auto log = b.ledgerEntries();
    CHECK(log.size() == 2);  // CREATE_ACCOUNT + DEPOSIT
    CHECK(log.back().type() == TxType::DEPOSIT);
    CHECK(log.back().status() == TxStatus::SUCCESS);
}

TEST_CASE("withdraw failure is still logged as FAILED") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{10.0, "USD"}, 0.0);
    Money big{999.0, "USD"};
    CHECK_THROWS_AS(b.withdraw(a.getId(), big), InsufficientFunds);
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::WITHDRAW);
    CHECK(log.back().status() == TxStatus::FAILED);
}

TEST_CASE("transfer moves funds and logs single TRANSFER entry") {
    Bank b = makeSeededBank();
    Account& from = b.createChecking("From", "USD", Money{100.0, "USD"}, 0.0);
    Account& to   = b.createSavings ("To",   "VND", Money{0.0,   "VND"}, 0.0);
    Money amt{10.0, "USD"};
    b.transfer(from.getId(), to.getId(), amt);
    CHECK(from.getBalance() == doctest::Approx(90.0));
    CHECK(to.getBalance()   == doctest::Approx(245000.0));
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::TRANSFER);
    CHECK(log.back().status() == TxStatus::SUCCESS);
}

TEST_CASE("transfer fails atomically when source has insufficient funds") {
    Bank b = makeSeededBank();
    Account& from = b.createSavings("From", "USD", Money{5.0, "USD"}, 0.0);
    Account& to   = b.createSavings("To",   "USD", Money{0.0, "USD"}, 0.0);
    Money amt{10.0, "USD"};
    CHECK_THROWS_AS(b.transfer(from.getId(), to.getId(), amt), InsufficientFunds);
    CHECK(from.getBalance() == doctest::Approx(5.0));
    CHECK(to.getBalance()   == doctest::Approx(0.0));
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::TRANSFER);
    CHECK(log.back().status() == TxStatus::FAILED);
}

TEST_CASE("setRate(0) on Bank throws InvalidRate and logs FAILED") {
    Bank b = makeSeededBank();
    CHECK_THROWS_AS(b.setRate("EUR", 0.0), InvalidRate);
    auto log = b.ledgerEntries();
    CHECK(log.back().type() == TxType::SET_RATE);
    CHECK(log.back().status() == TxStatus::FAILED);
}

TEST_CASE("lock / unlock pair and DoubleSpendDetected on second lock") {
    Bank b = makeSeededBank();
    Account& a = b.createSavings("A", "USD", Money{100.0, "USD"}, 0.0);
    b.lock(a.getId());
    CHECK_THROWS_AS(b.lock(a.getId()), DoubleSpendDetected);
    b.unlock(a.getId());
    b.lock(a.getId());  // reacquiring after unlock is fine
    b.unlock(a.getId());
}
