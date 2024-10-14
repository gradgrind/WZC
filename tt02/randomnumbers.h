#ifndef RANDOMNUMBERS_H
#define RANDOMNUMBERS_H

#include <cstdint>
#include <random>

class RandomNumbers
// JSF: Based on an algorithm by Bob Jenkins
{
public:
    RandomNumbers(uint32_t seed = 0) : a(0xf1ea5eed)
    // Original seed: uint32_t(0xcafe5eed00000001ULL)
    {
        if (seed == 0) {
            std::random_device rd;
            seed = rd();
        }
        b = c = d = seed;
        for (unsigned int i = 0; i < 20; ++i) {
            advance();
        }
    }

    uint32_t operator()()
    {
        return advance();
    }

private:
    uint32_t a, b, c, d;

    uint32_t advance()
    {
        uint32_t e = a - ((b<<27) | (b>>5));
        a = b ^ ((c<<17) | (c>>15));
        b = c + d;
        c = d + e;
        d = e + a;
        return d;
    }
};


// *************************

// xoshiro256** random number generator implementation.
// Based on original written in 2018 by David Blackman and Sebastiano Vigna
// (vigna@acm.org):
/*
To the extent possible under law, the author has dedicated all copyright
and related and neighboring rights to this software to the public domain
worldwide.

Permission to use, copy, modify, and/or distribute this software for any
purpose with or without fee is hereby granted.

THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR
IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE. */

/*
#include <array>
class Xoshiro256ss
{// Private
    using result_type = uint64_t;
    std::array<result_type,4> s;   // Internal state 256 bits

    static inline result_type rotl(const result_type x, const int k)
    {
        return (x << k) | (x >> (64 - k));
    }

    static inline result_type splitmix64(result_type x)
    {
        x += 0x9E3779B97F4A7C15;
        x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9;
        x = (x ^ (x >> 27)) * 0x94D049BB133111EB;
        return x ^ (x >> 31);
    }

public:
    Xoshiro256ss(const result_type val = std::random_device{}() )
    {// Initialization
        seed(val);
    }

    void seed(const result_type seed_value)
    {
        for (int i = 0; i < 4; ++i)
            s[i] = splitmix64(seed_value + i);
    }

    static result_type min()
    {
        return result_type(0ull);
    }

    static result_type max()
    {
        return  std::numeric_limits<result_type>::max();
    }

    result_type operator()()
    {   // 256**
        const uint64_t result = rotl(s[1] * 5, 7) * 9;
        const uint64_t t = s[1] << 17;

        s[2] ^= s[0];
        s[3] ^= s[1];
        s[1] ^= s[2];
        s[0] ^= s[3];
        s[2] ^= t;
        s[3] = rotl(s[3], 45);

        return result;
    }
};
*/

#endif // RANDOMNUMBERS_H
