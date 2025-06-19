#ifndef CP_PASSGEN_CTX_H
#define CP_PASSGEN_CTX_H

#include <string>
#include <stdexcept>


extern "C" {
    #include <passgen/passgen.h>
    #include "passgen/generate.h"
    #include "passgen/pattern/env.h"
}

class PassGenCtx {
public:
    PassGenCtx() {
        if (!passgen_random_system_open(&rng_))
            throw std::runtime_error("passgen: system RNG unavailable");

        passgen_env_init(&env_, &rng_);          // returns void
    }

    ~PassGenCtx() { }    // frees rng_ once

    PassGenCtx(const PassGenCtx&)            = delete;
    PassGenCtx& operator=(const PassGenCtx&) = delete;

    std::string generate(int len = 14) {
        const std::string spec =
            "[A-Za-z0-9]{" + std::to_string(len) + '}';

        passgen_pattern pat{};
        passgen_error   err{};
        if (passgen_parse(&pat, &err, spec.c_str()) != 0)
            throw std::runtime_error(
                "passgen parse failed: " + std::string(err.message));

        std::string out(len * 4, '\0');        // UTF-8 worst-case buffer
        size_t n = passgen_generate_fill_utf8(
                       &pat, &env_, nullptr,
                       reinterpret_cast<uint8_t*>(out.data()), out.size());
        passgen_pattern_free(&pat);

        out.resize(n);
        return out;
    }

private:
    passgen_random rng_{};
    passgen_env    env_{};
};

#endif
