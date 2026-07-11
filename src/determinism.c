#include "order_book.h"
#include "metrics.h"

/* FNV-1a 64-bit hash */
static uint64_t fnv1a_64(const void *data, size_t len, uint64_t hash) {
    const unsigned char *bytes = (const unsigned char *)data;
    for (size_t i = 0; i < len; i++) {
        hash ^= bytes[i];
        hash *= 0x100000001b3ULL;
    }
    return hash;
}

uint64_t ob_hash_state(const OrderBook *ob) {
    uint64_t hash = 14695981039346656037ULL;  /* FNV offset basis */
    
    /* Hash bids */
    hash = fnv1a_64(&ob->bid_count, sizeof(ob->bid_count), hash);
    for (size_t i = 0; i < ob->bid_count; i++) {
        hash = fnv1a_64(&ob->bids[i].price, sizeof(ob->bids[i].price), hash);
        hash = fnv1a_64(&ob->bids[i].total_volume, sizeof(ob->bids[i].total_volume), hash);
    }
    
    /* Hash asks */
    hash = fnv1a_64(&ob->ask_count, sizeof(ob->ask_count), hash);
    for (size_t i = 0; i < ob->ask_count; i++) {
        hash = fnv1a_64(&ob->asks[i].price, sizeof(ob->asks[i].price), hash);
        hash = fnv1a_64(&ob->asks[i].total_volume, sizeof(ob->asks[i].total_volume), hash);
    }
    
    /* Hash totals */
    hash = fnv1a_64(&ob->total_bid_volume, sizeof(ob->total_bid_volume), hash);
    hash = fnv1a_64(&ob->total_ask_volume, sizeof(ob->total_ask_volume), hash);
    hash = fnv1a_64(&ob->best_bid, sizeof(ob->best_bid), hash);
    hash = fnv1a_64(&ob->best_ask, sizeof(ob->best_ask), hash);
    
    return hash;
}

uint64_t ob_hash_snapshot(const Snapshot *snap) {
    uint64_t hash = 14695981039346656037ULL;
    hash = fnv1a_64(&snap->midprice, sizeof(snap->midprice), hash);
    hash = fnv1a_64(&snap->spread, sizeof(snap->spread), hash);
    hash = fnv1a_64(&snap->imbalance_bps, sizeof(snap->imbalance_bps), hash);
    hash = fnv1a_64(&snap->microprice, sizeof(snap->microprice), hash);
    hash = fnv1a_64(&snap->timestamp_ns, sizeof(snap->timestamp_ns), hash);
    return hash;
}
