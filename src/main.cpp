#include <server.hpp>
#include <logger.hpp>

#include <thread>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <vector>
#include <mutex>

int main()
{
    auto num_threads = std::max(4U, std::thread::hardware_concurrency());

    int port = 8080;
    if (const char *env_port = std::getenv("PORT"))
    {
        try
        {
            port = std::stoi(env_port);
        }
        catch (...)
        {
            LOG_WARN("invalid PORT env value '", env_port, "', falling back to 8080\n");
        }
    }

    Server server(port, num_threads);

    server.set_static_dir("../public");

    server.get("/ping", [](const Request &)
               { return Response::ok("pong"); });

    server.get("/info", [num_threads, port](const Request &)
               {
        std::ostringstream json;
        json << "{\n"
             << "  \"server\": \"Server From Scratch\",\n"
             << "  \"language\": \"C++17\",\n"
             << "  \"port\": " << port << ",\n"
             << "  \"worker_threads\": " << num_threads << "\n"
             << "}";
        return Response::json(json.str()); });

    server.get("/echo", [](const Request &req)
               {
        auto it = req.query_params().find("msg");
        auto body = (it == req.query_params().end()) ? "no msg= query param" : it->second;
        return Response::ok(body); });

    static std::vector<std::string> tasks = {"Learn C++", "Build a server"};
    static std::mutex tasks_mutex;

    server.get("/tasks", [](const Request &) {
        std::lock_guard<std::mutex> lock(tasks_mutex);
        std::ostringstream json;
        json << "[";
        for (size_t i = 0; i < tasks.size(); ++i) {
            json << "\"" << tasks[i] << "\"";
            if (i < tasks.size() - 1) json << ", ";
        }
        json << "]";
        return Response::json(json.str());
    });

    server.post("/tasks", [](const Request &req) {
        std::string task = req.body();
        if (task.empty()) {
            return Response::bad_request("Empty task");
        }
        std::lock_guard<std::mutex> lock(tasks_mutex);
        tasks.push_back(task);
        Response res;
        res.set_status(201);
        res.set_body("Task added");
        return res;
    });

    server.listen();

    return 0;
}
