#include <server.hpp>
#include <file_server.hpp>

#include <thread>
#include <iostream>

int main()
{
    auto num_threads = std::max(4U, std::thread::hardware_concurrency());

    Server server(8080, num_threads);

    server.set_static_dir("../public");

    server.get("/ping", [](const Request&) {
        return Response::ok("pong");
    });

    server.get("/info", [num_threads](const Request&) {
        std::ostringstream json;
        json << "{\n"
             << "  \"server\": \"Server From Scratch\",\n"
             << "  \"language\": \"C++17\",\n"
             << "  \"worker_threads\": " << num_threads << "\n"
             << "}";
        return Response::json(json.str());
    });

    server.get("/echo", [](const Request& req) {
        auto it = req.query_params().find("msg");
        auto body = (it == req.query_params().end()) ? "no msg= query param" : it->second;
        return Response::ok(body);
    });

    server.listen();

    return 0;
}