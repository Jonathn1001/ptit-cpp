#pragma once

#include "AuthService.h"
#include "Bank.h"
#include "Errors.h"
#include "Money.h"

#include <iosfwd>
#include <mutex>
#include <string>

struct AccessDenied : BankError {
    using BankError::BankError;
};

class BankService {
    Bank& bank_;
    mutable std::mutex bankMutex_;

    void requireAdmin(const User& actor) const;
    void requireOwnerOrAdmin(const User& actor, const std::string& accountId) const;

public:
    explicit BankService(Bank& bank) : bank_(bank) {}

    std::string createSavings(User& actor,
                              const std::string& ownerInput,
                              const std::string& currency,
                              const Money& opening,
                              double interestRate);

    std::string createChecking(User& actor,
                               const std::string& ownerInput,
                               const std::string& currency,
                               const Money& opening,
                               double overdraftLimit);

    void deposit(const User& actor, const std::string& accountId, const Money& money);
    void withdraw(const User& actor, const std::string& accountId, const Money& money);
    void transfer(const User& actor, const std::string& fromId, const std::string& toId, const Money& money);

    void updateRate(const User& actor, const std::string& currency, double rate);
    void applyInterest(const User& actor, const std::string& accountId);

    void printAccount(const User& actor, const std::string& accountId, std::ostream& os) const;
    void printVisibleAccounts(const User& actor, std::ostream& os) const;
    void printLedger(const User& actor, std::ostream& os) const;
    void printRates(std::ostream& os) const;

    std::string accountCurrency(const User& actor, const std::string& accountId) const;
    double accountBalance(const User& actor, const std::string& accountId) const;

    void demoConcurrentWithdraw(const User& actor,
                                const std::string& accountId,
                                double amount,
                                std::ostream& os);
};
