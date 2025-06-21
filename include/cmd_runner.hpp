#ifndef CMD_RUNNER_HPP
#define CMD_RUNNER_HPP

#include <reproc++/drain.hpp>
#include <reproc++/reproc.hpp>
#include <chrono>
#include <string>
#include <string_view>
#include <vector>

struct CmdResult
{
    int exit_code = -1;   // 0 on success, >0 from child
    std::error_code ec{}; // non-zero → launch/wait error
    std::string out;      // full stdout (optional)
    std::string err;      // full stderr (optional)

    [[nodiscard]] bool ok() const { return !ec && exit_code == 0; }
};

class CmdRunner
{
public:
    static CmdResult run(const std::vector<std::string> &argv,
                         std::chrono::milliseconds deadline = reproc::infinite)
    {
        CmdResult res;

        reproc::process proc;
        res.ec = proc.start(argv);
        if (res.ec)
            return res;

        res.ec = reproc::drain(proc,
                               reproc::sink::string(res.out),
                               reproc::sink::string(res.err));
        if (res.ec)
            return res;

        std::tie(res.exit_code, res.ec) = proc.wait(deadline);
        return res;
    }
};

#endif
