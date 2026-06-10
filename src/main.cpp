#include <server.hpp>
#include <logger.hpp>

#include <thread>

int main()
{
    auto num_threads = std::thread::hardware_concurrency();

    Server server(8080, num_threads);

    server.get("/", [](const Request&) {
        return Response::ok("Hello, Im mohit dubey");
    });

    server.listen();

    return 0;
}