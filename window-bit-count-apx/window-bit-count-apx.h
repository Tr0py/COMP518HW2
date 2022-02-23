#ifndef _WINDOW_BIT_COUNT_APX_
#define _WINDOW_BIT_COUNT_APX_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>

uint64_t N_MERGES = 0; // keep track of how many bucket merges occur

/*
    TODO: You can add code here.
*/

typedef struct {
    int idx;             // group index
    int n_vallid;        // number of valid buckets 
    int *buckets;           // circular array
    int head, tail;     // index of the head and tail element
    int k;              // algorithm parameter
    int bucket_size;    // the size of each bucket
} group_t;

/* Init group instance. */
group_t* creat_group(int idx, int k) {
    group_t *ret = (group_t*) malloc(sizeof(group_t));
    assert(ret != NULL);
    ret->idx = idx;
    ret->buckets = (int*) malloc((k + 1) * sizeof(int));
    assert(ret->buckets != NULL);
    ret->head = ret->tail = 0;
    ret->k = k;
    ret->bucket_size = pow(2, idx);
    return ret;
}

void free_group(group_t* grp) {
    free(grp->buckets);
    free(grp);
}

/* add an element to group.
 * @retval -1 no merge happens
 * @retval timestamp if merge happens returns the timestamp of the emitted bucket
 * @param grp pointer to group
 * @param ts timestamp of input item
 */
int group_add(group_t* grp, int ts) {
    int emit = -1;
    // check if full --> need to merge
    if (grp->n_vallid == (grp->k +1)) {
        // merge: keep the latest timestamp
        group_bpop(grp);
        emit = group_bpop(grp);
    }
    group_bpush(grp, ts);
    return emit;
}

static inline int group_bpop(group_t* grp) {
    int ret = grp->buckets[grp->tail];
    grp->tail++;
    grp->tail %= (grp->k + 1);
    grp->n_vallid--;
    return ret;
}

static inline void group_bpush(group_t* grp, int ts) {
    grp->buckets[grp->head] = ts;
    grp->head++;
    grp->head %= (grp->k + 1);
    grp->n_vallid++;
}

inline int group_get_sum(group_t* grp) {
    return grp->n_vallid * grp->bucket_size;
}
typedef struct {
    // TODO: Fill me.
} StateApx;

// k = 1/eps
// if eps = 0.01 (relative error 1%) then k = 100
// if eps = 0.001 (relative error 0.1%) the k = 1000
uint64_t wnd_bit_count_apx_new(StateApx* self, uint32_t wnd_size, uint32_t k) {
    assert(wnd_size >= 1);
    assert(k >= 1);

    // TODO: Fill me.

    // TODO:
    // The function should return the total number of bytes allocated on the heap.
    return 0;
}

void wnd_bit_count_apx_destruct(StateApx* self) {
    // TODO: Fill me.
    // Make sure you free the memory allocated on the heap.
}

void wnd_bit_count_apx_print(StateApx* self) {
    // This is useful for debugging.
}

uint32_t wnd_bit_count_apx_next(StateApx* self, bool item) {
    // TODO: Fill me.
    return 0;
}

#endif // _WINDOW_BIT_COUNT_APX_
