#ifndef METRICS_H
#define METRICS_H

#include <stdint.h>
#include "order_book.h"

typedef struct {
    price_t midprice;
    price_t spread;
    int64_t imbalance_bps;
    price_t microprice;
    uint64_t timestamp_ns;
} Snapshot;

void metrics_compute(OrderBook *ob, Snapshot *snap, uint64_t ts);

static inline price_t fixed_divide(int64_t numerator, int64_t denominator) {
    if (denominator == 0) return 0;
    return (numerator * 100000000LL) / denominator;
}

static inline int64_t fixed_multiply(int64_t a, int64_t b) {
    return (a * b) / 100000000LL;
}

#endif
