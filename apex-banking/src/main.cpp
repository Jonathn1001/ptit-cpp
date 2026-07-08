#include "AuthService.h"
#include "BankService.h"
#include "Bank.h"
#include "Errors.h"
#include "Money.h"

#include <iostream>
#include <sstream>
#include <string>

namespace {

struct UserSession {
    User* user{};
    std::string accountId;
};

std::string readLine(const std::string& prompt) {
    std::cout << prompt;
    std::string s;
    if (!std::getline(std::cin, s)) throw BadInput("kết thúc dữ liệu nhập ngoài dự kiến");
    return s;
}

double readDouble(const std::string& prompt) {
    std::string s = readLine(prompt);
    std::istringstream is(s);
    double v;
    if (!(is >> v)) throw BadInput("cần nhập một số, nhưng nhận được: " + s);
    std::string rest;
    if (is >> rest) throw BadInput("ký tự thừa sau số: " + s);
    return v;
}

int readMenuChoice() {
    std::string s = readLine("Lựa chọn > ");
    std::istringstream is(s);
    int v;
    if (!(is >> v)) throw BadInput("lựa chọn menu phải là số nguyên");
    return v;
}

void seed(Bank& bank, AuthService& auth) {
    bank.setRate("USD", 1.0);
    bank.setRate("VND", 24500.0);
    bank.setRate("EUR", 0.92);
    bank.setRate("JPY", 155.0);

    auth.registerUser("ADMIN", "Quản trị hệ thống", Role::ADMIN);

    Account& a1 = bank.createSavings("Nguyễn Văn A", "USD", Money{200.0, "USD"}, 0.05);
    Account& a2 = bank.createChecking("Trần Thị B", "VND", Money{1000000.0, "VND"}, 500000.0);

    auth.registerUser(a1.getId(), "Nguyễn Văn A", Role::USER);
    auth.registerUser(a2.getId(), "Trần Thị B", Role::USER);
    auth.addAccountToUser(a1.getId(), a1.getId());
    auth.addAccountToUser(a2.getId(), a2.getId());

    std::cout << "[seed] Admin vào trực tiếp.\n";
    std::cout << "[seed] User demo đăng nhập bằng Account ID: "
              << a1.getId() << ", " << a2.getId() << "\n";
    std::cout << "[seed] Đã seed 4 currency và 2 account mẫu.\n";
}

void printRoleMenu() {
    std::cout << "\n"
              << "==============================================\n"
              << " APEX Banking CLI \n"
              << "==============================================\n"
              << " 1. Admin\n"
              << " 2. User\n"
              << " 0. Thoát\n\n";
}

void printAdminEntryMenu() {
    std::cout << "\n"
              << "=========== ADMIN ===========\n"
              << " 1. Vào admin\n"
              << " 0. Quay lại\n\n";
}

void printUserEntryMenu() {
    std::cout << "\n"
              << "=========== USER ===========\n"
              << " 1. Nhập Account ID\n"
              << " 2. Tạo user mới\n"
              << " 0. Quay lại\n\n";
}

void printAdminMenu(const User& user) {
    std::cout << "\n"
              << "=========== ADMIN MENU ===========\n"
              << "Hiện tại: " << user.userId() << " (" << roleToString(user.role()) << ")\n"
              << "\n-- Account --\n"
              << " 1. Savings\n"
              << " 2. Checking\n"
              << " 3. Danh sách account\n"
              << " 4. Chi tiết account\n"
              << "\n-- Giao dịch --\n"
              << " 5. Nạp tiền\n"
              << " 6. Rút tiền\n"
              << " 7. Chuyển khoản\n"
              << "\n-- Currency --\n"
              << " 8. Tỷ giá\n"
              << " 9. Cập nhật tỷ giá\n"
              << "\n-- Report & Demo --\n"
              << "10. Ledger\n"
              << "11. Lãi Savings\n"
              << "12. Demo double-spend\n"
              << "13. Demo tỷ giá lỗi\n"
              << " 0. Đăng xuất\n\n";
}

void printUserMenu(const User& user, const std::string& accountId) {
    std::cout << "\n"
              << "=========== USER MENU ===========\n"
              << "Account ID: " << accountId << " (" << user.fullName() << ")\n"
              << "\n-- Account --\n"
              << " 1. Account\n"
              << " 2. Savings\n"
              << " 3. Checking\n"
              << "\n-- Giao dịch --\n"
              << " 4. Nạp tiền\n"
              << " 5. Rút tiền\n"
              << " 6. Chuyển khoản\n"
              << "\n-- Info --\n"
              << " 7. Tỷ giá\n"
              << " 8. Ledger của tôi\n"
              << " 0. Đăng xuất\n\n";
}

UserSession selectUserById(AuthService& auth, Role expectedRole) {
    std::string accountId = readLine("Account ID: ");

    User* user = auth.loginById(accountId);
    if (!user) {
        std::cout << "Không tìm thấy Account ID: " << accountId << "\n";
        return {};
    }

    if (user->role() != expectedRole) {
        std::cout << "User này có vai trò " << roleToString(user->role())
                  << ", không phải " << roleToString(expectedRole) << ".\n";
        return {};
    }

    if (!user->isAdmin() && !user->owns(accountId)) {
        std::cout << "Account ID này không thuộc user.\n";
        return {};
    }

    std::cout << "Đã vào Account ID: " << accountId << "\n";
    return {user, accountId};
}

User* getBuiltInAdmin(AuthService& auth) {
    User* admin = auth.findUser("ADMIN");
    if (!admin) {
        throw BadInput("thiếu user admin nội bộ");
    }
    return admin;
}
void assignAdminCreatedAccountToUser(AuthService& auth,
                                     const std::string& accountId,
                                     const std::string& fullName) {
    if (!auth.exists(accountId)) {
        User& user = auth.registerUser(accountId, fullName, Role::USER);
        user.addAccountId(accountId);
        return;
    }

    auth.addAccountToUser(accountId, accountId);
}
std::string doCreateSavings(BankService& service, User& actor, AuthService* auth = nullptr) {
    std::string owner;
    if (actor.isAdmin()) {
        owner = readLine("Owner: ");
    } else {
        owner = actor.fullName();
        std::cout << "Owner: " << owner << "\n";
    }

    std::string cur = readLine("Currency (USD/VND/EUR/JPY): ");
    double amt = readDouble("Opening balance: ");
    double rate = readDouble("Interest rate (0.05 = 5%): ");

    std::string id = service.createSavings(actor, owner, cur, Money{amt, cur}, rate);
    if (actor.isAdmin() && auth != nullptr) {
    assignAdminCreatedAccountToUser(*auth, id, owner);
}
    std::cout << "Tạo Savings thành công. Account ID = " << id << "\n";
    service.printAccount(actor, id, std::cout);
    return id;
}

std::string doCreateChecking(BankService& service, User& actor, AuthService* auth = nullptr) {
    std::string owner;
    if (actor.isAdmin()) {
        owner = readLine("Owner: ");
    } else {
        owner = actor.fullName();
        std::cout << "Owner: " << owner << "\n";
    }

    std::string cur = readLine("Currency (USD/VND/EUR/JPY): ");
    double amt = readDouble("Opening balance: ");
    double od = readDouble("Overdraft limit: ");

    std::string id = service.createChecking(actor, owner, cur, Money{amt, cur}, od);
    if (actor.isAdmin() && auth != nullptr) {
    assignAdminCreatedAccountToUser(*auth, id, owner);}
    std::cout << "Tạo Checking thành công. Account ID = " << id << "\n";
    service.printAccount(actor, id, std::cout);
    return id;
}

void doShowAccount(BankService& service, const User& actor) {
    std::string id = readLine("Account ID: ");
    service.printAccount(actor, id, std::cout);
}

void doDeposit(BankService& service, const User& actor, const std::string& currentAccountId = "") {
    std::string id;
    if (actor.isAdmin()) {
        id = readLine("Account ID: ");
    } else {
        id = currentAccountId;
        std::cout << "Account ID: " << id << "\n";
    }

    double amt = readDouble("Amount: ");
    std::string cur = readLine("Currency: ");

    service.deposit(actor, id, Money{amt, cur});
    std::cout << "Nạp tiền thành công.\n";
    service.printAccount(actor, id, std::cout);
}

void doWithdraw(BankService& service, const User& actor, const std::string& currentAccountId = "") {
    std::string id;
    if (actor.isAdmin()) {
        id = readLine("Account ID: ");
    } else {
        id = currentAccountId;
        std::cout << "Account ID: " << id << "\n";
    }

    double amt = readDouble("Amount: ");
    std::string cur = readLine("Currency: ");

    service.withdraw(actor, id, Money{amt, cur});
    std::cout << "Rút tiền thành công.\n";
    service.printAccount(actor, id, std::cout);
}

void doTransfer(BankService& service, const User& actor, const std::string& currentAccountId = "") {
    std::string fromId;
    if (actor.isAdmin()) {
        fromId = readLine("From Account ID: ");
    } else {
        fromId = currentAccountId;
        std::cout << "From Account ID: " << fromId << "\n";
    }

    std::string toId = readLine("To Account ID: ");
    double amt = readDouble("Amount: ");
    std::string cur = readLine("Currency: ");

    service.transfer(actor, fromId, toId, Money{amt, cur});
    std::cout << "Chuyển khoản thành công.\n";
    service.printAccount(actor, fromId, std::cout);
}

void doUpdateRate(BankService& service, const User& actor) {
    std::string code = readLine("Currency code: ");
    double rate = readDouble("Rate mới (theo 1 USD): ");
    service.updateRate(actor, code, rate);
    std::cout << "Cập nhật tỷ giá thành công.\n";
}

void doApplyInterest(BankService& service, const User& actor) {
    std::string id = readLine("Savings Account ID: ");
    service.applyInterest(actor, id);
    std::cout << "Áp dụng lãi suất thành công.\n";
    service.printAccount(actor, id, std::cout);
}

void doDoubleSpendDemo(BankService& service, const User& actor) {
    std::string id = readLine("Account ID demo: ");
    double amt = readDouble("Amount mỗi thread rút: ");
    service.demoConcurrentWithdraw(actor, id, amt, std::cout);
}

void doBadRateDemo(BankService& service, const User& actor) {
    std::cout << "Test rate EUR = -1.5 ...\n";
    try {
        service.updateRate(actor, "EUR", -1.5);
    } catch (const BankError& e) {
        std::cout << "THẤT BẠI: " << e.what() << "\n";
    }

    std::cout << "Test rate JPY = 0 ...\n";
    try {
        service.updateRate(actor, "JPY", 0.0);
    } catch (const BankError& e) {
        std::cout << "THẤT BẠI: " << e.what() << "\n";
    }

    std::cout << "Bank vẫn ghi SET_RATE thất bại vào ledger.\n";
}

void adminLoop(AuthService& auth, BankService& service, User& currentUser) {
    while (true) {
        printAdminMenu(currentUser);

        try {
            int choice = readMenuChoice();
            switch (choice) {
            case 0: std::cout << "Đăng xuất admin. \n"; return;
            case 1: doCreateSavings(service, currentUser, &auth); break;
            case 2: doCreateChecking(service, currentUser, &auth); break;
            case 3: service.printVisibleAccounts(currentUser, std::cout); break;
            case 4: doShowAccount(service, currentUser); break;
            case 5: doDeposit(service, currentUser); break;
            case 6: doWithdraw(service, currentUser); break;
            case 7: doTransfer(service, currentUser); break;
            case 8: service.printRates(std::cout); break;
            case 9: doUpdateRate(service, currentUser); break;
            case 10: service.printLedger(currentUser, std::cout); break;
            case 11: doApplyInterest(service, currentUser); break;
            case 12: doDoubleSpendDemo(service, currentUser); break;
            case 13: doBadRateDemo(service, currentUser); break;
            default: std::cout << "Lựa chọn không hợp lệ.\n";
            }
        } catch (const BankError& e) {
            std::cout << "Lỗi: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Lỗi hệ thống: " << e.what() << "\n";
        }
    }
}

void userLoop(BankService& service, User& currentUser, std::string currentAccountId) {
    while (true) {
        printUserMenu(currentUser, currentAccountId);

        try {
            int choice = readMenuChoice();
            switch (choice) {
            case 0:
                std::cout << "Đăng xuất user. \n";
                return;
            case 1:
                service.printAccount(currentUser, currentAccountId, std::cout);
                break;
            case 2:
                currentAccountId = doCreateSavings(service, currentUser);
                std::cout << "Account đang dùng: " << currentAccountId << "\n";
                break;
            case 3:
                currentAccountId = doCreateChecking(service, currentUser);
                std::cout << "Account đang dùng: " << currentAccountId << "\n";
                break;
            case 4:
                doDeposit(service, currentUser, currentAccountId);
                break;
            case 5:
                doWithdraw(service, currentUser, currentAccountId);
                break;
            case 6:
                doTransfer(service, currentUser, currentAccountId);
                break;
            case 7:
                service.printRates(std::cout);
                break;
            case 8:
                service.printLedger(currentUser, std::cout);
                break;
            default:
                std::cout << "Lựa chọn không hợp lệ.\n";
            }
        } catch (const BankError& e) {
            std::cout << "Lỗi: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Lỗi hệ thống: " << e.what() << "\n";
        }
    }
}

UserSession registerNewUser(AuthService& auth, BankService& service) {
    std::string fullName = readLine("Họ tên: ");
    if (fullName.empty()) throw BadInput("họ tên trống");

    std::cout << "Chọn account đầu tiên:\n"
              << " 1. Savings\n"
              << " 2. Checking\n";
    int type = readMenuChoice();
    if (type != 1 && type != 2) {
        throw BadInput("loại account không hợp lệ");
    }

    User* admin = getBuiltInAdmin(auth);
    std::string cur = readLine("Currency (USD/VND/EUR/JPY): ");
    double amt = readDouble("Opening balance: ");

    std::string accountId;
    if (type == 1) {
        double rate = readDouble("Interest rate (0.05 = 5%): ");
        accountId = service.createSavings(*admin, fullName, cur, Money{amt, cur}, rate);
    } else {
        double od = readDouble("Overdraft limit: ");
        accountId = service.createChecking(*admin, fullName, cur, Money{amt, cur}, od);
    }

    User& user = auth.registerUser(accountId, fullName, Role::USER);
    user.addAccountId(accountId);

    std::cout << "Đã tạo user. User ID = Account ID = " << accountId << "\n";
    service.printAccount(user, accountId, std::cout);
    return {&user, accountId};
}

void adminEntry(AuthService& auth, BankService& service) {
    while (true) {
        printAdminEntryMenu();
        try {
            int choice = readMenuChoice();
            if (choice == 0) return;
            if (choice != 1) {
                std::cout << "Lựa chọn không hợp lệ.\n";
                continue;
            }

            User* admin = getBuiltInAdmin(auth);
            std::cout << "Vào admin.\n";
            adminLoop(auth,service, *admin);
        } catch (const BankError& e) {
            std::cout << "Lỗi: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Lỗi hệ thống: " << e.what() << "\n";
        }
    }
}

void userEntry(AuthService& auth, BankService& service) {
    while (true) {
        printUserEntryMenu();
        try {
            int choice = readMenuChoice();
            if (choice == 0) return;

            if (choice == 1) {
                UserSession session = selectUserById(auth, Role::USER);
                if (session.user) userLoop(service, *session.user, session.accountId);
            } else if (choice == 2) {
                UserSession session = registerNewUser(auth, service);
                if (session.user) {
                    std::cout << "Vào user mới.\n";
                    userLoop(service, *session.user, session.accountId);
                }
            } else {
                std::cout << "Lựa chọn không hợp lệ.\n";
            }
        } catch (const BankError& e) {
            std::cout << "Lỗi: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Lỗi hệ thống: " << e.what() << "\n";
        }
    }
}

} // namespace

int main() {
    Bank bank;
    AuthService auth;
    seed(bank, auth);
    BankService service(bank);

    while (true) {
        printRoleMenu();
        try {
            int choice = readMenuChoice();
            switch (choice) {
            case 0:
                std::cout << "Bye!\n";
                return 0;
            case 1:
                adminEntry(auth, service);
                break;
            case 2:
                userEntry(auth, service);
                break;
            default:
                std::cout << "Lựa chọn không hợp lệ.\n";
            }
        } catch (const BankError& e) {
            std::cout << "Lỗi: " << e.what() << "\n";
        } catch (const std::exception& e) {
            std::cout << "Lỗi hệ thống: " << e.what() << "\n";
        }
    }
}
