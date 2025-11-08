//
// Created by Daniel Griffiths on 11/1/25.
//

#include "string_utils.hpp"

#include <filesystem>
#include <string>
#include <vector>

namespace string_utils {
    size_t write_to_string(const char *ptr, size_t size, size_t nmemb, void *userdata) {
        auto *body = static_cast<std::string *>(userdata);
        const size_t total = size * nmemb;
        body->append(ptr, total);
        return total;
    }

    bool ieq_prefix(const char *buf, size_t n, const char *key) {
        for (size_t i = 0; key[i] != '\0' && i < n; ++i) {
            if (std::tolower(buf[i]) != std::tolower(key[i])) {
                return false;
            }
            if (key[i + 1] == '\0') {
                return true;
            }
        }
        return false;
    }

    std::string trim(std::string s) {
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char c) { return std::isspace(c) == 0; }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char c) { return std::isspace(c) == 0; }).base(), s.end());
        return s;
    }

    std::filesystem::path append_to_path(const std::filesystem::path &path, const std::string &str) { return {path.string() + str}; }

    std::vector<std::string> split_comma_delimited_string(std::string_view sv) {
        std::vector<std::string> out;
        size_t start = 0;
        for (;;) {
            size_t pos = sv.find(',', start);
            size_t end = (pos == std::string_view::npos) ? sv.size() : pos;

            std::string_view token = sv.substr(start, end - start);
            const auto first = token.find_first_not_of(" \t");
            if (first != std::string_view::npos) {
                const auto last = token.find_last_not_of(" \t");
                out.emplace_back(token.substr(first, last - first + 1));
            }

            if (pos == std::string_view::npos) {
                break;
            }

            start = pos + 1;
        }
        return out;
    }
}  // namespace string_utils
