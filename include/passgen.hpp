#ifndef CP_PASSGEN_H
#define CP_PASSGEN_H

#include <string>

extern "C" {
    #include <passgen/passgen.h>
    #include "passgen/generate.h"
    #include "passgen/random.h"
    #include "passgen/pattern/env.h"
}

static std::string make_password(int length = 14)
{
    /* ---------- 1. build & parse pattern ---------- */
    const std::string spec = "[A-Za-z0-9]{" + std::to_string(length) + "}";

    passgen_pattern pat;          // not pre-initialised on purpose
    passgen_error   err{};
    if (passgen_parse(&pat, &err, spec.c_str()) != 0) {
        throw std::runtime_error("passgen_parse failed: " +
                                 std::string(err.message));
    }

    /* ---------- 2. RNG + environment ---------- */
    passgen_random rng;
    passgen_random_system_open(&rng);         // /dev/urandom, getrandom(), …

    passgen_env env;
    passgen_env_init(&env, &rng);

    /* ---------- 3. generate ---------- */
    std::string out(length * 4, '\0');        // worst-case UTF-8 room
    size_t n = passgen_generate_fill_utf8(
                  &pat, &env, /*entropy*/nullptr,
                  reinterpret_cast<uint8_t*>(out.data()), out.size());

    out.resize(n);

    /* ---------- 4. tidy up ---------- */
    // passgen_env_free(&env);
    // passgen_random_close(&rng);
    // passgen_pattern_free(&pat);

    return out;
}

#endif
