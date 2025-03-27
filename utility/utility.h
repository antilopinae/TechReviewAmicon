#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <errno.h>

#ifdef DEBUG
#define DEBUG_LOG(fmt, ...) \
fprintf(stderr, "--- %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) ((void)0)
#endif

#define SAFE_FREE(p) \
do               \
{                \
free(p);     \
(p) = NULL;  \
} while (0)

#endif // UTILS_H