#include <libwebsockets.h>
#include <cjson/cJSON.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "order_book.h"
#include "metrics.h"
#include "fast_parser.h"

static volatile int running = 1;
static OrderBook *book = NULL;
static uint64_t update_count = 0;
static uint64_t total_latency_ns = 0;

#define MAX_PRICE_LEVELS 5000
#define MAX_ORDERS 100000

static void sigint_handler(int sig) {
    (void)sig;
    running = 0;
}

static uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + (uint64_t)ts.tv_nsec;
}

static int callback(struct lws *wsi, enum lws_callback_reasons reason,
                    void *user, void *in, size_t len) {
    (void)wsi;
    (void)user;
    (void)len;

    switch (reason) {

    case LWS_CALLBACK_CLIENT_ESTABLISHED:
        printf("[CONNECTED] Live BTCUSDT depth stream\n");
        break;

    case LWS_CALLBACK_CLIENT_RECEIVE: {
        uint64_t t_start = now_ns();

        const char *msg = (const char *)in;

        cJSON *root = cJSON_Parse(msg);
        if (!root) {
            printf("[PARSE ERROR] %s\n", msg);
            break;
        }

        /* Clear existing book */
        ob_destroy(book);
        book = ob_create(MAX_PRICE_LEVELS, MAX_ORDERS);

        uint64_t ts = now_ns();
        order_id_t order_id = 0;

        /* Parse bids */
        cJSON *bids = cJSON_GetObjectItem(root, "bids");
        if (bids && cJSON_IsArray(bids)) {
            cJSON *bid = NULL;
            cJSON_ArrayForEach(bid, bids) {
                cJSON *price_str = cJSON_GetArrayItem(bid, 0);
                cJSON *qty_str = cJSON_GetArrayItem(bid, 1);

                if (!price_str || !qty_str) continue;

                price_t price = fast_parse_decimal(price_str->valuestring);
                volume_t qty = fast_parse_decimal(qty_str->valuestring);

                if (price > 0 && qty > 0) {
                    ob_add_order(book, order_id++, price, qty, ts);
                }
            }
        }

        /* Parse asks */
        cJSON *asks = cJSON_GetObjectItem(root, "asks");
        if (asks && cJSON_IsArray(asks)) {
            cJSON *ask = NULL;
            cJSON_ArrayForEach(ask, asks) {
                cJSON *price_str = cJSON_GetArrayItem(ask, 0);
                cJSON *qty_str = cJSON_GetArrayItem(ask, 1);

                if (!price_str || !qty_str) continue;

                price_t price = fast_parse_decimal(price_str->valuestring);
                volume_t qty = fast_parse_decimal(qty_str->valuestring);

                if (price > 0 && qty > 0) {
                    ob_add_order(book, order_id++, price, qty, ts);
                }
            }
        }

        cJSON_Delete(root);

        /* Compute metrics */
        Snapshot snap;
        metrics_compute(book, &snap, ts);

        uint64_t t_end = now_ns();
        uint64_t latency = t_end - t_start;
        total_latency_ns += latency;
        update_count++;

        /* Print metrics */
        double spread_val = (double)snap.spread / 1e8;
        double mid_val = (double)snap.midprice / 1e8;
        double imbalance_val = (double)snap.imbalance_bps / 100.0;

        printf("[%lu] SPREAD: %.2f | MID: %.2f | IMBALANCE: %.2f bps | "
               "BID_DEPTH: %zu | ASK_DEPTH: %zu | LAT: %lu ns | UPDATES: %lu\n",
               (unsigned long)time(NULL),
               spread_val,
               mid_val,
               imbalance_val,
               book->bid_count,
               book->ask_count,
               (unsigned long)latency,
               (unsigned long)update_count);

        break;
    }

    case LWS_CALLBACK_CLIENT_CONNECTION_ERROR:
        printf("[ERROR] Connection failed\n");
        running = 0;
        break;

    case LWS_CALLBACK_CLOSED:
        printf("[DISCONNECTED]\n");
        running = 0;
        break;

    default:
        break;
    }

    return 0;
}

static const struct lws_protocols protocols[] = {
    {
        .name = "lobster-binance",
        .callback = callback,
        .per_session_data_size = 0,
        .rx_buffer_size = 4096,
    },
    { NULL, NULL, 0, 0 }
};

int main(void) {
    signal(SIGINT, sigint_handler);

    book = ob_create(MAX_PRICE_LEVELS, MAX_ORDERS);

    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = CONTEXT_PORT_NO_LISTEN;
    info.protocols = protocols;
    info.options = LCCSCF_USE_SSL;
    info.gid = -1;
    info.uid = -1;
    info.client_ssl_ca_filepath = "/opt/homebrew/etc/ca-certificates/cert.pem";
    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "Failed to create WebSocket context\n");
        return 1;
    }

    struct lws_client_connect_info conn_info;
    memset(&conn_info, 0, sizeof(conn_info));
    conn_info.context = context;
    conn_info.address = "stream.binance.com";
    conn_info.port = 9443;
    conn_info.path = "/ws/btcusdt@depth20@100ms";
    conn_info.host = conn_info.address;
    conn_info.origin = conn_info.address;
    conn_info.protocol = protocols[0].name;
    conn_info.ssl_connection = LCCSCF_USE_SSL | LCCSCF_ALLOW_SELFSIGNED | LCCSCF_SKIP_SERVER_CERT_HOSTNAME_CHECK;
    struct lws *wsi = lws_client_connect_via_info(&conn_info);
    if (!wsi) {
        fprintf(stderr, "Failed to connect to Binance\n");
        lws_context_destroy(context);
        ob_destroy(book);
        return 1;
    }

    printf("[STARTING] LOBSTER live depth stream...\n");

    while (running) {
        lws_service(context, 500);
    }

    printf("\n[DONE] Processed %lu updates\n", (unsigned long)update_count);
    if (update_count > 0) {
        printf("[STATS] Avg latency: %lu ns (%.2f µs)\n",
               (unsigned long)(total_latency_ns / update_count),
               (double)(total_latency_ns / update_count) / 1000.0);
    }

    lws_context_destroy(context);
    ob_destroy(book);

    return 0;
}
