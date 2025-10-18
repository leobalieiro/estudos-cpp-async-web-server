#pragma once
#ifndef USERCONTROLLER_HPP
#define USERCONTROLLER_HPP

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <pplx/pplxtasks.h>
#include <vector>
#include <map>
#include <sstream>
#include <cpprest/asyncrt_utils.h> // Para utility::conversions

using namespace web;
using namespace web::http;

class UserController
{
private:
    static std::map<utility::string_t, json::value> userDatabase;
    static std::atomic<bool> cache_initialized;
    static json::value cached_users_response;

    static void initializeDatabase()
    {
        if (userDatabase.empty())
        {
            for (int i = 1; i <= 100; i++)
            {
                json::value user;

                // Converter número para string de forma compatível
                utility::string_t id_str = utility::conversions::to_string_t(std::to_string(i));

                std::basic_stringstream<utility::char_t> name_ss;
                name_ss << U("Usuário ") << i;
                utility::string_t name_str = name_ss.str();

                std::basic_stringstream<utility::char_t> email_ss;
                email_ss << U("user") << i << U("@email.com");
                utility::string_t email_str = email_ss.str();

                user[U("id")] = json::value::string(id_str);
                user[U("name")] = json::value::string(name_str);
                user[U("email")] = json::value::string(email_str);

                userDatabase[id_str] = user;
            }
        }
    }

    static void initializeCache()
    {
        if (!cache_initialized.exchange(true))
        {
            initializeDatabase();

            // Criar resposta em cache
            cached_users_response[U("users")] = json::value::array();
            int index = 0;
            for (const auto &user_pair : userDatabase)
            {
                cached_users_response[U("users")][index++] = user_pair.second;
            }
            cached_users_response[U("total_users")] = json::value::number(userDatabase.size());
            cached_users_response[U("cached")] = json::value::boolean(true);
            cached_users_response[U("performance")] = json::value::string(U("optimized"));
        }
    }

public:
    static pplx::task<void> getAllUsers(http_request request)
    {
        // Resposta do cache - sem processamento
        initializeCache();

        request.reply(status_codes::OK, cached_users_response);
        return pplx::task_from_result();
    }
};

// Inicializar variáveis estáticas
std::map<utility::string_t, json::value> UserController::userDatabase;
std::atomic<bool> UserController::cache_initialized{false};
json::value UserController::cached_users_response;

#endif