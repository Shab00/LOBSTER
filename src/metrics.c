#include "metrics.h"

void metrics_compute(OrderBook *ob, Snapshot *snap, uint64_t ts) {
    snap->timestamp_ns = ts;
    
    price_t bid = ob_get_best_bid(ob);
    price_t ask = ob_get_best_ask(ob);
    
    if (bid > 0 && ask > 0) {
        snap->midprice = (bid + ask) / 2;
        snap->spread = ask - bid;
    } else {
        snap->midprice = 0;
        snap->spread = 0;
    }
    
    volume_t total_vol = ob->total_bid_volume + ob->total_ask_volume;
    if (total_vol > 0) {
        int64_t imbalance = (ob->total_bid_volume - ob->total_ask_volume) * 10000LL / total_vol;
        snap->imbalance_bps = imbalance;
    } else {
        snap->imbalance_bps = 0;
    }
    
    if (total_vol > 0) {
        snap->microprice = (bid * ob->total_ask_volume + ask * ob->total_bid_volume) / total_vol;
    } else {
        snap->microprice = 0;
    }
}
