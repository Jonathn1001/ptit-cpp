#include "Transaction.h"
#include <iomanip>
#include <ostream>
#include <utility>

namespace {
const char* typeName(TxType t) {
    switch (t) {
        case TxType::CREATE_ACCOUNT: return "CREATE_ACCOUNT";
        case TxType::DEPOSIT:        return "DEPOSIT";
        case TxType::WITHDRAW:       return "WITHDRAW";
        case TxType::TRANSFER:       return "TRANSFER";
        case TxType::SET_RATE:       return "SET_RATE";
        case TxType::APPLY_INTEREST: return "APPLY_INTEREST";
    }
    return "?";
}
const char* statusName(TxStatus s) {
    return s == TxStatus::SUCCESS ? "SUCCESS" : "FAILED";
}
}

Transaction::Transaction(int id, TxType type, TxStatus status,
                         std::string fromId, std::string toId,
                         Money amount, std::string note, std::time_t timestamp)
    : id_(id), type_(type), status_(status),
      fromId_(std::move(fromId)), toId_(std::move(toId)),
      amount_(std::move(amount)), note_(std::move(note)),
      timestamp_(timestamp) {}

void Transaction::print(std::ostream& os) const {
    char timebuf[32] = {};
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &timestamp_);
#else
    localtime_r(&timestamp_, &tm);
#endif
    std::strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", &tm);

    os << "#" << std::setw(4) << std::setfill('0') << id_ << std::setfill(' ')
       << "  " << timebuf
       << "  " << std::setw(14) << std::left << typeName(type_)
       << " " << std::setw(7) << statusName(status_)
       << std::right;
  if (!fromId_.empty()) {
    os << "  from=" << fromId_;
}

if (!toId_.empty()) {
    os << "  to=" << toId_;
}

if (!amount_.currency.empty()) {
    os << "  amt=" << amount_;
}

if (!note_.empty()) {
    os << "  (" << note_ << ")";
}
    os << "\n";
}