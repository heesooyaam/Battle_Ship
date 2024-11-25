#include <Wt/Dbo/Dbo.h>
#include <Wt/Dbo/Session.h>
#include <Wt/Dbo/backend/Sqlite3.h>

namespace NDataBase {
    static constexpr const char* USER_DB_PATH = "./data_bases/user.db";
    static constexpr const char* USER_DB_NAME = "user";

    class TUserDb {
    public:
        explicit TUserDb() = default;
        TUserDb(int userId, std::string_view hash);

        template<typename Action>
        void persist(Action& action) {
            Wt::Dbo::field(action, UserId, "user_id");
            Wt::Dbo::field(action, Hash, "hash");
        }

        std::string_view getHash() const;
        long long getUserId() const;

        void setHash(std::string_view hash);
        void setUserId(int userId);

    private:
        long long UserId;
        std::string Hash;
    };
}