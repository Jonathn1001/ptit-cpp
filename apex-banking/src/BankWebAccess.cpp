#include "Bank.h"

std::vector<const Account*> Bank::allAccounts() const {
    std::vector<const Account*> result;
    result.reserve(accounts.size());

    for (const auto& [id, ptr] : accounts) {
        (void)id;
        result.push_back(ptr.get());
    }

    return result;
}
