#include "user_db.h"

namespace NDataBase {
    TUserDb::TUserDb(const int userId, const std::string_view hash)
    : UserId(userId)
    , Hash(hash) { }

    std::string_view TUserDb::getHash() const {
        return Hash;
    }

    long long TUserDb::getUserId() const {
        return UserId;
    }

    void TUserDb::setHash(const std::string_view hash) {
        Hash = hash;
    }

    void TUserDb::setUserId(const int userId) {
        UserId = userId;
    }
}