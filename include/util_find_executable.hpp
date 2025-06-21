// Platform‑aware PATH search with optional Windows API, env caching,
// ACL‑aware exec test, runtime faccessat2 detection, and SUID control.

#ifndef UTIL_FIND_EXECUTABLE_HPP
#define UTIL_FIND_EXECUTABLE_HPP

#include <filesystem>
#include <optional>
#include <string_view>
#include <cstdlib>
#include <vector>
#include <mutex>
#include <shared_mutex>

#if defined(__linux__)
#include <dlfcn.h> // dlopen, dlsym for faccessat2 probe
#include <unistd.h>
#include <fcntl.h>
#endif

#ifdef _WIN32
#define NOMINMAX
#include <windows.h>
#include <cstring> // _stricmp
#endif

// ──────────────────────────────── compile‑time toggles ─────────────────────────────
#ifndef UTIL_WANT_WINAPI
#define UTIL_WANT_WINAPI true // Windows: use SearchPathW if true
#endif
#ifndef UTIL_WANT_ENV_CACHING
#define UTIL_WANT_ENV_CACHING true // cache PATH / PATHEXT between calls
#endif

// ──────────────────────────────── helpers ─────────────────────────────
#ifdef _WIN32
inline constexpr char kPathSep = ';';
#else
inline constexpr char kPathSep = ':';
#endif

inline std::vector<std::string_view> split_sv(std::string_view sv,
                                              char sep = kPathSep)
{
    std::vector<std::string_view> out;
    std::size_t start = 0;
    while (start < sv.size())
    {
        std::size_t pos = sv.find(sep, start);
        if (pos == std::string_view::npos)
            pos = sv.size();
        if (pos != start)
            out.emplace_back(sv.substr(start, pos - start));
        start = pos + 1;
    }
    return out;
}

#ifdef _WIN32
inline bool ci_equal(std::string_view a, std::string_view b) noexcept
{
    return _stricmp(a.data(), b.data()) == 0;
}
#endif

// ──────────────────────────────── environment fetch (portable) ─────────────────────
#ifdef _WIN32
// Use WinAPI UTF‑16 env fetch → UTF‑8 string.
inline std::string getenv_utf8(const wchar_t *wide_name)
{
    DWORD wlen = GetEnvironmentVariableW(wide_name, nullptr, 0);
    if (wlen == 0)
        return {};
    std::wstring wbuf(wlen, L'\0');
    GetEnvironmentVariableW(wide_name, wbuf.data(), wlen);

    int u8len = WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string u8(u8len - 1, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wbuf.c_str(), -1, u8.data(), u8len, nullptr, nullptr);
    return u8;
}
#else
inline std::string getenv_utf8(const char *name) { return std::getenv(name) ?: ""; }
#endif

// ──────────────────────────────── EnvCache ─────────────────────────────
#if UTIL_WANT_ENV_CACHING
struct EnvCache
{
    std::string path_raw, pext_raw;
    std::vector<std::filesystem::path> dirs;
#ifdef _WIN32
    std::vector<std::string> exts;
#endif
    mutable std::shared_mutex mx;

    static EnvCache &instance()
    {
        static EnvCache c;
        return c;
    }

    void refresh_if_needed()
    {
        std::string cur_path;
        std::string cur_pext;
#ifdef _WIN32
        cur_path = getenv_utf8(L"PATH");
        cur_pext = getenv_utf8(L"PATHEXT");
#else
        cur_path = getenv_utf8("PATH");
#endif
        {
            std::shared_lock r{mx};
            if (cur_path == path_raw && cur_pext == pext_raw)
                return; // unchanged
        }
        std::unique_lock w{mx};
        // recompute
        dirs.clear();
        for (auto sv : split_sv(cur_path))
            dirs.emplace_back(std::filesystem::weakly_canonical(std::filesystem::path{sv}));
#ifdef _WIN32
        exts.assign({""});
        for (auto sv : split_sv(cur_pext, ';'))
            exts.emplace_back(sv);
#endif
        path_raw.swap(cur_path);
        pext_raw.swap(cur_pext);
    }
};
#endif // UTIL_WANT_ENV_CACHING

// ──────────────────────────────── Linux faccessat2 runtime probe ───────────────────
#if defined(__linux__)
namespace detail
{
    using faccessat2_fn = int (*)(int, const char *, int, int);
    inline faccessat2_fn load_faccessat2()
    {
        static faccessat2_fn fn = []
        {
            void *h = dlopen("libc.so.6", RTLD_LAZY);
            if (!h)
                return (faccessat2_fn) nullptr;
            auto sym = reinterpret_cast<faccessat2_fn>(dlsym(h, "faccessat2"));
            // keep handle open for rest of program
            return sym;
        }();
        return fn;
    }
} // namespace detail
#endif

// ──────────────────────────────── backends ─────────────────────────────
#ifdef _WIN32
#if UTIL_WANT_WINAPI
inline std::optional<std::filesystem::path>
search_winapi(std::string_view name)
{
    int wlen = MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), nullptr, 0);
    if (wlen == 0)
        return std::nullopt;
    std::wstring wname(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, name.data(), static_cast<int>(name.size()), wname.data(), wlen);

    wchar_t buf[MAX_PATH];
    DWORD n = SearchPathW(nullptr, wname.c_str(), nullptr, MAX_PATH, buf, nullptr);
    if (n == 0 || n >= MAX_PATH)
        return std::nullopt;
    return std::filesystem::path{buf};
}
#endif // UTIL_WANT_WINAPI

inline std::optional<std::filesystem::path>
search_windows_fs(std::string_view name)
{
#if UTIL_WANT_ENV_CACHING
    auto &c = EnvCache::instance();
    c.refresh_if_needed();
    const auto &dirs = c.dirs;
    const auto &exts = c.exts;
#else
    std::vector<std::filesystem::path> dirs;
    for (auto sv : split_sv(getenv_utf8(L"PATH")))
        dirs.emplace_back(sv);
    std::vector<std::string> exts{""};
    for (auto sv : split_sv(getenv_utf8(L"PATHEXT"), ';'))
        exts.emplace_back(sv);
#endif

    bool has_dot = name.find('.') != std::string_view::npos;
    for (const auto &d : dirs)
    {
        for (const auto &ext : exts)
        {
            if (has_dot && !ext.empty())
                continue;
            std::filesystem::path cand = d / (std::string{name} + ext);
            std::error_code ec;
            if (std::filesystem::exists(cand, ec))
                return cand;
        }
    }
    return std::nullopt;
}
#else // POSIX ──────────────────────────────────────────────────────────
inline bool is_exec(const std::filesystem::path &p, bool allow_suid)
{
    std::error_code ec;
    auto st = std::filesystem::status(p, ec);
    if (ec || !std::filesystem::is_regular_file(st))
        return false;

#if defined(__linux__)
    if (auto fn = detail::load_faccessat2(); fn)
    {
        if (fn(AT_FDCWD, p.c_str(), X_OK, AT_EACCESS | AT_SYMLINK_NOFOLLOW) == 0)
            return true;
    }
#endif

    using perms = std::filesystem::perms;
    auto pr = st.permissions();
    bool exec = (pr & (perms::owner_exec | perms::group_exec | perms::others_exec)) != perms::none;
    bool suid = (pr & (perms::set_uid | perms::set_gid)) != perms::none;
    return exec && (allow_suid || !suid);
}

inline std::optional<std::filesystem::path>
search_posix(std::string_view name, bool allow_suid)
{
#if UTIL_WANT_ENV_CACHING
    auto &c = EnvCache::instance();
    c.refresh_if_needed();
    const auto &dirs = c.dirs;
#else
    std::vector<std::filesystem::path> dirs;
    for (auto sv : split_sv(getenv_utf8("PATH")))
        dirs.emplace_back(sv);
#endif

    for (const auto &d : dirs)
    {
        std::filesystem::path cand = d / name;
        if (is_exec(cand, allow_suid))
            return cand;
    }
    return std::nullopt;
}
#endif // _WIN32

// ──────────────────────────────── public API ──────────────────────────
#ifdef _WIN32
inline std::optional<std::filesystem::path>
find_executable(std::string_view name,
                bool allow_suid = false, // ignored on Windows
                bool use_winapi = UTIL_WANT_WINAPI)
{
    (void)allow_suid;
#if UTIL_WANT_WINAPI
    if (use_winapi)
    {
        if (auto p = search_winapi(name))
            return p;
    }
#endif
    return search_windows_fs(name);
}
#else // POSIX overload (no winapi param) ──────────────────────────────
inline std::optional<std::filesystem::path>
find_executable(std::string_view name,
                bool allow_suid = false)
{
    return search_posix(name, allow_suid);
}
#endif

#endif // UTIL_FIND_EXECUTABLE_HPP
