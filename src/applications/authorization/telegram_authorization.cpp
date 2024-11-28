#include "telegram_authorization.h"
#include "../../data_bases/user_db/user_db.h"

#include <Wt/WContainerWidget.h>
#include <Wt/WEnvironment.h>
#include <Wt/WTemplate.h>
#include <Wt/WText.h>
#include <Wt/Http/Cookie.h>
#include <Wt/Http/Response.h>
#include <Wt/Utils.h>
#include <Wt/WSslCertificate.h>

#include <openssl/hmac.h>
#include <openssl/sha.h>

#include <nlohmann/json.hpp>

#include <memory>


namespace {
    constexpr const char* USER_ID_COOKIE_KEY = "tg_user_id";
    constexpr const char* HASH_COOKIE_KEY = "tg_hash";
    constexpr const char* BOT_TOKEN = "XXXXXXXXXX:YYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYYY";
    
    const std::string telegramWidgetCode = R"js(
        (function() {
            var container = document.getElementById('tg-container');

            if (container) {
                var script = document.createElement('script');
                script.async = true;
                script.src = "https://telegram.org/js/telegram-widget.js?22";
                script.setAttribute('data-telegram-login', 'HesoyamBattleShipBot');
                script.setAttribute('data-size', 'large');
                script.setAttribute('data-userpic', 'true');
                script.setAttribute('data-onauth', 'onTelegramAuth(user)');
                container.appendChild(script);
            } else {
                console.error("Telegram container not found!");
            }
        })();

        window.onTelegramAuth = function(user) {
            var authData = JSON.stringify(user);
            window.location.href = window.location.pathname + '?auth_data=' + encodeURIComponent(authData);
        };
    )js";

    std::vector<unsigned char> computeSHA256(const std::string& input) {
        unsigned char hash[SHA256_DIGEST_LENGTH];
        SHA256(reinterpret_cast<const unsigned char*>(input.c_str()), input.size(), hash);
        return {hash, hash + SHA256_DIGEST_LENGTH};
    }

    std::vector<unsigned char> computeHMACSHA256(const std::vector<unsigned char>& key, const std::string& data) {
        unsigned char result[EVP_MAX_MD_SIZE];
        unsigned int resultLen;

        HMAC(EVP_sha256(), key.data(), key.size(),
             reinterpret_cast<const unsigned char*>(data.c_str()), data.size(),
             result, &resultLen);

        return {result, result + resultLen};
    }


    std::string toHexString(const std::vector<unsigned char>& data) {
        std::stringstream ss;
        for (const auto byte : data) {
            ss << std::setw(2) << std::setfill('0') << std::hex << static_cast<int>(byte);
        }
        return ss.str();
    }


    bool validateAuthorization(const nlohmann::json& json, const std::vector<unsigned char>& botTokenHash) {
        if (!json.contains("hash")) {
            return false;
        }

        const std::string receivedHash = json["hash"];
        std::map<std::string, std::string> sortedData;

        for (auto& [key, value] : json.items()) {
            if (key == "hash") {
                continue;
            }
            sortedData[key] = value.is_string() ? value.get<std::string>() : std::to_string(value.get<int>());
        }

        std::string preparedData;
        for (const auto& [key, value] : sortedData) {
            preparedData += key;
            preparedData.push_back('=');
            preparedData += value;
            preparedData.push_back('\n');
        }

        if (preparedData.empty()) {
            return false;
        }
        preparedData.pop_back();

        const std::vector<unsigned char> hmac = computeHMACSHA256(botTokenHash, preparedData);
        const std::string computedHash = toHexString(hmac);

        Wt::log("validateAuthorization")
            << "\nPrepared data: " << preparedData
            << "\nComputed hash: " << computedHash
            << "\nReceived hash: " << receivedHash;

        return computedHash == receivedHash;
    }
}

namespace NApplication {
    TTelegramAuthorization::TTelegramAuthorization(const Wt::WEnvironment &env)
        : Wt::WApplication(env)
        , UserDbSession()
        , UserDb(NDataBase::USER_DB_PATH)
    {
        Wt::log("TelegramAuthorization::TelegramAuthorization") << "started";
        setTitle("Authorization");

        UserDbSession.setConnection(std::make_unique<Wt::Dbo::backend::Sqlite3>(NDataBase::USER_DB_PATH));
        UserDbSession.mapClass<NDataBase::TUserDb>(NDataBase::USER_DB_NAME);
        try {
            UserDbSession.createTables();
        } catch (...) {
            Wt::log("TTelegramAuthorization: ") << NDataBase::USER_DB_NAME << " table already created";
        }

        Wt::WApplication::instance()->useStyleSheet("./resources/themes/auth_page_themes/tg_widget.css");
        root()->setStyleClass("root_options");

        const auto cookie = Wt::WApplication::instance()->environment().cookies();
        const auto* authData = env.getParameter("auth_data");

        if (authData || cookie.find(USER_ID_COOKIE_KEY) != cookie.end()) {
            Wt::log("TTelegramAuthorization::TTelegramAuthorization") << "" << (authData ? *authData : "");
            processAuthData(authData);
        } else {
        Wt::log("TTelegramAuthorization::TTelegramAuthorization") << "making button";

        auto* telegramContainer = root()->addWidget(std::make_unique<Wt::WContainerWidget>());
        telegramContainer->setId("tg-container");

        Wt::WApplication::instance()->doJavaScript(telegramWidgetCode);
        }
    }

    void TTelegramAuthorization::processAuthData(const std::string* authData) {
        Wt::log("TTelegramAuthorization::processAuthData:") << "started";
        const auto cookie = Wt::WApplication::instance()->environment().cookies();

        if (cookie.find(USER_ID_COOKIE_KEY) == cookie.end()) {
            Wt::log("TTelegramAuthorization::processAuthData:") << "auth_data was not found in cookies";

            const auto botTokenHash{computeSHA256(BOT_TOKEN)};
            const auto json = nlohmann::json::parse(*authData);

            if (!validateAuthorization(json, botTokenHash)) {
                root()->addWidget(
                    std::make_unique<Wt::WText>("Error: Invalid signature!")
                );
                return;
            }

            Wt::WApplication::instance()->setCookie(
                    {USER_ID_COOKIE_KEY, to_string(json["id"]), std::chrono::duration<long long>(20 * 24 * 60 * 60)}
            );

            Wt::WApplication::instance()->setCookie(
                    {HASH_COOKIE_KEY, json["hash"], std::chrono::duration<long long>(20 * 24 * 60 * 60)}
            );

            Wt::Dbo::Transaction transaction(UserDbSession);
            try {
                UserDbSession.add(
                    std::make_unique<NDataBase::TUserDb>(
                        json["id"].get<int>(),
                        json["hash"].get<std::string>()
                    )
                );
            } catch (const Wt::Dbo::Exception& e) {
                Wt::log("TTelegramAuthorization::processAuthData:")
                    << "Wt::Dbo error: " << e.what();
            } catch (const std::exception& e) {
                Wt::log("TTelegramAuthorization::processAuthData:")
                    << "Error: " << e.what();
            }
            transaction.commit();
        } else {
            Wt::log("TTelegramAuthorization::processAuthData:") << "auth_data was found in cookies, checking...";

            if (auto it = cookie.find(USER_ID_COOKIE_KEY); it != cookie.end()) {
                Wt::Dbo::Transaction transaction(UserDbSession);
                const Wt::Dbo::ptr<NDataBase::TUserDb> rowIt
                    = UserDbSession.find<NDataBase::TUserDb>()
                        .where("user_id = ?")
                        .bind(std::stoi(it->second));

                std::string message;
                if (rowIt->getHash() == cookie.find(HASH_COOKIE_KEY)->second) {
                    message = "Nice! You logged in!";
                } else {
                    message = "Hmm.. Haven't u changed cookies..?";
                }

                transaction.commit();

                root()->addWidget(
                    std::make_unique<Wt::WText>(std::move(message))
                );
            } else {
                root()->addWidget(
                    std::make_unique<Wt::WText>("Hmm.. are you hacker?")
                );
            }
        }
    }
}
