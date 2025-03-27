#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <arpa/inet.h>
#include <sys/socket.h>

#define SAFE_FREE(p)                                                           \
    do {                                                                       \
        free(p);                                                               \
        (p) = NULL;                                                            \
    } while (0)

#ifdef DEBUG
#define DEBUG_PRINT(fmt, ...)                                                  \
    fprintf(stderr, "DEBUG: %s:%d:%s(): " fmt "\n", __FILE__, __LINE__,        \
            __func__, __VA_ARGS__)
#else
#define DEBUG_PRINT(fmt, ...) ((void)0)
#endif


#define DEFINE_LIST(T)                                                         \
    typedef struct {                                                           \
        T* data;                                                               \
        size_t count;                                                          \
        size_t size;                                                           \
    } list_##T;                                                                \
                                                                               \
    void list_##T##_new(list_##T* v) {                                         \
        v->data = NULL;                                                        \
        v->count = v->size = 0;                                                \
    }                                                                          \
                                                                               \
    void list_##T##_push(list_##T* v, T elem) {                                \
        if (v->count == v->size) {                                             \
            v->size = v->size ? v->size << 1 : 1;                              \
            v->data = (T*)realloc(v->data, v->size * sizeof(T));               \
        }                                                                      \
        v->data[v->count++] = elem;                                            \
    }                                                                          \
                                                                               \
    T list_##T##_pop(list_##T* v) { return v->data[--v->count]; }              \
                                                                               \
    void list_##T##_free(list_##T* v) {                                        \
        free(v->data);                                                         \
        list_##T##_new(v);                                                     \
    }

DEFINE_LIST(int)

int test_list(void) {
    list_int numbers;
    list_int_new(&numbers);
    list_int_push(&numbers, 22);
    int n = list_int_pop(&numbers);
    printf("n: %d\n", n);

    list_int_push(&numbers, 11);
    list_int_push(&numbers, 12);
    list_int_push(&numbers, 13);

    for (size_t i = 0; i < numbers.count; ++i) {
        printf("numbers[%lu]: %d\n", i, numbers.data[i]);
    }

    list_int_free(&numbers);
    return 0;
}
