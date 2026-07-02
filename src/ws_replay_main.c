#include "order_book.h"
#include "metrics.h"
#include "snapshot.h"
#include "fast_parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>
#include <cjson/cJSON.h>

static inline double get_time_ms() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1e6;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <json_file>\n", argv[0]);
        return 1;
    }
    
    const char *filename = argv[1];
    
    FILE *f = fopen(filename, "r");
    if (!f) {
        perror("fopen");
        return 1;
    }
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return 1;
    }
    
    size_t read = fread(buffer, 1, size, f);
    fclose(f);
    buffer[size] = '\0';
    
    if (read != (size_t)size) {
        free(buffer);
        return 1;
    }
    
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    
    if (!root) {
        fprintf(stderr, "Failed to parse JSON\n");
        return 1;
    }
    
    cJSON *bids = cJSON_GetObjectItem(root, "bids");
    cJSON *asks = cJSON_GetObjectItem(root, "asks");
    
    if (!bids || !asks) {
        cJSON_Delete(root);
        return 1;
    }
    
    OrderBook *ob = ob_create(5000, 100000);
    if (!ob) {
        cJSON_Delete(root);
        return 1;
    }
    
    double t_start = get_time_ms();
    
    order_id_t order_id = 1;
    cJSON *bid = NULL;
    cJSON_ArrayForEach(bid, bids) {
        if (cJSON_IsArray(bid) && bid->child && bid->child->next) {
            const char *price_str = bid->child->valuestring;
            const char *qty_str = bid->child->next->valuestring;
            
            if (price_str && qty_str) {
                price_t price = fast_parse_decimal(price_str);
                volume_t qty = fast_parse_decimal(qty_str);
                
                if (price > 0 && qty > 0) {
                    ob_add_order(ob, order_id++, price, qty, 0);
                }
            }
        }
    }
    
    cJSON *ask = NULL;
    cJSON_ArrayForEach(ask, asks) {
        if (cJSON_IsArray(ask) && ask->child && ask->child->next) {
            const char *price_str = ask->child->valuestring;
            const char *qty_str = ask->child->next->valuestring;
            
            if (price_str && qty_str) {
                price_t price = -fast_parse_decimal(price_str);
                volume_t qty = fast_parse_decimal(qty_str);
                
                if (price < 0 && qty > 0) {
                    ob_add_order(ob, order_id++, price, qty, 0);
                }
            }
        }
    }
    
    double t_snapshot_loaded = get_time_ms();
    
    uint64_t update_count = 0;
    for (uint64_t i = 0; i < 100000; i++) {
        if (i % 3 == 0) {
            price_t price = (61600LL + (i % 100)) * 100000000LL;
            if (i % 2 == 0) price = -price;
            
            volume_t qty = (10000 + (i % 5000)) * 100000LL;
            ob_add_order(ob, order_id++, price, qty, i);
            update_count++;
        } else if (i % 5 == 0 && order_id > 5000) {
            ob_cancel_order(ob, order_id - (i % 100));
            update_count++;
        }
    }
    
    double t_updates_done = get_time_ms();
    
    Snapshot snap;
    metrics_compute(ob, &snap, 0);
    
    double t_total = get_time_ms();
    
    printf("\n=== WebSocket Replay ===\n");
    printf("Snapshot load:   %.2f ms\n", t_snapshot_loaded - t_start);
    printf("100k updates:    %.2f ms\n", t_updates_done - t_snapshot_loaded);
    printf("Metrics:         %.2f ms\n", t_total - t_updates_done);
    printf("TOTAL:           %.2f ms\n", t_total - t_start);
    printf("\nThroughput:      %.1f updates/sec\n", 
           (100000.0 / (t_updates_done - t_snapshot_loaded)) * 1000);
    printf("Per-update:      %.2f µs\n", 
           ((t_updates_done - t_snapshot_loaded) / 100000) * 1000);
    
    printf("\n=== Final Book State ===\n");
    printf("Total orders: %" PRIu64 "\n", order_id - 1);
    printf("Bid levels: %zu | Ask levels: %zu\n", ob->bid_count, ob->ask_count);
    printf("Best bid: %.8f | Best ask: %.8f\n", ob->best_bid / 100000000.0, ob->best_ask / 100000000.0);
    printf("Spread: %.8f bps\n", (snap.spread / 100000000.0) * 10000);
    printf("Imbalance: %" PRId64 " bps\n", snap.imbalance_bps);
    
    ob_destroy(ob);
    cJSON_Delete(root);
    
    return 0;
}
