#pragma once

#include <request.hpp>
#include <response.hpp>

#include <string>
#include <functional>

using Handler = std::function<Response(const Request&)>;

