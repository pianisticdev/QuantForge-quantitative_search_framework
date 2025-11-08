//
// Created by Daniel Griffiths on 11/1/25.
//

#ifndef QUANT_FORGE_UTILS_HPP
#define QUANT_FORGE_UTILS_HPP

#include <filesystem>
#include <string>
#include <vector>

namespace string_utils {
    size_t write_to_string(const char* ptr, size_t size, size_t nmemb, void* userdata);

    bool ieq_prefix(const char* buf, size_t n, const char* key);

    std::string trim(std::string s);

    std::filesystem::path append_to_path(const std::filesystem::path& path, const std::string& str);

    std::vector<std::string> split_comma_delimited_string(std::string_view sv);
}  // namespace string_utils

#endif
