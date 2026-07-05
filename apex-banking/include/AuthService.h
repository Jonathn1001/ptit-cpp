#pragma once

#include "Errors.h"

#include <algorithm>
#include <iosfwd>
#include <map>
#include <string>
#include <vector>

enum class Role {
    ADMIN,
    USER
};

std::string roleToString(Role role);

class User {
    std::string userId_;
    std::string fullName_;
    Role role_;
    std::vector<std::string> accountIds_;

public:
    User(std::string userId, std::string fullName, Role role);

    const std::string& userId() const { return userId_; }
    const std::string& fullName() const { return fullName_; }
    Role role() const { return role_; }
    bool isAdmin() const { return role_ == Role::ADMIN; }

    const std::vector<std::string>& accountIds() const { return accountIds_; }
    bool owns(const std::string& accountId) const;
    void addAccountId(const std::string& accountId);
};

class AuthService {
    // Key = User ID. In this version, User ID is the first Account ID.
    std::map<std::string, User> users_;

public:
    bool exists(const std::string& userId) const;

    User& registerUser(const std::string& userId,
                       const std::string& fullName,
                       Role role = Role::USER);

    // Demo-level login: no password. User logs in by entering an Account ID.
    User* loginById(const std::string& userId);
    const User* findUser(const std::string& userId) const;
    User* findUser(const std::string& userId);

    void addAccountToUser(const std::string& userId, const std::string& accountId);
    void listUsers(std::ostream& os) const;
};
