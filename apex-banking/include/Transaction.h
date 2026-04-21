#pragma once
#include "Money.h"
#include <ctime>
#include <iosfwd>
#include <string>

enum class TxType   { CREATE_ACCOUNT, DEPOSIT, WITHDRAW, TRANSFER, SET_RATE, APPLY_INTEREST };
enum class TxStatus { SUCCESS, FAILED };

/**
 * Immutable ledger entry. Populated once by Bank and never mutated.
 * Fields that don't apply to a given TxType are left empty/zero —
 * e.g., SET_RATE uses `note` for the rate value and leaves fromId/toId empty.
 */
class Transaction {
    int id_;
    TxType type_;
    TxStatus status_;
    std::string fromId_;
    std::string toId_;
    Money amount_;
    std::string note_;
    std::time_t timestamp_;
public:
    Transaction(int id, TxType type, TxStatus status,
                std::string fromId, std::string toId,
                Money amount, std::string note, std::time_t timestamp);

    int id() const { return id_; }
    TxType type() const { return type_; }
    TxStatus status() const { return status_; }
    const std::string& fromId() const { return fromId_; }
    const std::string& toId() const { return toId_; }
    const Money& amount() const { return amount_; }
    const std::string& note() const { return note_; }
    std::time_t timestamp() const { return timestamp_; }

    /** Prints one human-readable line to `os`. Trailing newline included. */
    void print(std::ostream& os) const;
};
