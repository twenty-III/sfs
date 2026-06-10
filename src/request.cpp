#include <request.hpp>
#include <string_utils.hpp>

#include <string>
#include <cctype>
#include <sstream>
#include <stdexcept>
#include <algorithm>
#include <unordered_map>
#include <sys/socket.h>

void RequestParser::parse_query_string(const std::string &query, std::unordered_map<std::string, std::string> &out)
{
    std::istringstream ss(query);

    std::string pair;
    while (getline(ss, pair, '&'))
    {
        auto eql = pair.find('=');
        if (eql == std::string::npos)
        {
            out.insert_or_assign(std::move(pair), "");
        }
        else
        {
            auto key = pair.substr(0, eql);
            auto value = pair.substr(eql + 1);
            out.insert_or_assign(std::move(key), std::move(value));
        }
    }
}

std::string Request::get_header(const std::string &key) const
{
    auto it = headers_.find(to_lower_str(key));
    return it == headers_.end() ? "" : it->second;
}

std::string Request::get_path(const std::string &key) const
{
    auto it = path_params_.find(key);
    return it == path_params_.end() ? "" : it->second;
}

std::string Request::get_query(const std::string &key) const
{
    auto it = query_params_.find(key);
    return it == query_params_.end() ? "" : it->second;
}

bool RequestParser::parse(int client_fd, Request &req)
{
    constexpr size_t KB = 1024;

    constexpr size_t MAX_HEADER_SIZE = 8 * KB;
    constexpr size_t MAX_BODY_SIZE = KB * KB;

    std::string raw;
    raw.reserve(4 * KB);
    char buf[4 * KB];

    while (true)
    {
        ssize_t n = recv(client_fd, buf, sizeof(buf), 0);

        if (n <= 0)
            return false;

        raw.append(buf, n);

        if (raw.find("\r\n\r\n") != std::string::npos)
            break;
        if (raw.size() > MAX_HEADER_SIZE)
            return false;
    }

    auto header_end = raw.find("\r\n\r\n");
    std::string header_block = raw.substr(0, header_end);
    std::string leftover_body = raw.substr(header_end + 4);

    std::istringstream stream(header_block);

    std::string request_line;
    std::getline(stream, request_line);
    if (!request_line.empty() && request_line.back() == '\r')
    {
        request_line.pop_back();
    }

    std::istringstream rl(request_line);
    rl >> req.method_ >> req.path_ >> req.version_;
    if (req.method_.empty() || req.path_.empty() || req.version_.empty())
    {
        return false;
    }

    auto qmark = req.path_.find('?');
    if (qmark != std::string::npos)
    {
        parse_query_string(req.path_.substr(qmark + 1), req.query_params_);
        req.path_ = req.path_.substr(0, qmark);
    }

    std::string header_line;
    while (std::getline(stream, header_line))
    {
        if (!header_line.empty() && header_line.back() == '\r')
        {
            header_line.pop_back();
        }

        auto colon = header_line.find(':');
        if (colon == std::string::npos)
        {
            continue;
        }

        std::string key = to_lower_str(header_line.substr(0, colon));
        std::string value = header_line.substr(colon + 1);
        value.erase(0, value.find_first_not_of(" \t"));
        req.headers_.insert_or_assign(std::move(key), std::move(value));
    }

    std::string cl_str = req.get_header("Content-Length");
    if (!cl_str.empty())
    {
        size_t content_len = 0;
        try
        {
            content_len = std::stoul(cl_str);
        }
        catch (...)
        {
            return false;
        }

        if (content_len > MAX_BODY_SIZE)
            return false;

        req.body_ = leftover_body;

        while (req.body_.size() < content_len)
        {
            size_t remaining = content_len - req.body_.size();
            ssize_t n = recv(client_fd, buf, std::min(sizeof(buf), remaining), 0);

            if (n <= 0)
            {
                return false;
            }

            req.body_.append(buf, n);
        }
    }

    return true;
}