#include <server.hpp>
#include <logger.hpp>

#include <iostream>
#include <stdexcept>
#include <cerrno>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

Server::Server(int port, size_t num_thread)
    : port_(port), server_fd_(-1), thread_pool_(num_thread)
{
    setup_socket();
    LOG_INFO("server started at port ", port, " with ", num_thread, " worker threads");
}

Server::~Server()
{
    if (server_fd_ >= 0)
    {
        close(server_fd_);
    }
}

void Server::setup_socket()
{
    if ((server_fd_ = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        throw std::runtime_error("socket() failed: " + std::string(strerror(errno)));
    }

    int opt = 1;
    if (setsockopt(server_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0)
    {
        throw std::runtime_error("setsockopt(SO_REUSEADDR) failed: " + std::string(strerror(errno)));
    }

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(static_cast<uint16_t>(port_));

    if (bind(server_fd_, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) < 0)
    {
        throw std::runtime_error("bind() failed on port " + std::to_string(port_) + ": " + strerror(errno));
    }

    if (::listen(server_fd_, 128) < 0)
    {
        throw std::runtime_error("listen() failed: " + std::string(strerror(errno)));
    }
}

void Server::listen()
{
    LOG_INFO("server accepting connections at http://localhost: ", port_);

    while (true)
    {
        sockaddr_in client_addr{};
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd_, reinterpret_cast<sockaddr *>(&client_addr), &client_len);
        if (client_fd < 0)
        {
            LOG_ERROR("accept() failed: ", strerror(errno));
            continue;
        }

        char ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
        LOG_INFO("connection ", ip, ":", ntohs(client_addr.sin_port));

        thread_pool_.enqueue([this, client_fd]()
                             {
            handle_client(client_fd);
            close(client_fd); });
    }
}

void Server::handle_client(int client_fd)
{
    struct timeval tv{};
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

    Request req;
    if (!RequestParser::parse(client_fd, req))
    {
        send_response(client_fd, Response::bad_request("Malformed or incomplete request"));
        return;
    }

    LOG_INFO("request ", req.method(), " ", req.path());

    Response res = router_.dispatch(req);
    send_response(client_fd, res);
}

void Server::send_response(int client_fd, const Response &res)
{
    std::string raw = res.serialize();
    size_t total = 0;

    while (total < raw.size())
    {
        ssize_t sent = send(client_fd, raw.data() + total, raw.size() - total, MSG_NOSIGNAL);
        if (sent <= 0)
        {
            LOG_ERROR("Some problem occured while sending response code ", res.status_code());
            return;
        }
        total += static_cast<size_t>(sent);
    }
}