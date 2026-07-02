#ifndef SIMULATOR_H
#define SIMULATOR_H

#include "order_book.h"
#include <stdint.h>
#include <stdio.h>
#include <time.h>

typedef struct {
    FILE *output_csv;
    uint64_t event_count;
    struct timespec start_time;
} SimulatorContext;

SimulatorContext* simulator_init(const char *output_file);
void simulator_destroy(SimulatorContext *ctx);

int simulator_process_depth_snapshot(OrderBook *ob, SimulatorContext *ctx, 
                                     const char *json_str, uint64_t ts);
int simulator_process_trade(OrderBook *ob, SimulatorContext *ctx, 
                            price_t price, volume_t qty, uint64_t ts);

#endif
