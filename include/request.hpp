#pragma once

#include <string>
#include <unordered_map>

class Request {
    friend class Router;
    friend class RequestParser;

public:
    const std::string& method() const { return method_; }
    const std::string& path() const { return path_; }
    const std::string& version() const { return version_; }
    const std::string& body() const { return body_; }

    const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
    const std::unordered_map<std::string, std::string>& path_params() const { return path_params_; }
    const std::unordered_map<std::string, std::string>& query_params() const { return query_params_; }

    std::string get_header(const std::string& key) const;
    std::string get_path(const std::string& key) const;
    std::string get_query(const std::string& key) const;

private:
    std::string method_;
    std::string path_;
    std::string version_;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

    std::unordered_map<std::string, std::string> path_params_;
    std::unordered_map<std::string, std::string> query_params_;
};

class RequestParser {
public:
    static bool parse(int client_fd, Request& req);

private:
    static void parse_query_string(const std::string& query, std::unordered_map<std::string, std::string>& out);
};