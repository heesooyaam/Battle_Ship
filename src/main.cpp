#include "applications/authorization/telegram_authorization.h"

#include <Wt/WServer.h>

int main(int argc, char **argv) {
    Wt::WServer server(argc, argv);

    server.addEntryPoint(
        Wt::EntryPointType::Application,
        [](const Wt::WEnvironment& env) {
            return std::make_unique<NApplication::TTelegramAuthorization>(env);
        }
    );

    server.addEntryPoint(
        Wt::EntryPointType::Application,
        [](const Wt::WEnvironment& env) {
            return std::make_unique<NApplication::TTelegramAuthorization>(env);
        },
        "/login"
    );

    if (server.start()) {
        Wt::WServer::waitForShutdown();
        server.stop();
    }

    return 0;
}
