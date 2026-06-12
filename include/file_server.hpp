#pragma once

#include <response.hpp>
#include <request.hpp>

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class FileServer
{
public:
    static Response serve(const Request &req, const std::string &static_dir);

private:
    static std::string mime_type(const std::string &ext);
};