#include <cpprest/http_listener.h>
#include <iostream>
#include "system/Router.hpp"
#include "routes/web.cpp"

using namespace web;
using namespace web::http;
using namespace web::http::experimental::listener;

int main() {
    std::cout << "🚀 Iniciando servidor C++ REST com tracking..." << std::endl;
    
    try {
        // Configuração
        http_listener_config config;
        config.set_timeout(std::chrono::seconds(30));
        
        http_listener listener(U("http://0.0.0.0:8080"), config);
        
        // Configura rotas
        std::cout << "🛣️  Configurando sistema de rotas..." << std::endl;
        auto router = setup_routes();
        
        // Handler principal com logging
        listener.support(methods::GET, [&router](http_request request) {
            auto path = utility::conversions::to_utf8string(request.request_uri().path());
            auto query = utility::conversions::to_utf8string(request.request_uri().query());
            
            std::cout << "📨 GET " << path;
            if (!query.empty()) {
                std::cout << "?" << query;
            }
            std::cout << std::endl;
            
            router.handle_request(request);
        });
        
        listener.support(methods::POST, [&router](http_request request) {
            auto path = utility::conversions::to_utf8string(request.request_uri().path());
            std::cout << "📨 POST " << path << std::endl;
            router.handle_request(request);
        });

        // Iniciar servidor
        std::cout << "🔧 Iniciando listener..." << std::endl;
        listener.open().wait();
        
        std::cout << "========================================" << std::endl;
        std::cout << "✅ Servidor C++ REST Iniciado!" << std::endl;
        std::cout << "📍 http://localhost:8080" << std::endl;
        std::cout << "📊 Sistema de tracking: ATIVADO" << std::endl;
        std::cout << "📋 Endpoints disponíveis:" << std::endl;
        std::cout << "   GET /api" << std::endl;
        std::cout << "   GET /api/health" << std::endl;
        std::cout << "   GET /api/stats          ← Ver contagem de requests" << std::endl;
        std::cout << "   GET /api/users" << std::endl;
        std::cout << "   GET /api/delay" << std::endl;
        std::cout << "   GET /api/params?name=valor" << std::endl;
        std::cout << "⏹️  Pressione Enter para parar..." << std::endl;
        std::cout << "========================================" << std::endl;
        
        std::string line;
        std::getline(std::cin, line);
        
        std::cout << "🛑 Parando servidor..." << std::endl;
        listener.close().wait();
        std::cout << "✅ Servidor parado!" << std::endl;
    }
    catch (const std::exception &e) {
        std::cerr << "💥 ERRO CRÍTICO: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}