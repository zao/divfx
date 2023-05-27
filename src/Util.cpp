#include "Util.hpp"

#include <fstream>

std::string SlurpTextFile(std::filesystem::path const &path) {
    std::string ret;
    auto fh = std::ifstream(path, std::ios::binary);
    ret.resize(file_size(path));
    fh.read(ret.data(), ret.size());
    return ret;
}
