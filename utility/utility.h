#ifndef UTILS_H
#define UTILS_H

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...) \
fprintf(stderr, "--- %s:%d:%s(): " fmt, __FILE__, __LINE__, __func__, ##__VA_ARGS__)
#else
#define DEBUG_LOG(fmt, ...) ((void)0)
#endif

// ********************************* //

#define SAFE_FREE(p) \
do               \
{                \
free(p);     \
(p) = NULL;  \
} while (0)

#endif // UTILS_H