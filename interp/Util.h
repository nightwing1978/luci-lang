#ifndef GUARDIAN_OF_INCLUSION_UTIL_H
#define GUARDIAN_OF_INCLUSION_UTIL_H

#include <vector>
#include <deque>
#include <string>

namespace util
{
	std::string join(const std::vector<std::string>& strings, const std::string& separator);

    namespace color
    {
        namespace fg
        {
            extern const std::string default_;

            extern const std::string black;
            extern const std::string white;
            extern const std::string red;
            extern const std::string green;
            extern const std::string blue;
            extern const std::string yellow;
            extern const std::string magenta;
        }
        namespace bg
        {
            extern const std::string black;
            extern const std::string white;
            extern const std::string red;
            extern const std::string green;
            extern const std::string blue;
            extern const std::string yellow;
            extern const std::string magenta;
        }

        /* colorize the string and return back to the default color */
        std::string colorize(const std::string& input, const std::string& color);
    }

    std::string withEllipsis(const std::string& input, size_t max_length);
}

#endif