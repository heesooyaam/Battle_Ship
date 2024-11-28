#include <Wt/WApplication.h>
#include <Wt/Dbo/backend/Sqlite3.h>
#include <Wt/Dbo/Session.h>

namespace NApplication {
    class TTelegramAuthorization : public Wt::WApplication {
    public:
        explicit TTelegramAuthorization(const Wt::WEnvironment& env);
    private:
        void processAuthData(const std::string* authData);

    private:
        static const std::string BotToken;

        Wt::Dbo::Session UserDbSession;
        Wt::Dbo::backend::Sqlite3 UserDb;
    };
}
