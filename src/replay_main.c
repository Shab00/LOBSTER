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
    
    double t_start = get_time_ms();
    
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
    
    if (read != (size_t)size) {
        fprintf(stderr, "Failed to read file\n");
        free(buffer);
        return 1;
    }
    
    double t_file_read = get_time_ms();
    buffer[size] = '\0';
    
    cJSON *root = cJSON_Parse(buffer);
    free(buffer);
    
    double t_json_parse = get_time_ms();
    
    if (!root) {
        fprintf(stderr, "Failed to parse JSON\n");
        return 1;
    }
    
    cJSON *bids = cJSON_GetObjectItem(root, "bids");
    cJSON *asks = cJSON_GetObjectItem(root, "asks");
    
    if (!bids || !asks) {
        fprintf(stderr, "Invalid format: missing bids or asks\n");
        cJSON_Delete(root);
        return 1;
    }
    
    OrderBook *ob = ob_create(5000, 100000);
    if (!ob) {
        fprintf(stderr, "Failed to create order book\n");
        cJSON_Delete(root);
        return 1;
    }
    
    double t_ob_create = get_time_ms();
    
    double t_orders_start = get_time_ms();
    
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
    
    double t_bids_done = get_time_ms();
    
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
    
    double t_load_orders = get_time_ms();
    
    Snapshot snap;
    metrics_compute(ob, &snap, 0);
    
    double t_metrics = get_time_ms();
    
    printf("\n=== Timing Breakdown ===\n");
    printf("File read:      %.2f ms\n", t_file_read - t_start);
    printf("JSON parse:     %.2f ms\n", t_json_parse - t_file_read);
    printf("OB create:      %.2f ms\n", t_ob_create - t_json_parse);
    printf("Load bids:      %.2f ms\n", t_bids_done - t_orders_start);
    printf("Load asks:      %.2f ms\n", t_load_orders - t_bids_done);
    printf("Metrics:        %.2f ms\n", t_metrics - t_load_orders);
    printf("TOTAL:          %.2f ms\n", t_metrics - t_start);
    
    printf("\n=== Order Book ===\n");
    printf("Orders: %" PRIu64 "\n", order_id - 1);
    printf("Bid levels: %zu | Ask levels: %zu\n", ob->bid_count, ob->ask_count);
    printf("Best bid: %.8f | Best ask: %.8f\n", ob->best_bid / 100000000.0, ob->best_ask / 100000000.0);
    printf("Spread: %.8f | Imbalance: %" PRId64 " bps\n", snap.spread / 100000000.0, snap.imbalance_bps);
    
    ob_destroy(ob);
    cJSON_Delete(root);
    
    return 0;
}
