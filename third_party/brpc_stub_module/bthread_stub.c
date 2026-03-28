#include "bthread/bthread.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>

#if defined(__linux__)
#include <sched.h>
#endif

int bthread_yield(void) {
#if defined(__linux__)
    (void)sched_yield();
#endif
    return 0;
}

struct bthread_start_args {
    void* (*fn)(void*);
    void* arg;
};

static void* bthread_stub_trampoline(void* p) {
    struct bthread_start_args* a = (struct bthread_start_args*)p;
    void* (*fn)(void*) = a->fn;
    void* arg = a->arg;
    free(a);
    return fn(arg);
}

int bthread_start_background(
    bthread_t* __restrict tid,
    const bthread_attr_t* __restrict attr,
    void* (*fn)(void*),
    void* __restrict arg) {
    (void)attr;
    struct bthread_start_args* a = malloc(sizeof(struct bthread_start_args));
    if (!a) {
        return ENOMEM;
    }
    a->fn = fn;
    a->arg = arg;
    pthread_t pt;
    int rc = pthread_create(&pt, NULL, bthread_stub_trampoline, a);
    if (rc != 0) {
        free(a);
        return rc;
    }
    *tid = (bthread_t)pt;
    return 0;
}

int bthread_join(bthread_t bt, void** bthread_return) {
    pthread_t pt = (pthread_t)bt;
    return pthread_join(pt, bthread_return);
}
