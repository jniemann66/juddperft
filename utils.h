#ifndef UTILS_H
#define UTILS_H

#include <cstdint>
#include <string>

namespace juddperft {

class Utils
{
public:
    // convert a large number of bytes into "B", "KiB", "MiB", "GiB" ... etc
    static std::string memorySizeWithBinaryPrefix(size_t bytes);
};

} // namespace juddperft

#endif // UTILS_H
