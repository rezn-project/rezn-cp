#include <utility>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

// returns (fd, path); caller closes fd and removes path when done
static std::pair<int, fs::path> secure_temp_file(const std::string &prefix = "prefix")
{
    fs::path tmpl = fs::temp_directory_path() / (prefix + "stepXXXXXX");
    std::string s = tmpl.string();
    int fd = mkstemp(s.data()); // creates + opens O_RDWR
    if (fd == -1)
        throw std::runtime_error("mkstemp failed");
    fs::permissions(fs::path{s},
                    fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace);
    return {fd, fs::path{s}};
}

static void secure_overwrite_and_remove(const fs::path &p)
{
    std::error_code ec;
    auto sz = fs::file_size(p, ec);
    if (!ec && sz > 0)
    {
        int fd = ::open(p.c_str(), O_WRONLY);
        if (fd != -1)
        {
            std::vector<char> zeros(4096, '\0');
            size_t remaining = sz;
            while (remaining)
            {
                size_t n = std::min(remaining, zeros.size());
                if (::write(fd, zeros.data(), n) != static_cast<ssize_t>(n))
                    break;
                remaining -= n;
            }
            ::fsync(fd);
            ::close(fd);
        }
    }
    fs::remove(p, ec);
}