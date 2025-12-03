#include "utils.h"

#include <map>
#include <sstream>

namespace juddperft {

std::string Utils::memorySizeWithBinaryPrefix(size_t bytes)
{
    int shift = 60;
    size_t n = 0;
    if (bytes) {
        while (shift >= 0) {
            n = (bytes >> shift) & 0x3ff;
            if (n) {
                break;
            }
            shift -= 10;
        }

        static const std::map<int, std::string> m
            {
                {60, "EiB"},
                {50, "PiB"},
                {40, "TiB"},
                {30, "GiB"},
                {20, "MiB"},
                {10, "KiB"},
                {0, "B"}
            };

        std::stringstream ss;
        ss << n << " " << m.at(shift);
        return ss.str();
    }
    return "0 B";
}


} // namespace juddperft
