#include <server.hpp>

#include <iostream>
#include <thread>
#include <sstream>

int main() {
    size_t num_threads = std::max(4U, std::thread::hardware_concurrency());

    Server server(8080, static_cast<int>(num_threads));

    server.get("/", [](const Request&) {
        return Response::html(R"html(
            <!DOCTYPE html>
            <html>
            <head><title>MyCppServer</title></head>
            <body>
              <h1>C++ HTTP Server</h1>
              <p>Try these endpoints with curl:</p>
              <pre>
curl http://localhost:8080/ping
curl "http://localhost:8080/echo?msg=hello"
curl http://localhost:8080/info
curl -X POST http://localhost:8080/users -H "Content-Type: application/json" -d '{"name":"Alice"}'
curl http://localhost:8080/users/42
              </pre>
            </body>
            </html>
        )html");
    });

    server.get("/ping", [](const Request&) {
        return Response::ok("pong\n");
    });

    server.get("/echo", [](const Request& req) {
        const auto& params = req.query_params();
        auto it = params.find("msg");
        std::string msg = (it != params.end()) ? it->second : "(no msg=param)";
        return Response::ok("Echo: " + msg + "\n");
    });

    server.get("/info", [num_threads](const Request&) {
        std::ostringstream json;
        json << "{\n"
             << "  \"server\": \"SFS/1.0\",\n"
             << "  \"language\": \"C++17\",\n"
             << "  \"worker_threads\": " << num_threads << "\n"
             << "}";
        return Response::json(json.str());
    });

    server.post("/users", [](const Request& req) {
        std::ostringstream json;
        json << "{\n"
             << "  \"status\": \"created\",\n"
             << "  \"received_bytes\": " << req.body().size() << ",\n"
             << "  \"body\": " << (req.body().empty() ? "null" : "\"" + req.body() + "\"") << "\n"
             << "}";
        Response res = Response::json(json.str());
        res.set_status(201);

        std::cout << res.serialize() << '\n';

        return res;
    });

    server.get("/users/:id", [](const Request& req) {
        const auto& params = req.path_params();
        auto it = params.find("id");
        std::string id = (it != params.end()) ? it->second : "unknown";

        std::ostringstream json;
        json << "{\n"
             << "  \"id\": \"" << id << "\",\n"
             << "  \"name\": \"User " << id << "\"\n"
             << "}";
        return Response::json(json.str());
    });

    server.del("/users/:id", [](const Request& /*req*/) {
        Response res;
        res.set_status(204);
        return res;
    });

    server.listen();

    return 0;
}
