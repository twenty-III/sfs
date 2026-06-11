#include <response.hpp>
#include <string_utils.hpp>

#include <string>
#include <sstream>
#include <iostream>

std::string Response::status_text_for(int code)
{
    switch (code)
    {
    case (200):
        return "OK";
    case (201):
        return "Created";
    case (204):
        return "No Content";
    case (400):
        return "Bad Request";
    case (403):
        return "Forbidden";
    case (404):
        return "Not Found";
    case (405):
        return "Method Not Allowed";
    case (500):
        return "Internal Server Error";
    default:
        return "Unknown";
    }
}

void Response::set_header(std::string key, std::string value)
{
    auto lower_key = to_lower_str(key);
    if (lower_key == "content-length" || lower_key == "connection")
    {
        std::cerr << "[Warning] Skipped manual override of restricted header '" << key << "'." << '\n';
        return;
    }
    headers_.insert_or_assign(std::move(lower_key), std::move(value));
}

void Response::set_body(std::string body)
{
    headers_["content-length"] = std::to_string(body.size());
    body_ = std::move(body);
}

std::string Response::serialize() const
{
    std::ostringstream oss;

    oss << "HTTP/1.1 " << status_code_ << " " << status_text_for(status_code_) << "\r\n";

    for (const auto &[key, value] : headers_)
    {
        oss << key << ": " << value << "\r\n";
    }

    oss << "\r\n"
        << body_;

    return oss.str();
}

Response Response::ok(std::string body, std::string content_type)
{
    Response res;
    res.set_status(200);
    res.set_header("content-type", std::move(content_type));
    res.set_body(std::move(body));
    return res;
}

Response Response::json(std::string body)
{
    return ok(std::move(body), "application/json");
}

Response Response::html(std::string body)
{
    return ok(std::move(body), "text/html");
}

Response Response::not_found(std::string msg)
{
    Response res;
    res.set_status(404);
    res.set_header("content-type", "text/plain");
    res.set_body(std::move(msg));
    return res;
}

Response Response::bad_request(std::string msg)
{
    Response res;
    res.set_status(400);
    res.set_header("content-type", "text/plain");
    res.set_body(std::move(msg));
    return res;
}

Response Response::server_error(std::string msg)
{
    Response res;
    res.set_status(500);
    res.set_header("content-type", "text/plain");
    res.set_body(std::move(msg));
    return res;
}