#ifndef _WINDOW_BIT_COUNT_APX_
#define _WINDOW_BIT_COUNT_APX_

#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <math.h>
#include <string.h>

//#define DEBUG
#ifdef DEBUG
#define DBG(x, ...) printf("[Debug]%s:%d\t"x, __func__, __LINE__, ##__VA_ARGS__)
#else
#define DBG(x, ...)
#endif

uint64_t N_MERGES = 0; // keep track of how many bucket merges occur

typedef struct {
    int idx;             // group index
    int n_vallid;        // number of valid buckets 
    int *buckets;           // circular array
    int head, tail;     // index of the head and tail element
    int k;              // algorithm parameter
    int bucket_size;    // the size of each bucket
} group_t;

static inline int group_bpop(group_t* grp);
static inline void group_bpush(group_t* grp, int ts);

/* Init group instance. */
void init_group(group_t *self, int idx, int k) {
    assert(self != NULL);
    self->idx = idx;
    self->buckets = (int*) malloc((k + 1) * sizeof(int));
    memset(self->buckets, 0, ((k + 1) * sizeof(int)));
    //DBG("memset len: %d\n", sizeof(self->buckets));
    assert(self->buckets != NULL);
    self->head = self->tail = 0;
    self->k = k;
    self->bucket_size = pow(2, idx);
    self->n_vallid = 0;
}

void free_group(group_t* grp) {
    free(grp->buckets);
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
    DBG("adding item %d to group %d\n", ts, grp->idx);
    if (grp->n_vallid == (grp->k +1)) {
        // merge: keep the latest timestamp
        N_MERGES++;
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

/**
 * @brief return the number of 1's expired
 * 
 * @param grp 
 * @param time 
 * @param w 
 * @return the number of 1's expired
 * @retval -1 this group is empty 
 */
int group_check_expire(group_t *grp, int time, int w) {
    int ret = 0;
    int expired = 0;

    if (grp->n_vallid == 0) 
        return -1;
    assert(grp->n_vallid > 0);

        DBG("checking grp %d bucket of time %d\n", grp->idx, grp->buckets[grp->tail]);
    if (grp->buckets[grp->tail] <= (time - w)) {
        expired = group_bpop(grp);
        DBG("expired bucket: %d of size %d\n", expired, grp->bucket_size);
    }
    
    if (expired) {
        ret = grp->bucket_size;
    }

    return ret;
}

/** 
 * calculate number of groups according to xxxxx.
 * @param w window size
 * @param k algorithm parameter
 * @retval num_groups total number of groups needed.
 */
int calculate_num_groups(int w, int k) {
    int ret;
    double m;
    m = ceil(log2(  (w / (k + 1) +1)  ));
    // cast
    DBG("before cast: %lf", m);
    ret = (int) m;
    DBG("after cast: %d", ret);
    // According to https://stackoverflow.com/questions/7657326/how-to-convert-double-to-int-in-c
    // We need to make sure the converted value >= double
    ret++;

    return ret;
}

typedef struct {
    // TODO: Fill me.
    group_t *groups;
    int last_grp;
    int time;
    int ngroup;
    uint32_t w;
    uint32_t sum;
} StateApx;

// k = 1/eps
// if eps = 0.01 (relative error 1%) then k = 100
// if eps = 0.001 (relative error 0.1%) the k = 1000
uint64_t wnd_bit_count_apx_new(StateApx* self, uint32_t wnd_size, uint32_t k) {
    uint64_t nbytes;
    int i, j, ngroup;

    assert(wnd_size >= 1);
    assert(k >= 1);

    N_MERGES = 0;
    ngroup = calculate_num_groups(wnd_size, k);
    nbytes = ngroup * sizeof(group_t);
    nbytes += ngroup * (k+1) * sizeof(int);
    self->ngroup = ngroup;
    self->last_grp = 0;
    self->w = wnd_size;
    self->sum = 0;

    self->groups = (group_t*) malloc(nbytes);
    memset(self->groups, 0, nbytes);
    for (i=0; i < ngroup; i++) {
        init_group(&(self->groups[i]), i, k);
    }

    return nbytes;
}

void wnd_bit_count_apx_destruct(StateApx* self) {
    int i;
    for (i=0; i < self->ngroup; i++) {
        free_group(&(self->groups[i]));
    }
    free(self->groups);
}

void print_group(group_t* grp) {
    printf("group %d: size %d, active buckets %d, head %d, tail %d, group_1_count: %d\nbuckets: ", 
                   grp->idx, grp->bucket_size, grp->n_vallid, grp->head, grp->tail, grp->bucket_size * grp->n_vallid);
    int tail = grp->tail;
    for (int i=0; i < (grp->k + 1); i++) {
        printf("|%d|", grp->buckets[tail++]);
        tail %= (grp->k + 1);
    }
    printf("\n");
    
}
void wnd_bit_count_apx_print(StateApx* self) {
    // This is useful for debugging.
    DBG("printing time: %d, last_group %d\n", self->time, self->last_grp);
    printf("window-size: %d\n", self->w);
    for (int i=0; i < self->ngroup; i++) {
        print_group(&(self->groups[i]));
    }
}

uint32_t wnd_bit_count_apx_next(StateApx* self, bool item) {
    int i = 0;
    int input_item, num_expire;
    self->time++;
    input_item = self->time;
    if (item == 1) {
        self->sum++;
        // recursive add item to group
        for (i = 0; i < self->ngroup; i++) {
            if (i > self->last_grp) self->last_grp = i;
            input_item = group_add(&(self->groups[i]), input_item);
            DBG("input_item %d comes from group %d\n", input_item, i);
            if (input_item < 0) 
                break;
        }
    }


    do {
        num_expire = group_check_expire(&(self->groups[self->last_grp]), self->time, self->w);
        if (num_expire < 0) {
            if (self->last_grp == 0) {
                // this means the 0th group is also empty.
                DBG("All empty!\n");
                num_expire = 0;
            }
            else {
                self->last_grp--;
            }
        }
        DBG("last group is %d", self->last_grp);
        assert(self->last_grp >= 0);
    } while (num_expire < 0);
    
    self->sum -= num_expire;

    DBG("self sum = %d\n", self->sum);
    return self->sum - self->groups[self->last_grp].bucket_size + 1;
}

#endif // _WINDOW_BIT_COUNT_APX_
