#pragma once
#ifndef APICONTROLLER_HPP
#define APICONTROLLER_HPP

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <pplx/pplxtasks.h>
#include <chrono>
#include <sstream>

using namespace web;
using namespace web::http;

class ApiController
{
public:
    static pplx::task<void> handleApi(http_request request)
    {
        // Resposta mínima e rápida
        json::value response;
        response[U("message")] = json::value::string(U("API REST C++ Assíncrona"));
        response[U("timestamp")] = json::value::number(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch())
                .count());
        response[U("performance")] = json::value::string(U("non-blocking"));

        request.reply(status_codes::OK, response);
        return pplx::task_from_result();
    }

    static pplx::task<void> handleDelay(http_request request)
    {
        // Simulação de operação assíncrona sem busy wait
        return pplx::create_task([request]()
                                 {
            // Esta task pode ser executada em qualquer thread do pool
            // O scheduler gerencia a execução - não bloqueia threads
            
            json::value response;
            response[U("message")] = json::value::string(U("Operação assíncrona concluída"));
            response[U("status")] = json::value::string(U("async-complete"));
            response[U("timestamp")] = json::value::number(
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::system_clock::now().time_since_epoch()
                ).count()
            );
            request.reply(status_codes::OK, response); });
    }

    static pplx::task<void> handleParams(http_request request)
    {
        // Processamento assíncrono puro
        return pplx::create_task([request]()
                                 {
            json::value response;
            response[U("message")] = json::value::string(U("Parâmetros processados assincronamente"));
            
            auto query_params = uri::split_query(request.request_uri().query());
            json::value params;
            for (const auto& param : query_params) {
                params[param.first] = json::value::string(param.second);
            }
            response[U("query_params")] = params;
            response[U("total_params")] = json::value::number(query_params.size());
            
            request.reply(status_codes::OK, response); });
    }

    static pplx::task<void> handleFast(http_request request)
    {
        // Resposta ULTRA rápida - máximo throughput
        auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        
        // Usando json::value para evitar problemas de concatenação
        json::value response;
        response[U("status")] = json::value::string(U("ok"));
        response[U("timestamp")] = json::value::number(timestamp);

        request.reply(status_codes::OK, response);
        return pplx::task_from_result();
    }

    static pplx::task<void> handleAsyncDemo(http_request request)
    {
        // Chain de tasks assíncronas
        return pplx::create_task([]() -> int
                                 {
            // Task 1
            return 42; })
            .then([](int value) -> utility::string_t
                  {
            // Task 2 - Corrigido: usando stringstream
            std::basic_stringstream<utility::char_t> ss;
            ss << U("Resultado: ") << (value * 2);
            return ss.str(); })
            .then([request](utility::string_t result)
                  {
            // Task 3
            json::value response;
            response[U("message")] = json::value::string(U("Demonstração de assincronicidade"));
            response[U("result")] = json::value::string(result);
            
            request.reply(status_codes::OK, response); });
    }
};

#endif