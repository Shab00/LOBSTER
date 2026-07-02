#ifndef SNAPSHOT_H
#define SNAPSHOT_H

#include "order_book.h"
#include "metrics.h"
#include <stdio.h>

int snapshot_to_csv(OrderBook *ob, Snapshot *snap, FILE *f);

int ob_dump_depth(OrderBook *ob, FILE *f);

#endif
