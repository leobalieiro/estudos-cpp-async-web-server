#pragma once
#ifndef HEALTHCONTROLLER_HPP
#define HEALTHCONTROLLER_HPP

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <pplx/pplxtasks.h>
#include <chrono>

using namespace web;
using namespace web::http;

class HealthController
{
public:
    static pplx::task<void> healthCheck(http_request request)
    {
        // Resposta instantânea
        json::value health;
        health[U("status")] = json::value::string(U("healthy"));
        health[U("service")] = json::value::string(U("C++ High-Performance Async REST API"));
        health[U("performance")] = json::value::string(U("non-blocking-io"));

        request.reply(status_codes::OK, health);
        return pplx::task_from_result();
    }
};

#endif