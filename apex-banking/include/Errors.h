#pragma once
#include <stdexcept>

/**
 * Root of every exception thrown by the banking domain.
 * The CLI catches this type exactly once at its top level.
 */
struct BankError : std::runtime_error {
    using std::runtime_error::runtime_error;
};

struct AccountNotFound     : BankError { using BankError::BankError; };
struct DuplicateAccountId  : BankError { using BankError::BankError; };
struct InsufficientFunds   : BankError { using BankError::BankError; };
struct CurrencyUnknown     : BankError { using BankError::BankError; };
struct InvalidRate         : BankError { using BankError::BankError; };
struct DoubleSpendDetected : BankError { using BankError::BankError; };
struct BadInput            : BankError { using BankError::BankError; };
