#include <router.hpp>

#include <string>
#include <vector>
#include <sstream>
#include <iostream>
#include <stdexcept>

namespace {
    std::vector<std::string> split_path(const std::string& path) {
        std::vector<std::string> parts;

        std::stringstream ss(path);
        std::string seg;
        while (getline(ss, seg, '/')) {
            if (!seg.empty()) parts.push_back(seg);
        }

        return parts;
    }
}

void Router::add_route(const std::string& method, const std::string& path, Handler handler) {
    Route new_route;
    new_route.method = method;
    new_route.pattern = path;
    new_route.handler = std::move(handler);

    for (const std::string& seg : split_path(path)) {
        if (seg[0] == ':') {
            new_route.params.push_back(seg.substr(1));
        }
    }

    routes_.push_back(new_route);
}

void Router::get(const std::string& path, Handler handler) {
    add_route("GET", path, std::move(handler));
}

void Router::put(const std::string& path, Handler handler) {
    add_route("PUT", path, std::move(handler));
}

void Router::post(const std::string& path, Handler handler) {
    add_route("POST", path, std::move(handler));
}

void Router::del(const std::string& path, Handler handler) {
    add_route("DELETE", path, std::move(handler));
}

bool Router::match(const Route& route, const std::string& path, std::unordered_map<std::string, std::string>& out_params) {
    auto pattern_seg = split_path(route.pattern);
    auto path_seg = split_path(path);

    if (pattern_seg.size() != path_seg.size()) return false;

    size_t param_idx = 0;
    std::unordered_map<std::string, std::string> temp;

    for (size_t i = 0; i < pattern_seg.size(); ++i) {
        if (pattern_seg[i][0] == ':') {
            temp[route.params[param_idx++]] = path_seg[i];
        }
        else if (pattern_seg[i] != path_seg[i]) return false;
    }

    out_params = std::move(temp);
    return true;
}

Response Router::dispatch(Request& req) const {
    for (const auto& route : routes_) {
        if (route.method != req.method()) continue;

        std::unordered_map<std::string, std::string> out_params;

        if (match(route, req.path(), out_params)) {
            req.path_params_ = std::move(out_params);
            try { return route.handler(req); }
            catch (const std::exception& e) {
                std::cerr << "[Router] handler throw: " << e.what() << '\n';
                return Response::server_error(e.what());
            }
        }
    }

    for (const auto& route : routes_) {
        std::unordered_map<std::string, std::string> out_params;

        if (match(route, req.path(), out_params)) {
            Response res;
            res.set_status(405);
            res.set_body("Method Not Allowed " + req.method() + ' ' + req.path());
            res.set_header("Content-Type", "text/plain");
            return res;
        }
    }

    return Response::not_found("Not found " + req.method() + ' ' + req.path());
}