#pragma once

#include <string>
#include <cctype>
#include <vector>
#include <sstream>
#include <algorithm>

inline std::string to_lower_str(const std::string &str)
{
    std::string lower_str = str;
    std::transform(lower_str.begin(), lower_str.end(), lower_str.begin(), [](unsigned char ch)
                   { return std::tolower(ch); });

    return lower_str;
}

inline std::vector<std::string> split_str(const std::string &str, char delimiter = '\n')
{
    std::istringstream ss(str);
    std::vector<std::string> res;

    std::string tmp;
    while (std::getline(ss, tmp, delimiter))
    {
        if (!tmp.empty())
        {
            res.push_back(tmp);
        }
    }

    return res;
}