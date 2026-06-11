#pragma once

#include <string>
#include <router.hpp>
#include <thread_pool.hpp>

class Server
{
public:
    explicit Server(int port, size_t num_threads);
    ~Server();

    Server(const Server &) = delete;
    Server &operator=(const Server &) = delete;

    void set_static_dir(std::string path)
    {
        router_.set_static_dir(std::move(path));
    }

    void get(std::string path, Handler handler)
    {
        router_.get(std::move(path), std::move(handler));
    }

    void put(std::string path, Handler handler)
    {
        router_.put(std::move(path), std::move(handler));
    }

    void post(std::string path, Handler handler)
    {
        router_.post(std::move(path), std::move(handler));
    }

    void del(std::string path, Handler handler)
    {
        router_.del(std::move(path), std::move(handler));
    }

    void listen();

private:
    int port_;
    int server_fd_;
    Router router_;
    ThreadPool thread_pool_;

    void setup_socket();
    void handle_client(int client_fd);
    void send_response(int client_fd, const Response &res);
};