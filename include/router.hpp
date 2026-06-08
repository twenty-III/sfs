#pragma once

#include <request.hpp>
#include <response.hpp>

#include <string>
#include <vector>
#include <functional>
#include <unordered_map>

using Handler = std::function<Response(const Request&)>;

struct Route {
    std::string method;
    std::string pattern;
    std::vector<std::string> params;
    Handler handler;
};

class Router {
public:
    void get(const std::string& path, Handler handler);
    void put(const std::string& path, Handler handler);
    void post(const std::string& path, Handler handler);
    void del(const std::string& path, Handler handler);

    Response dispatch(Request& req) const;

private:
    std::vector<Route> routes_;
    
    void add_route(const std::string& method, const std::string& path, Handler handler);

    static bool match(const Route& route, const std::string& path, std::unordered_map<std::string, std::string>& out_params);
};