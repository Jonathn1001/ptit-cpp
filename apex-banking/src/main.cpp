#include "Bank.h"
#include "Errors.h"
#include "Money.h"
#include "Savings.h"
#include "Checking.h"

#include <iostream>
#include <limits>
#include <sstream>
#include <string>

namespace {

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s)) throw BadInput("unexpected EOF");
    return s;
}

double readDouble(const std::string& prompt) {
    std::string s = readLine(prompt);
    std::istringstream is(s);
    double v;
    if (!(is >> v)) throw BadInput("expected a number, got: " + s);
    return v;
}

int readMenuChoice() {
    std::string s = readLine("Lựa chọn › ");
    std::istringstream is(s);
    int v;
    if (!(is >> v)) throw BadInput("menu choice must be an integer");
    return v;
}

void printMenu() {
    std::cout << "\n"
        "╔══════════════════════════════════════════════╗\n"
        "║  APEX — Hệ thống ngân hàng đa tiền tệ        ║\n"
        "╚══════════════════════════════════════════════╝\n"
        "\n── Tài khoản ──\n"
        "  1. Tạo tài khoản Savings\n"
        "  2. Tạo tài khoản Checking\n"
        "  3. Xem danh sách tài khoản\n"
        "  4. Xem chi tiết một tài khoản\n"
        "\n── Giao dịch ──\n"
        "  5. Nạp tiền (Deposit)\n"
        "  6. Rút tiền (Withdraw)\n"
        "  7. Chuyển khoản (Transfer)\n"
        "\n── Tiền tệ ──\n"
        "  8. Xem tỷ giá hiện tại\n"
        "  9. Cập nhật tỷ giá (Admin)\n"
        "\n── Báo cáo & Demo ──\n"
        " 10. In sổ cái giao dịch (Ledger)\n"
        " 11. Áp dụng lãi suất (Savings)\n"
        " 12. ⚠ Demo: Double-Spend\n"
        " 13. ⚠ Demo: Tỷ giá không hợp lệ\n"
        "\n  0. Thoát\n\n";
}

void seed(Bank& b) {
    b.setRate("USD", 1.0);
    b.setRate("VND", 24500.0);
    b.setRate("EUR", 0.92);
    b.setRate("JPY", 155.0);
    b.createSavings ("Nguyen Van A", "USD", Money{200.0,      "USD"}, 0.05);
    b.createChecking("Tran Thi B",   "VND", Money{1'000'000.0,"VND"}, 500'000.0);
    std::cout << "[seed] 4 currencies + 2 accounts loaded.\n";
}

void doCreateSavings(Bank& b) {
    std::string owner = readLine("Chủ tài khoản: ");
    std::string cur   = readLine("Loại tiền tệ (USD/VND/EUR/JPY): ");
    double amt        = readDouble("Số dư mở tài khoản: ");
    double rate       = readDouble("Lãi suất (0.05 = 5%): ");
    Account& a = b.createSavings(owner, cur, Money{amt, cur}, rate);
    std::cout << "✅ Tạo thành công: "; a.print(std::cout); std::cout << "\n";
}

void doCreateChecking(Bank& b) {
    std::string owner = readLine("Chủ tài khoản: ");
    std::string cur   = readLine("Loại tiền tệ: ");
    double amt        = readDouble("Số dư mở tài khoản: ");
    double od         = readDouble("Hạn mức thấu chi (overdraft): ");
    Account& a = b.createChecking(owner, cur, Money{amt, cur}, od);
    std::cout << "✅ Tạo thành công: "; a.print(std::cout); std::cout << "\n";
}

void doDeposit(Bank& b) {
    std::string id  = readLine("Account ID: ");
    double amt      = readDouble("Amount: ");
    std::string cur = readLine("Currency: ");
    b.deposit(id, Money{amt, cur});
    std::cout << "✅ Đã nạp " << amt << " " << cur << " vào " << id << "\n";
    b.require(id).print(std::cout); std::cout << "\n";
}

void doWithdraw(Bank& b) {
    std::string id  = readLine("Account ID: ");
    double amt      = readDouble("Amount: ");
    std::string cur = readLine("Currency: ");
    b.withdraw(id, Money{amt, cur});
    std::cout << "✅ Đã rút " << amt << " " << cur << " từ " << id << "\n";
    b.require(id).print(std::cout); std::cout << "\n";
}

void doTransfer(Bank& b) {
    std::string fromId = readLine("From account ID: ");
    std::string toId   = readLine("To account ID: ");
    double amt         = readDouble("Amount: ");
    std::string cur    = readLine("Currency: ");
    b.transfer(fromId, toId, Money{amt, cur});
    std::cout << "✅ Chuyển thành công.\n";
    b.require(fromId).print(std::cout); std::cout << "\n";
    b.require(toId).print(std::cout);   std::cout << "\n";
}

void doSetRate(Bank& b) {
    std::string code = readLine("Currency code: ");
    double rate      = readDouble("New rate (per 1 USD): ");
    b.setRate(code, rate);
    std::cout << "✅ Đã cập nhật tỷ giá.\n";
}

void doApplyInterest(Bank& b) {
    std::string id = readLine("Savings account ID: ");
    Account& a = b.require(id);
    Savings* s = dynamic_cast<Savings*>(&a);
    if (!s) throw BadInput(id + " is not a Savings account");
    s->applyInterest();
    std::cout << "✅ Đã áp dụng lãi. "; a.print(std::cout); std::cout << "\n";
}

void doShowAccount(Bank& b) {
    std::string id = readLine("Account ID: ");
    b.require(id).print(std::cout); std::cout << "\n";
}

void doDoubleSpendDemo(Bank& b) {
    std::string id  = readLine("Account ID to attack: ");
    double amt      = readDouble("Withdrawal amount (each terminal): ");
    b.simulateDoubleSpend(id, amt);
}

void doBadRateDemo(Bank& b) {
    std::cout << "Thử đặt tỷ giá EUR = -1.5 ...\n";
    try { b.setRate("EUR", -1.5); }
    catch (const BankError& e) {
        std::cout << "❌ " << e.what() << "\n";
    }
    std::cout << "Thử đặt tỷ giá JPY = 0 ...\n";
    try { b.setRate("JPY", 0.0); }
    catch (const BankError& e) {
        std::cout << "❌ " << e.what() << "\n";
    }
    std::cout << "(Cả hai giao dịch FAILED được ghi vào sổ cái.)\n";
}

}  // namespace

int main() {
    Bank bank;
    seed(bank);

    while (true) {
        printMenu();
        int choice = 0;
        try { choice = readMenuChoice(); }
        catch (const BankError& e) {
            std::cout << "❌ " << e.what() << "\n";
            continue;
        }

        try {
            switch (choice) {
                case 0:  std::cout << "Tạm biệt!\n"; return 0;
                case 1:  doCreateSavings(bank);  break;
                case 2:  doCreateChecking(bank); break;
                case 3:  bank.listAccounts(std::cout); break;
                case 4:  doShowAccount(bank);    break;
                case 5:  doDeposit(bank);        break;
                case 6:  doWithdraw(bank);       break;
                case 7:  doTransfer(bank);       break;
                case 8:  bank.rates().list(std::cout); break;
                case 9:  doSetRate(bank);        break;
                case 10: bank.printLedger(std::cout); break;
                case 11: doApplyInterest(bank);  break;
                case 12: doDoubleSpendDemo(bank); break;
                case 13: doBadRateDemo(bank);    break;
                default: std::cout << "❌ Lựa chọn không hợp lệ.\n";
            }
        } catch (const BankError& e) {
            std::cout << "❌ " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "❌ Lỗi hệ thống: " << e.what() << "\n";
        }
    }
}
