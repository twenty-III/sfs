#pragma once

#include <string>
#include <unordered_map>

class Response {
public:
    Response() {
        headers_["connection"] = "Close";
        headers_["content-length"] = "0";
    }

    int status_code() const { return status_code_; }
    std::string status_text() const { return status_text_for(status_code_); }
    const std::unordered_map<std::string, std::string>& headers() const { return headers_; }
    const std::string& body() const { return body_; }

    void set_status(int code) { status_code_ = code; }
    void set_header(const std::string& key, const std::string& value);
    void set_body(std::string body);

    std::string serialize() const;

    static Response ok(const std::string& body, const std::string& content_type = "text/plain");
    static Response json(const std::string& body);
    static Response html(const std::string& body);
    static Response not_found(const std::string& msg = "404 Not Found");
    static Response bad_request(const std::string& msg = "400 Bad Request");
    static Response server_error(const std::string& msg = "500 Internal Server Error");

private:
    int status_code_ = 200;
    std::unordered_map<std::string, std::string> headers_;
    std::string body_;

    static std::string status_text_for(int code);
};