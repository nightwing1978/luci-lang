/*******************************************************************
 * Copyright (c) 2022-2023 TheWallSoft
 * This file is part of the Luci Language
 * tom@thewallsoft.com, https://github.com/nightwing1978/luci-lang
 * See Copyright Notice in the LICENSE file or at
 * https://github.com/nightwing1978/luci-lang/blob/main/LICENSE
 *******************************************************************/

#include "Util.h"
#include <sstream>
#include <regex>

namespace util
{
    std::string join(const std::vector<std::string> &strings, const std::string &separator)
    {
        if (strings.empty())
            return std::string();

        std::stringstream ss;
        ss << strings.front();

        for (size_t i = 1; i < strings.size(); ++i)
        {
            ss << separator << strings[i];
        }
        return ss.str();
    }

    std::string removeNewLines(const std::string &input)
    {
        std::regex newLines("\n+");
        return std::regex_replace(input, newLines, " ");
    }

    std::string withEllipsis(const std::string &input, size_t max_length)
    {
        if (input.size() <= max_length)
            return input;

        std::string without = input.substr(0, std::max<size_t>(3, max_length) - 3);
        return without + "...";
    }

    namespace color
    {
        namespace fg
        {
            const std::string default_ = "\033[0m";

            const std::string black = "\033[30m";
            const std::string red = "\033[31m";
            const std::string green = "\033[32m";
            const std::string yellow = "\033[33m";
            const std::string blue = "\033[34m";
            const std::string magenta = "\033[35m";
            const std::string white = "\033[97m";
        }
        namespace bg
        {
            const std::string black = "\033[40m";
            const std::string red = "\033[41m";
            const std::string green = "\033[42m";
            const std::string yellow = "\033[43m";
            const std::string blue = "\033[44m";
            const std::string magenta = "\033[45m";
            const std::string white = "\033[107m";
        }

        /* colorize the string and return back to the default color */
        std::string colorize(const std::string &input, const std::string &color)
        {
            return color + input + fg::default_;
        }
    }
}