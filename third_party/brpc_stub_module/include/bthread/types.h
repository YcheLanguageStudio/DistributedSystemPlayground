#ifndef BTHREAD_TYPES_H
#define BTHREAD_TYPES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t bthread_t;
typedef unsigned bthread_tag_t;

#define BTHREAD_TAG_DEFAULT 0

typedef struct bthread_attr_t {
    int _unused;
} bthread_attr_t;

#ifdef __cplusplus
}
#endif

#endif
