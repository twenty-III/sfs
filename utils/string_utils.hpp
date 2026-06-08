#pragma once

#include <string>
#include <cctype>
#include <algorithm>

inline std::string to_lower_str(const std::string& str) {
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), [](unsigned char ch) {
        return std::tolower(ch);
    }); 

    return lower_str;
}