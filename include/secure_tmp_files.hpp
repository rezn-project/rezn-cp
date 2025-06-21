#include <utility>
#include <filesystem>
#include <fstream>
#include <system_error>
#include <unistd.h>
#include <fcntl.h>
#include <stdexcept>
#include <vector>

namespace fs = std::filesystem;

class SecureTempFile
{
    fs::path path_;

public:
    explicit SecureTempFile(fs::path p) : path_(std::move(p)) {}
    ~SecureTempFile()
    {
        if (!path_.empty())
        {
            fs::remove(path_);
        }
    }
    const fs::path &path() const { return path_; }
    // Non-copyable, movable
    SecureTempFile(const SecureTempFile &) = delete;
    SecureTempFile &operator=(const SecureTempFile &) = delete;
    SecureTempFile(SecureTempFile &&) = default;
    SecureTempFile &operator=(SecureTempFile &&) = default;
};

// returns (fd, path); caller closes fd and removes path when done
static std::pair<int, fs::path> secure_temp_file(const std::string &prefix = "prefix")
{
    fs::path tmpl = fs::temp_directory_path() / (prefix + "XXXXXX");
    std::string s = tmpl.string();
    int fd = mkstemp(s.data()); // creates + opens O_RDWR
    if (fd == -1)
        throw std::runtime_error("mkstemp failed");
    fs::permissions(fs::path{s},
                    fs::perms::owner_read | fs::perms::owner_write,
                    fs::perm_options::replace);
    return {fd, fs::path{s}};
}
