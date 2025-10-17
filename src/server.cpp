#include <cpprest/http_listener.h>
#include <cpprest/json.h>
#include <cpprest/astreambuf.h>
#include <cpprest/filestream.h>
#include <iostream>
#include <thread>
#include <chrono>
#include <atomic>
#include <iomanip>
#include <sstream>
#include <mutex>

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;
using namespace concurrency::streams;

// ===== SISTEMA DE LOGGING =====

class Logger {
private:
    std::mutex log_mutex_;
    
    std::string get_current_time() {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
            now.time_since_epoch()) % 1000;
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
        ss << "." << std::setfill('0') << std::setw(3) << ms.count();
        return ss.str();
    }
    
    void log_with_color(const std::string& level, const std::string& message, const std::string& color_code) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        std::cout << "\033[" << color_code << "m[" << get_current_time() << "] [" << level << "]\033[0m " << message << std::endl;
    }

public:
    void info(const std::string& message) {
        log_with_color("INFO", message, "32"); // Verde
    }
    
    void warning(const std::string& message) {
        log_with_color("WARN", message, "33"); // Amarelo
    }
    
    void error(const std::string& message) {
        log_with_color("ERROR", message, "31"); // Vermelho
    }
    
    void access(const http_request& request, const status_code& response_code, long long duration_ms = -1) {
        std::lock_guard<std::mutex> lock(log_mutex_);
        
        auto method = request.method();
        auto path = utility::conversions::to_utf8string(request.request_uri().path());
        auto query = utility::conversions::to_utf8string(request.request_uri().query());
        auto client_addr = utility::conversions::to_utf8string(request.remote_address());
        
        std::stringstream ss;
        ss << "\033[36m" << client_addr << "\033[0m"; // Ciano para IP
        ss << " \"" << method << " " << path;
        if (!query.empty()) {
            ss << "?" << query;
        }
        ss << "\" ";
        
        // Código de status colorido
        if (response_code >= 200 && response_code < 300) {
            ss << "\033[32m" << response_code << "\033[0m"; // Verde para sucesso
        } else if (response_code >= 400 && response_code < 500) {
            ss << "\033[33m" << response_code << "\033[0m"; // Amarelo para client error
        } else {
            ss << "\033[31m" << response_code << "\033[0m"; // Vermelho para server error
        }
        
        if (duration_ms >= 0) {
            ss << " " << duration_ms << "ms";
        }
        
        std::cout << "[" << get_current_time() << "] [ACCESS] " << ss.str() << std::endl;
    }
};

// Logger global
Logger logger;

// Métricas globais
std::atomic<int> total_requests{0};
std::atomic<int> active_requests{0};

// ===== FUNÇÕES AUXILIARES =====

// Calcula duração entre dois time_points
long long calculate_duration_ms(
    const std::chrono::time_point<std::chrono::high_resolution_clock>& start,
    const std::chrono::time_point<std::chrono::high_resolution_clock>& end) {
    return std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
}

// Wrapper para logging de responses
void log_and_reply(
    http_request request, 
    status_code code, 
    const json::value& response_data, 
    const std::chrono::time_point<std::chrono::high_resolution_clock>& start_time) {
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = calculate_duration_ms(start_time, end_time);
    
    logger.access(request, code, duration_ms);
    request.reply(code, response_data);
}

// ===== HANDLERS COM LOGGING =====

// Simula operação assíncrona corretamente
pplx::task<json::value> simulate_async_operation() {
    return pplx::create_task([]() -> json::value {
        json::value result;
        result[U("message")] = json::value::string(U("Operação assíncrona concluída!"));
        result[U("timestamp")] = json::value::number(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        return result;
    });
}

// Manipulador GET assíncrono com logging
void handle_get(http_request request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    total_requests++;
    active_requests++;
    
    logger.info("Nova requisição GET para /api - Requisições ativas: " + std::to_string(active_requests));
    
    simulate_async_operation()
    .then([request, start_time](pplx::task<json::value> previous_task) {
        try {
            json::value response_data = previous_task.get();
            log_and_reply(request, status_codes::OK, response_data, start_time);
        }
        catch (const std::exception& e) {
            json::value error;
            error[U("error")] = json::value::string(utility::conversions::to_string_t(e.what()));
            logger.error("Erro em handle_get: " + std::string(e.what()));
            log_and_reply(request, status_codes::InternalError, error, start_time);
        }
    })
    .then([start_time](pplx::task<void> task) {
        active_requests--;
        try {
            task.get();
        }
        catch (const std::exception& e) {
            logger.error("Erro na chain de tasks: " + std::string(e.what()));
        }
    });
}

// Handler com delay
void handle_get_with_delay(http_request request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    total_requests++;
    active_requests++;
    
    logger.info("Requisição para /api/delay - Requisições ativas: " + std::to_string(active_requests));
    
    pplx::create_task([]() { return true; })
    .then([request, start_time](bool) {
        json::value response_data;
        response_data[U("message")] = json::value::string(U("Resposta após operação assíncrona"));
        response_data[U("timestamp")] = json::value::number(
            std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()
            ).count()
        );
        log_and_reply(request, status_codes::OK, response_data, start_time);
    })
    .then([start_time](pplx::task<void> task) {
        active_requests--;
        try {
            task.get();
        }
        catch (const std::exception& e) {
            logger.error("Erro no handler com delay: " + std::string(e.what()));
        }
    });
}

// Handler para I/O
void handle_get_async_io(http_request request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    total_requests++;
    active_requests++;
    
    logger.info("Requisição para /api/io - Requisições ativas: " + std::to_string(active_requests));
    
    pplx::create_task([]() -> json::value {
        json::value result;
        result[U("status")] = json::value::string(U("Operação de I/O simulada"));
        result[U("data_size")] = json::value::number(1024);
        return result;
    })
    .then([request, start_time](json::value result) {
        log_and_reply(request, status_codes::OK, result, start_time);
    })
    .then([start_time](pplx::task<void> task) {
        active_requests--;
        try {
            task.get();
        }
        catch (const std::exception& e) {
            logger.error("Erro em I/O assíncrono: " + std::string(e.what()));
        }
    });
}

// Handler para parâmetros
void handle_get_with_params(http_request request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    total_requests++;
    active_requests++;
    
    auto query_params = uri::split_query(request.request_uri().query());
    logger.info("Requisição para /api/params com " + std::to_string(query_params.size()) + " parâmetros");
    
    pplx::create_task([query_params]() -> json::value {
        json::value result;
        result[U("parameters_received")] = json::value::number(query_params.size());
        
        for (const auto& param : query_params) {
            result[param.first] = json::value::string(param.second);
        }
        
        return result;
    })
    .then([request, start_time](json::value result) {
        log_and_reply(request, status_codes::OK, result, start_time);
    })
    .then([start_time](pplx::task<void> task) {
        active_requests--;
        try {
            task.get();
        }
        catch (const std::exception& e) {
            logger.error("Erro ao processar parâmetros: " + std::string(e.what()));
        }
    });
}

// Handler de demonstração assíncrona
void handle_get_async_demo(http_request request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    total_requests++;
    active_requests++;
    
    logger.info("Requisição para /api/async-demo - Thread: " + 
                std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
    
    pplx::create_task([]() -> int {
        logger.info("Task 1 executando em thread: " + 
                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
        return 42;
    })
    .then([](int value) -> std::string {
        logger.info("Task 2 processando valor: " + std::to_string(value) + " em thread: " + 
                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
        return "Processado: " + std::to_string(value);
    })
    .then([request, start_time](std::string processed_value) -> json::value {
        logger.info("Task 3 finalizando em thread: " + 
                   std::to_string(std::hash<std::thread::id>{}(std::this_thread::get_id())));
        
        json::value result;
        result[U("message")] = json::value::string(utility::conversions::to_string_t(processed_value));
        result[U("thread_demo")] = json::value::string(U("Verdadeira assincronicidade - diferentes threads"));
        return result;
    })
    .then([request, start_time](json::value result) {
        log_and_reply(request, status_codes::OK, result, start_time);
    })
    .then([start_time](pplx::task<void> task) {
        active_requests--;
        try {
            task.get();
            logger.info("Requisição async-demo completada com sucesso");
        }
        catch (const std::exception& e) {
            logger.error("Erro na demonstração assíncrona: " + std::string(e.what()));
        }
    });
}

// Handler de estatísticas
void handle_stats(http_request request) {
    auto start_time = std::chrono::high_resolution_clock::now();
    
    json::value stats;
    stats[U("total_requests")] = json::value::number(total_requests.load());
    stats[U("active_requests")] = json::value::number(active_requests.load());
    stats[U("status")] = json::value::string(U("online"));
    stats[U("timestamp")] = json::value::number(
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::system_clock::now().time_since_epoch()
        ).count()
    );
    
    auto end_time = std::chrono::high_resolution_clock::now();
    auto duration_ms = calculate_duration_ms(start_time, end_time);
    
    logger.access(request, status_codes::OK, duration_ms);
    request.reply(status_codes::OK, stats);
}

// ===== MAIN =====

int main() {
    // Configuração do listener
    http_listener listener(U("http://0.0.0.0:8080"));
    
    // Handler principal com routing
    listener.support(methods::GET, [](http_request request) {
        auto path = request.request_uri().path();
        
        if (path == U("/api") || path == U("/api/")) {
            handle_get(request);
        }
        else if (path == U("/api/delay")) {
            handle_get_with_delay(request);
        }
        else if (path == U("/api/io")) {
            handle_get_async_io(request);
        }
        else if (path == U("/api/params")) {
            handle_get_with_params(request);
        }
        else if (path == U("/api/async-demo")) {
            handle_get_async_demo(request);
        }
        else if (path == U("/api/stats")) {
            handle_stats(request);
        }
        else {
            auto start_time = std::chrono::high_resolution_clock::now();
            json::value response;
            response[U("error")] = json::value::string(U("Endpoint não encontrado"));
            response[U("available_endpoints")] = json::value::string(
                U("/api, /api/delay, /api/io, /api/params, /api/async-demo, /api/stats")
            );
            logger.warning("Endpoint não encontrado: " + utility::conversions::to_utf8string(path));
            log_and_reply(request, status_codes::NotFound, response, start_time);
        }
    });

    try {
        listener.open().wait();
        logger.info("🎯 Servidor REST verdadeiramente assíncrono iniciado!");
        logger.info("📍 Endpoint: http://localhost:8080");
        logger.info("📋 Endpoints disponíveis:");
        logger.info("   GET http://localhost:8080/api");
        logger.info("   GET http://localhost:8080/api/delay");
        logger.info("   GET http://localhost:8080/api/io");
        logger.info("   GET http://localhost:8080/api/params?param1=value1&param2=value2");
        logger.info("   GET http://localhost:8080/api/async-demo (demonstra threads)");
        logger.info("   GET http://localhost:8080/api/stats (estatísticas)");
        logger.info("⏹️  Pressione Enter para parar o servidor...");
        
        std::string line;
        std::getline(std::cin, line);
        
        logger.info("🛑 Parando servidor...");
        listener.close().wait();
        logger.info("✅ Servidor parado com sucesso!");
    }
    catch (const std::exception &e) {
        logger.error("❌ Erro: " + std::string(e.what()));
        return 1;
    }

    return 0;
}