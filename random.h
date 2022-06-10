#ifndef RANDOM_H
#define RANDOM_H

#include <cstdint>
#include <random>
#include <memory>

class Random {
private:
    std::unique_ptr<std::minstd_rand> random;
public:
    Random(uint32_t seed) {
        random = std::make_unique<std::minstd_rand>(seed);
    }

    uint32_t next() {
        return (uint32_t)(*random)();
    }
};

#endif