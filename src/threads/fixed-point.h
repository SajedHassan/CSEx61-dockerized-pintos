#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#include <stdint.h>

#define F (1 << 14)

typedef int fixed_point;

fixed_point int_to_fixed(int n) {
    return n * F;
}
int fixed_to_int_zero(fixed_point x) {
    return x / F;
}
int fixed_to_int_nearest(fixed_point x) {
    if (x >= 0) {
        return (x + F / 2) / F;
    } else {
        return (x - F / 2) / F;
    }
}
fixed_point add(fixed_point x, fixed_point y) {
    return x + y;
}

fixed_point sub(fixed_point x, fixed_point y) {
    return x - y;
}

fixed_point add_int(fixed_point x, int n) {
    return x + n * F;
}

fixed_point sub_int(fixed_point x, int n) {
    return x - n * F;
}

fixed_point mul(fixed_point x, fixed_point y) {
    return ((int64_t) x) * y / F;
}

fixed_point mul_int(fixed_point x, int n) {
    return x * n;
}

fixed_point div(fixed_point x, fixed_point y) {
    return ((int64_t) x) * F / y;
}

fixed_point div_int(fixed_point x, int n) {
    return x / n;
}
#endif