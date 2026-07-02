#include "order_book.h"
#include "metrics.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_ITERATIONS 1000000

int main() {
    OrderBook *ob = ob_create(1000, 10000000);
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (order_id_t i = 1; i <= NUM_ITERATIONS; i++) {
        price_t price = (i % 2 == 0) ? (10000LL + (i % 100)) * 100000000LL 
                                      : -(10001LL + (i % 100)) * 100000000LL;
        volume_t qty = (i % 10 + 1) * 100000000LL;
        ob_add_order(ob, i, price, qty, i);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) + 
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    
    printf("Added %d orders in %.3f seconds\n", NUM_ITERATIONS, elapsed);
    printf("Throughput: %.1f events/sec\n", NUM_ITERATIONS / elapsed);
    printf("Per-order: %.1f ns\n", (elapsed / NUM_ITERATIONS) * 1e9);
    printf("Order book size: %zu bid levels, %zu ask levels\n", ob->bid_count, ob->ask_count);
    
    ob_destroy(ob);
    return 0;
}
