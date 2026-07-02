#include "snapshot.h"
#include <inttypes.h>

int snapshot_to_csv(OrderBook *ob, Snapshot *snap, FILE *f) {
    if (!f || !ob || !snap) return -1;
    
    fprintf(f, "%" PRIu64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 ",%" PRId64 "\n",
        snap->timestamp_ns,
        snap->midprice,
        snap->spread,
        ob->total_bid_volume,
        ob->total_ask_volume,
        snap->imbalance_bps,
        snap->microprice,
        ob->best_bid
    );
    return 0;
}

int ob_dump_depth(OrderBook *ob, FILE *f) {
    if (!f || !ob) return -1;
    
    fprintf(f, "=== BIDS (best first) ===\n");
    for (size_t i = 0; i < ob->bid_count; i++) {
        fprintf(f, "  Level %zu: price=%" PRId64 ", volume=%" PRId64 "\n", 
            i, ob->bids[i].price, ob->bids[i].total_volume);
    }
    
    fprintf(f, "=== ASKS (best first) ===\n");
    for (size_t i = 0; i < ob->ask_count; i++) {
        fprintf(f, "  Level %zu: price=%" PRId64 ", volume=%" PRId64 "\n", 
            i, ob->asks[i].price, ob->asks[i].total_volume);
    }
    
    return 0;
}
