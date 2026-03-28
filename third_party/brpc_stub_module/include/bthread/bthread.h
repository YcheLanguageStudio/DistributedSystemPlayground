#ifndef BTHREAD_BTHREAD_H
#define BTHREAD_BTHREAD_H

#include "bthread/types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern int bthread_start_background(
    bthread_t* __restrict tid,
    const bthread_attr_t* __restrict attr,
    void* (*fn)(void*),
    void* __restrict arg);

extern int bthread_join(bthread_t bt, void** bthread_return);

extern int bthread_yield(void);

#ifdef __cplusplus
}
#endif

#endif
