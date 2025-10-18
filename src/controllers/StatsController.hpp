#pragma once
#ifndef STATSCONTROLLER_HPP
#define STATSCONTROLLER_HPP

#include <cpprest/http_msg.h>
#include <cpprest/json.h>
#include <pplx/pplxtasks.h>
#include <atomic>
#include <chrono>

using namespace web;
using namespace web::http;

class StatsController
{
private:
    static std::atomic<int> total_requests;
    static std::atomic<int> active_requests;
    static std::chrono::time_point<std::chrono::system_clock> start_time;

public:
    static void initialize()
    {
        if (total_requests == 0)
        {
            start_time = std::chrono::system_clock::now();
        }
    }

    static void incrementRequest()
    {
        total_requests++;
        active_requests++;
    }

    static void decrementRequest()
    {
        if (active_requests > 0)
        {
            active_requests--;
        }
    }

    static pplx::task<void> getStats(http_request request)
    {
        return pplx::create_task([]()
                                 {
            initialize();
            
            auto now = std::chrono::system_clock::now();
            auto uptime = std::chrono::duration_cast<std::chrono::seconds>(now - start_time);
            double requests_per_second = uptime.count() > 0 ? 
                static_cast<double>(total_requests.load()) / uptime.count() : 0.0;
            
            json::value stats;
            stats[U("total_requests")] = json::value::number(total_requests.load());
            stats[U("active_requests")] = json::value::number(active_requests.load());
            stats[U("uptime_seconds")] = json::value::number(uptime.count());
            stats[U("requests_per_second")] = json::value::number(requests_per_second);
            stats[U("status")] = json::value::string(U("online"));
            stats[U("performance")] = json::value::string(U("optimized-high-load"));
            
            return stats; })
            .then([request](json::value response_data)
                  { request.reply(status_codes::OK, response_data); });
    }

    static pplx::task<void> trackRequest(http_request request, pplx::task<void> handler_task)
    {
        incrementRequest();

        // ✅ Agora usando o parâmetro 'request'
        auto method = request.method();
        auto path = request.request_uri().path();

        return handler_task.then([method, path](pplx::task<void> task)
                                 {
        decrementRequest();

        try {
            task.get();
            // Log de sucesso
            std::cout << "✅ " << method << " " << path << " - Sucesso" << std::endl;
        }
        catch (const std::exception& e) {
            std::cerr << "❌ " << method << " " << path << " - Erro: " << e.what() << std::endl;
            throw;
        } });
    }
};

std::atomic<int> StatsController::total_requests{0};
std::atomic<int> StatsController::active_requests{0};
std::chrono::time_point<std::chrono::system_clock> StatsController::start_time;

#endif