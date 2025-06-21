#include <string>
#include <string_view>

namespace util
{
    // Characters regarded as “space” by <ctype> but expressed as a constexpr:
    constexpr std::string_view whitespace{" \f\n\r\t\v"};

    // ----  view-returning versions  ----
    [[nodiscard]] inline std::string_view ltrim(std::string_view sv) noexcept
    {
        const auto first = sv.find_first_not_of(whitespace);
        return first == std::string_view::npos ? std::string_view{} : sv.substr(first);
    }

    [[nodiscard]] inline std::string_view rtrim(std::string_view sv) noexcept
    {
        const auto last = sv.find_last_not_of(whitespace);
        return last == std::string_view::npos ? std::string_view{} : sv.substr(0, last + 1);
    }

    [[nodiscard]] inline std::string_view trim(std::string_view sv) noexcept
    {
        return rtrim(ltrim(sv));
    }
} // namespace util
