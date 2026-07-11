#ifndef DETERMINISM_H
#define DETERMINISM_H

#include <stdint.h>
#include "order_book.h"
#include "metrics.h"

uint64_t ob_hash_state(const OrderBook *ob);
uint64_t ob_hash_snapshot(const Snapshot *snap);

#endif
