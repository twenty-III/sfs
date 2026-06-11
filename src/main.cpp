#include <server.hpp>
#include <file_server.hpp>

#include <thread>
#include <iostream>

int main()
{
    Server server(8080, std::thread::hardware_concurrency());

    server.set_static_dir("../public");

    server.listen();

    return 0;
}