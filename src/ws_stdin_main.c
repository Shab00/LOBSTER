#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <cjson/cJSON.h>
#include "order_book.h"
#include "metrics.h"
#include "fast_parser.h"

#define MAX_PRICE_LEVELS 5000
#define MAX_ORDERS 100000
#define MAX_LINE 65536

static volatile int running = 1;
static uint64_t update_count = 0;
static uint64_t total_latency_ns = 0;

static void sigint_handler(int sig) { (void)sig; running = 0; }

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

int main(void) {
    signal(SIGINT, sigint_handler);

    OrderBook *book = ob_create(MAX_PRICE_LEVELS, MAX_ORDERS);
    char line[MAX_LINE];

    fprintf(stderr, "[STARTING] abyss stdin depth stream...\n");

    while (running && fgets(line, sizeof(line), stdin)) {
        uint64_t t_start = now_ns();

        /* Remove trailing newline */
        line[strcspn(line, "\n")] = '\0';

        cJSON *root = cJSON_Parse(line);
        if (!root) continue;

        /* Rebuild book from snapshot */
        ob_destroy(book);
        book = ob_create(MAX_PRICE_LEVELS, MAX_ORDERS);
        uint64_t ts = now_ns();
        order_id_t order_id = 0;

        /* Parse bids */
        cJSON *bids = cJSON_GetObjectItem(root, "bids");
        if (bids && cJSON_IsArray(bids)) {
            cJSON *bid;
            cJSON_ArrayForEach(bid, bids) {
                cJSON *ps = cJSON_GetArrayItem(bid, 0);
                cJSON *qs = cJSON_GetArrayItem(bid, 1);
                if (!ps || !qs) continue;
                price_t p = fast_parse_decimal(ps->valuestring);
                volume_t q = fast_parse_decimal(qs->valuestring);
                if (p > 0 && q > 0) ob_add_order(book, order_id++, p, q, ts);
            }
        }

        /* Parse asks — NOTE: negate price to distinguish from bids */
        cJSON *asks = cJSON_GetObjectItem(root, "asks");
        if (asks && cJSON_IsArray(asks)) {
            cJSON *ask;
            cJSON_ArrayForEach(ask, asks) {
                cJSON *ps = cJSON_GetArrayItem(ask, 0);
                cJSON *qs = cJSON_GetArrayItem(ask, 1);
                if (!ps || !qs) continue;
                price_t p = -fast_parse_decimal(ps->valuestring);
                volume_t q = fast_parse_decimal(qs->valuestring);
                if (p < 0 && q > 0) ob_add_order(book, order_id++, p, q, ts);
            }
        }

        cJSON_Delete(root);

        Snapshot snap;
        metrics_compute(book, &snap, ts);

        uint64_t latency = now_ns() - t_start;
        total_latency_ns += latency;
        update_count++;

        printf("[%lu] SPREAD: %.2f | MID: %.2f | IMBALANCE: %.2f bps | "
               "BID: %zu | ASK: %zu | LAT: %lu ns | UPDATES: %lu\n",
               (unsigned long)time(NULL),
               (double)snap.spread / 1e8,
               (double)snap.midprice / 1e8,
               (double)snap.imbalance_bps / 100.0,
               book->bid_count, book->ask_count,
               (unsigned long)latency, (unsigned long)update_count);
    }

    fprintf(stderr, "\n[DONE] Processed %lu updates\n", (unsigned long)update_count);
    if (update_count > 0) {
        fprintf(stderr, "[STATS] Avg latency: %lu ns (%.2f µs)\n",
               (unsigned long)(total_latency_ns / update_count),
               (double)(total_latency_ns / update_count) / 1000.0);
    }

    ob_destroy(book);
    return 0;
}
