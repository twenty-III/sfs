#include <file_server.hpp>
#include <logger.hpp>
#include <string_utils.hpp>

#include <sstream>
#include <fstream>
#include <filesystem>
#include <unordered_map>

std::string FileServer::mime_type(const std::string &ext)
{
    static const std::unordered_map<std::string, std::string> mime_types = {
        {".html", "text/html"},
        {".css", "text/css"},
        {".js", "application/javascript"},
        {".json", "application/json"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".ico", "image/x-icon"},
        {".txt", "text/plain"}};

    auto it = mime_types.find(to_lower_str(ext));

    return (it == mime_types.end()) ? "application/octet-stream" : it->second;
}

Response FileServer::serve(const Request &req, const std::string &static_dir)
{
    auto base_dir = fs::weakly_canonical(static_dir);

    std::string req_path = req.path();

    if (!req_path.empty() && req_path[0] == '/')
    {
        req_path = req_path.substr(1);
    }

    fs::path tgt_path = base_dir / req_path;

    tgt_path = fs::weakly_canonical(tgt_path);

    if (tgt_path.string().find(base_dir.string()) != 0)
    {
        LOG_WARN("blocked directory traversal attack: ", req_path);

        Response res;
        res.set_status(403);
        res.set_body("Forbidded to access requested file");
        return res;
    }

    if (fs::is_directory(tgt_path))
    {
        tgt_path /= "index.html";
    }

    if (!fs::exists(tgt_path) || !fs::is_regular_file(tgt_path))
    {
        return Response::not_found("Reuqested file not found");
    }

    std::ifstream file(tgt_path, std::ios::binary);

    if (!file.is_open())
    {
        LOG_ERROR("failed to read requested file: ", req_path);
    }

    std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

    std::string ext = tgt_path.extension().string();

    LOG_INFO("served file: ", req_path);

    return Response::ok(std::move(content), mime_type(ext));
}