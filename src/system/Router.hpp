#pragma once
#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <functional>
#include <unordered_map>
#include <vector>
#include <regex>
#include <memory>
#include <iostream>

using namespace web;
using namespace web::http;

namespace System
{
    namespace Routes
    {

        using AsyncHandler = std::function<pplx::task<void>(http_request)>;

        struct Route
        {
            std::string method;
            std::string path;
            AsyncHandler handler;
            std::regex path_regex;
            std::vector<std::string> param_names;

            Route(const std::string &m, const std::string &p, AsyncHandler h)
                : method(m), path(p), handler(h)
            {
                compile_regex();
            }

        private:
            void compile_regex()
            {
                std::string regex_path = path;

                // Substitui {param} por ([^/]+) e coleta nomes dos parâmetros
                std::string::size_type pos = 0;
                param_names.clear();

                while ((pos = regex_path.find('{', pos)) != std::string::npos)
                {
                    std::string::size_type end_pos = regex_path.find('}', pos);
                    if (end_pos == std::string::npos)
                    {
                        break;
                    }

                    // Extrai nome do parâmetro
                    std::string param_name = regex_path.substr(pos + 1, end_pos - pos - 1);
                    param_names.push_back(param_name);

                    // Substitui por grupo de captura regex
                    regex_path.replace(pos, end_pos - pos + 1, "([^/]+)");
                    pos += 8; // Avança após a substituição
                }

                // Adiciona âncoras de início e fim
                regex_path = "^" + regex_path + "$";

                try
                {
                    path_regex = std::regex(regex_path);
                }
                catch (const std::regex_error &e)
                {
                    std::cerr << "❌ Erro regex na rota '" << path << "': " << e.what() << std::endl;
                    throw;
                }
            }
        };

        class Router
        {
        private:
            std::vector<std::shared_ptr<Route>> routes;
            AsyncHandler not_found_handler;
            AsyncHandler error_handler;

        public:
            Router()
            {
                not_found_handler = [](http_request request)
                {
                    json::value response;
                    response[U("error")] = json::value::string(U("Endpoint não encontrado"));
                    response[U("code")] = json::value::number(404);
                    request.reply(status_codes::NotFound, response);
                    return pplx::task_from_result();
                };

                error_handler = [](http_request request)
                {
                    json::value response;
                    response[U("error")] = json::value::string(U("Erro interno do servidor"));
                    response[U("code")] = json::value::number(500);
                    request.reply(status_codes::InternalError, response);
                    return pplx::task_from_result();
                };
            }

            void get(const std::string &path, AsyncHandler handler)
            {
                std::cout << "📝 Registrando rota: GET " << path << std::endl;
                try
                {
                    routes.push_back(std::shared_ptr<Route>(new Route("GET", path, handler)));
                }
                catch (const std::exception &e)
                {
                    std::cerr << "❌ Erro ao registrar rota '" << path << "': " << e.what() << std::endl;
                    throw;
                }
            }

            void post(const std::string &path, AsyncHandler handler)
            {
                std::cout << "📝 Registrando rota: POST " << path << std::endl;
                routes.push_back(std::shared_ptr<Route>(new Route("POST", path, handler)));
            }

            void put(const std::string &path, AsyncHandler handler)
            {
                std::cout << "📝 Registrando rota: PUT " << path << std::endl;
                routes.push_back(std::shared_ptr<Route>(new Route("PUT", path, handler)));
            }

            void del(const std::string &path, AsyncHandler handler)
            {
                std::cout << "📝 Registrando rota: DELETE " << path << std::endl;
                routes.push_back(std::shared_ptr<Route>(new Route("DELETE", path, handler)));
            }

            void set_not_found_handler(AsyncHandler handler)
            {
                not_found_handler = handler;
            }

            void set_error_handler(AsyncHandler handler)
            {
                error_handler = handler;
            }

            // pplx::task<void> handle_request(http_request request)
            // {
            //     return pplx::create_task([this, request]()
            //                              {
            //         try {
            //             auto method = request.method();
            //             auto path = utility::conversions::to_utf8string(request.request_uri().path());

            //             std::cout << "🔍 Procurando rota: " << method << " " << path << std::endl;

            //             for (const auto& route : routes) {
            //                 if (route->method == method) {
            //                     std::smatch matches;
            //                     if (std::regex_match(path, matches, route->path_regex)) {
            //                         std::cout << "✅ Rota encontrada: " << route->path << std::endl;
            //                         return route->handler(request);
            //                     }
            //                 }
            //             }

            //             std::cout << "❌ Rota não encontrada: " << method << " " << path << std::endl;
            //             return not_found_handler(request);
            //         }
            //         catch (const std::exception& e) {
            //             std::cerr << "💥 Erro no roteamento: " << e.what() << std::endl;
            //             return error_handler(request);
            //         } });
            // }
            // No método handle_request, adicionar proteção:
            pplx::task<void> handle_request(http_request request)
            {
                return pplx::create_task([this, request]()
                                         {
        try {
            // Limite de concorrência - proteger o servidor
            static std::atomic<int> current_concurrency{0};
            const int MAX_CONCURRENCY = 5000; // Ajustável
            
            if (current_concurrency >= MAX_CONCURRENCY) {
                json::value response;
                response[U("error")] = json::value::string(U("Servidor sobrecarregado"));
                response[U("code")] = json::value::number(503);
                response[U("message")] = json::value::string(U("Tente novamente em alguns segundos"));
                request.reply(status_codes::ServiceUnavailable, response);
                return pplx::task_from_result();
            }
            
            current_concurrency++;
            
            auto method = request.method();
            auto path = utility::conversions::to_utf8string(request.request_uri().path());
            
            // Busca otimizada
            for (const auto& route : routes) {
                if (route->method == method) {
                    std::smatch matches;
                    if (std::regex_match(path, matches, route->path_regex)) {
                        return route->handler(request)
                            .then([current_concurrency_ptr = &current_concurrency](pplx::task<void> task) {
                                (*current_concurrency_ptr)--;
                                try {
                                    task.get();
                                } catch (const std::exception& e) {
                                    std::cerr << "Erro em handler: " << e.what() << std::endl;
                                }
                            });
                    }
                }
            }
            
            current_concurrency--;
            return not_found_handler(request);
        }
        catch (const std::exception& e) {
            std::cerr << "💥 Erro no roteamento: " << e.what() << std::endl;
            return error_handler(request);
        } });
            }

            std::vector<std::string> get_routes() const
            {
                std::vector<std::string> route_list;
                for (const auto &route : routes)
                {
                    route_list.push_back(route->method + " " + route->path);
                }
                return route_list;
            }
        };

    }
}

#endif