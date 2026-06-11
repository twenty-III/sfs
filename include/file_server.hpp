#pragma once

#include <response.hpp>
#include <request.hpp>

#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class FileServer
{
public:
    FileServer(const std::string &base_dir) : base_dir_(fs::weakly_canonical(base_dir)) {}

    Response serve(const Request &req);

private:
    fs::path base_dir_;

    static std::string mime_type(const std::string &ext);
};