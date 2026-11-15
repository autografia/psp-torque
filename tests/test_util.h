#ifndef TEST_UTIL_H
#define TEST_UTIL_H

#include <math.h>
#include <stdio.h>

extern int g_failures;

#define CHECK(cond)                                                  \
    do {                                                             \
        if (!(cond)) {                                               \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);   \
            g_failures++;                                            \
        }                                                            \
    } while (0)

#define CHECK_NEAR(actual, expected, eps)                            \
    do {                                                             \
        double a_ = (double)(actual);                                \
        double e_ = (double)(expected);                              \
        if (fabs(a_ - e_) > (eps)) {                                 \
            printf("FAIL %s:%d: %s = %.4f, esperado %.4f\n",         \
                   __FILE__, __LINE__, #actual, a_, e_);             \
            g_failures++;                                            \
        }                                                            \
    } while (0)

#endif
