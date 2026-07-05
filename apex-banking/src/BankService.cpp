#include "BankService.h"

#include "Account.h"
#include "Savings.h"
#include "Transaction.h"

#include <iostream>
#include <thread>

void BankService::requireAdmin(const User& actor) const {
    if (!actor.isAdmin()) {
        throw AccessDenied("access denied: cần admin");
    }
}

void BankService::requireOwnerOrAdmin(const User& actor, const std::string& accountId) const {
    if (actor.isAdmin()) return;
    if (!actor.owns(accountId)) {
        throw AccessDenied("access denied: account " + accountId + " không thuộc user " + actor.userId());
    }
}

std::string BankService::createSavings(User& actor,
                                       const std::string& ownerInput,
                                       const std::string& currency,
                                       const Money& opening,
                                       double interestRate) {
    std::lock_guard<std::mutex> guard(bankMutex_);

    const std::string owner = actor.isAdmin() ? ownerInput : actor.fullName();
    Account& account = bank_.createSavings(owner, currency, opening, interestRate);

    if (!actor.isAdmin()) {
        actor.addAccountId(account.getId());
    }

    return account.getId();
}

std::string BankService::createChecking(User& actor,
                                        const std::string& ownerInput,
                                        const std::string& currency,
                                        const Money& opening,
                                        double overdraftLimit) {
    std::lock_guard<std::mutex> guard(bankMutex_);

    const std::string owner = actor.isAdmin() ? ownerInput : actor.fullName();
    Account& account = bank_.createChecking(owner, currency, opening, overdraftLimit);

    if (!actor.isAdmin()) {
        actor.addAccountId(account.getId());
    }

    return account.getId();
}

void BankService::deposit(const User& actor, const std::string& accountId, const Money& money) {
    requireOwnerOrAdmin(actor, accountId);
    std::lock_guard<std::mutex> guard(bankMutex_);
    bank_.deposit(accountId, money);
}

void BankService::withdraw(const User& actor, const std::string& accountId, const Money& money) {
    requireOwnerOrAdmin(actor, accountId);
    std::lock_guard<std::mutex> guard(bankMutex_);
    bank_.withdraw(accountId, money);
}

void BankService::transfer(const User& actor,
                           const std::string& fromId,
                           const std::string& toId,
                           const Money& money) {
    requireOwnerOrAdmin(actor, fromId);
    std::lock_guard<std::mutex> guard(bankMutex_);
    bank_.transfer(fromId, toId, money);
}

void BankService::updateRate(const User& actor, const std::string& currency, double rate) {
    requireAdmin(actor);
    std::lock_guard<std::mutex> guard(bankMutex_);
    bank_.setRate(currency, rate);
}

void BankService::applyInterest(const User& actor, const std::string& accountId) {
    requireAdmin(actor);
    std::lock_guard<std::mutex> guard(bankMutex_);

    Account& account = bank_.require(accountId);
    Savings* savings = dynamic_cast<Savings*>(&account);
    if (!savings) {
        throw BadInput(accountId + " không phải Savings");
    }

    savings->applyInterest();
}

void BankService::printAccount(const User& actor, const std::string& accountId, std::ostream& os) const {
    requireOwnerOrAdmin(actor, accountId);
    std::lock_guard<std::mutex> guard(bankMutex_);
    bank_.require(accountId).print(os);
    os << "\n";
}

void BankService::printVisibleAccounts(const User& actor, std::ostream& os) const {
    std::lock_guard<std::mutex> guard(bankMutex_);

    if (actor.isAdmin()) {
        bank_.listAccounts(os);
        return;
    }

    if (actor.accountIds().empty()) {
        os << " (user chưa có account)\n";
        return;
    }

    for (const std::string& accountId : actor.accountIds()) {
        bank_.require(accountId).print(os);
        os << "\n";
    }
}

void BankService::printLedger(const User& actor, std::ostream& os) const {
    std::lock_guard<std::mutex> guard(bankMutex_);

    if (actor.isAdmin()) {
        bank_.printLedger(os);
        return;
    }

    bool printed = false;
    for (const Transaction& tx : bank_.ledgerEntries()) {
        if (actor.owns(tx.fromId()) || actor.owns(tx.toId())) {
            tx.print(os);
            printed = true;
        }
    }

    if (!printed) {
        os << " (user chưa có giao dịch)\n";
    }
}

void BankService::printRates(std::ostream& os) const {
    std::lock_guard<std::mutex> guard(bankMutex_);
    bank_.rates().list(os);
}

std::string BankService::accountCurrency(const User& actor, const std::string& accountId) const {
    requireOwnerOrAdmin(actor, accountId);
    std::lock_guard<std::mutex> guard(bankMutex_);
    return bank_.require(accountId).getCurrency();
}

double BankService::accountBalance(const User& actor, const std::string& accountId) const {
    requireOwnerOrAdmin(actor, accountId);
    std::lock_guard<std::mutex> guard(bankMutex_);
    return bank_.require(accountId).getBalance();
}

void BankService::demoConcurrentWithdraw(const User& actor,
                                         const std::string& accountId,
                                         double amount,
                                         std::ostream& os) {
    requireOwnerOrAdmin(actor, accountId);

    const std::string currency = accountCurrency(actor, accountId);
    std::mutex outputMutex;

    os << "\n--- Double-Spend demo: std::thread + std::mutex ---\n";
    os << "Hai thread cùng rút " << amount << " " << currency
       << " từ " << accountId << " cùng lúc.\n";
    os << "BankService dùng std::mutex nên giao dịch chạy tuần tự.\n";

    auto worker = [&](const std::string& name) {
        try {
            withdraw(actor, accountId, Money{amount, currency});
            std::lock_guard<std::mutex> lock(outputMutex);
            os << name << ": THÀNH CÔNG\n";
        } catch (const BankError& e) {
            std::lock_guard<std::mutex> lock(outputMutex);
            os << name << ": THẤT BẠI -> " << e.what() << "\n";
        }
    };

    std::thread t1(worker, "Thread A");
    std::thread t2(worker, "Thread B");

    t1.join();
    t2.join();

    os << "Account cuối: ";
    printAccount(actor, accountId, os);
    os << "--- Kết thúc demo ---\n";
}
