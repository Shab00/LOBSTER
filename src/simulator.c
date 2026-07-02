#include "simulator.h"
#include "snapshot.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <cjson/cJSON.h>

int simulator_process_depth_snapshot(OrderBook *ob, SimulatorContext *ctx, const char *json_str, uint64_t ts) {
    if (parse_binance_depth(ob, json_str) != 0) {
        fprintf(stderr, "Failed to parse depth snapshot\n");
        return -1;
    }
    
    Snapshot snap;
    metrics_compute(ob, &snap, ts);
    snapshot_to_csv(ob, &snap, ctx->output_csv);
    
    ctx->event_count++;
    return 0;
}

int simulator_process_trade(OrderBook *ob, SimulatorContext *ctx, 
                            price_t price, volume_t qty, uint64_t ts) {
    (void)ob;
    (void)price;
    (void)qty;
    
    Snapshot snap;
    metrics_compute(ob, &snap, ts);
    snapshot_to_csv(ob, &snap, ctx->output_csv);
    
    ctx->event_count++;
    return 0;
}

SimulatorContext* simulator_init(const char *output_file) {
    SimulatorContext *ctx = malloc(sizeof(SimulatorContext));
    if (!ctx) return NULL;
    
    ctx->output_csv = fopen(output_file, "w");
    if (!ctx->output_csv) {
        free(ctx);
        return NULL;
    }
    
    fprintf(ctx->output_csv, "timestamp_ns,midprice,spread,bid_volume,ask_volume,imbalance_bps,microprice,best_bid\n");
    ctx->event_count = 0;
    clock_gettime(CLOCK_MONOTONIC, &ctx->start_time);
    
    return ctx;
}

void simulator_destroy(SimulatorContext *ctx) {
    if (!ctx) return;
    
    struct timespec end_time;
    clock_gettime(CLOCK_MONOTONIC, &end_time);
    double elapsed = (end_time.tv_sec - ctx->start_time.tv_sec) + 
                     (end_time.tv_nsec - ctx->start_time.tv_nsec) / 1e9;
    
    printf("\n=== Simulation Stats ===\n");
    printf("Events processed: %" PRIu64 "\n", ctx->event_count);
    printf("Time elapsed: %.3f sec\n", elapsed);
    printf("Throughput: %.1f events/sec\n", ctx->event_count / elapsed);
    printf("Per-event: %.1f ns\n", (elapsed / ctx->event_count) * 1e9);
    
    fclose(ctx->output_csv);
    free(ctx);
}
