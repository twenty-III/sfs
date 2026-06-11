#pragma once

#include <request.hpp>
#include <response.hpp>

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

using Handler = std::function<Response(const Request &)>;

class Router
{
public:
    void get(std::string path, Handler handler)
    {
        add_route("GET", std::move(path), std::move(handler));
    }

    void put(std::string path, Handler handler)
    {
        add_route("PUT", std::move(path), std::move(handler));
    }

    void post(std::string path, Handler handler)
    {
        add_route("POST", std::move(path), std::move(handler));
    }

    void del(std::string path, Handler handler)
    {
        add_route("DELETE", std::move(path), std::move(handler));
    }

    void set_static_dir(std::string path)
    {
        static_dir_ = std::move(path);
    }

    Response dispatch(Request &req) const;

private:
    struct Route
    {
        std::string method;
        std::string pattern;
        std::vector<std::string> params;
        Handler handler;
    };

    std::string static_dir_;

    std::vector<Route> routes_;

    void add_route(std::string method, std::string path, Handler handler);

    static bool match(const Route &route, const std::string &path, std::unordered_map<std::string, std::string> &out_params);
};