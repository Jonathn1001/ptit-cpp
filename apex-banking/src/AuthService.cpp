#include "AuthService.h"

#include <iostream>
#include <utility>

std::string roleToString(Role role) {
    return role == Role::ADMIN ? "ADMIN" : "USER";
}

User::User(std::string userId, std::string fullName, Role role)
    : userId_(std::move(userId)),
      fullName_(std::move(fullName)),
      role_(role) {}

bool User::owns(const std::string& accountId) const {
    return std::find(accountIds_.begin(), accountIds_.end(), accountId) != accountIds_.end();
}

void User::addAccountId(const std::string& accountId) {
    if (!owns(accountId)) {
        accountIds_.push_back(accountId);
    }
}

bool AuthService::exists(const std::string& userId) const {
    return users_.find(userId) != users_.end();
}

User& AuthService::registerUser(const std::string& userId,
                                const std::string& fullName,
                                Role role) {
    if (userId.empty()) throw BadInput("Account ID trống");
    if (fullName.empty()) throw BadInput("họ tên trống");
    if (exists(userId)) throw BadInput("Account ID đã tồn tại: " + userId);

    auto [it, inserted] = users_.emplace(
        userId,
        User{userId, fullName, role}
    );
    (void)inserted;
    return it->second;
}

User* AuthService::loginById(const std::string& userId) {
    auto it = users_.find(userId);
    if (it != users_.end()) return &it->second;

    // Cho phép đăng nhập bằng bất kỳ Account ID nào thuộc user.
    for (auto& [id, user] : users_) {
        if (user.owns(userId)) return &user;
    }

    return nullptr;
}

const User* AuthService::findUser(const std::string& userId) const {
    auto it = users_.find(userId);
    return it == users_.end() ? nullptr : &it->second;
}

User* AuthService::findUser(const std::string& userId) {
    auto it = users_.find(userId);
    return it == users_.end() ? nullptr : &it->second;
}

void AuthService::addAccountToUser(const std::string& userId, const std::string& accountId) {
    User* user = findUser(userId);
    if (!user) throw BadInput("không có Account ID: " + userId);
    user->addAccountId(accountId);
}

void AuthService::listUsers(std::ostream& os) const {
    if (users_.empty()) {
        os << " (chưa có user)\n";
        return;
    }

    for (const auto& [userId, user] : users_) {
        os << " - Account ID: " << userId << " | " << user.fullName()
           << " | " << roleToString(user.role())
           << " | số account=" << user.accountIds().size() << "\n";
    }
}
