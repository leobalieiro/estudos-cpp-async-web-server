#include "../system/Router.hpp"
#include "../controllers/ApiController.hpp"
#include "../controllers/UserController.hpp"
#include "../controllers/StatsController.hpp"
#include "../controllers/HealthController.hpp"
#include <iostream>

System::Routes::Router setup_routes()
{
    System::Routes::Router router;

    std::cout << "🛣️  Configurando rotas OTIMIZADAS..." << std::endl;

    // Rotas de alta performance
    router.get("/", [](http_request request)
               { return StatsController::trackRequest(request, ApiController::handleFast(request)); });

    router.get("/api", [](http_request request)
               { return StatsController::trackRequest(request, ApiController::handleApi(request)); });

    router.get("/api/", [](http_request request)
               { return StatsController::trackRequest(request, ApiController::handleApi(request)); });

    router.get("/api/fast", [](http_request request)
               { return StatsController::trackRequest(request, ApiController::handleFast(request)); });

    router.get("/api/async-demo", [](http_request request)
               { return StatsController::trackRequest(request, ApiController::handleAsyncDemo(request)); });

    router.get("/api/health", [](http_request request)
               { return StatsController::trackRequest(request, HealthController::healthCheck(request)); });

    router.get("/api/stats", [](http_request request)
               { return StatsController::getStats(request); });

    router.get("/api/delay", [](http_request request)
               { return StatsController::trackRequest(request, ApiController::handleDelay(request)); });

    router.get("/api/params", [](http_request request)
               { return StatsController::trackRequest(request, ApiController::handleParams(request)); });

    router.get("/api/users", [](http_request request)
               { return StatsController::trackRequest(request, UserController::getAllUsers(request)); });

    // 404 otimizado
    router.set_not_found_handler([](http_request request)
                                 {
        StatsController::incrementRequest();
        
        json::value response;
        response[U("error")] = json::value::string(U("Rota não encontrada"));
        response[U("code")] = json::value::number(404);
        
        request.reply(status_codes::NotFound, response);
        
        StatsController::decrementRequest();
        return pplx::task_from_result(); });

    std::cout << "✅ Rotas otimizadas registradas:" << std::endl;
    for (const auto &route : router.get_routes())
    {
        std::cout << "   " << route << std::endl;
    }

    return router;
}