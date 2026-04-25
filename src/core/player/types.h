#pragma once

#define UNUSED(x) do { \
    (void)(x); \
} while(0)

struct gains_t
{
    float _31;
    float _63;
    float _125;
    float _250;
    float _500;
    float _1k;
    float _2k;
    float _4k;
    float _8k;
    float _16k;
};
