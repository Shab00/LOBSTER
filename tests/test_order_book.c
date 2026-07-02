#include <assert.h>
#include <stdio.h>
#include "order_book.h"
#include "metrics.h"

void test_add_order() {
    OrderBook *ob = ob_create(100, 1000);
    
    assert(ob_add_order(ob, 1, 10000 * 100000000LL, 1 * 100000000LL, 0) == 0);
    assert(ob_get_best_bid(ob) == 10000 * 100000000LL);
    
    assert(ob_add_order(ob, 2, -(10001 * 100000000LL), 2 * 100000000LL, 0) == 0);
    assert(ob_get_best_ask(ob) == 10001 * 100000000LL);
    
    ob_destroy(ob);
    printf("test_add_order PASSED\n");
}

void test_cancel_order() {
    OrderBook *ob = ob_create(100, 1000);
    
    ob_add_order(ob, 1, 10000 * 100000000LL, 1 * 100000000LL, 0);
    ob_add_order(ob, 2, 10000 * 100000000LL, 1 * 100000000LL, 0);
    
    assert(ob_cancel_order(ob, 1) == 0);
    assert(ob_cancel_order(ob, 999) == -1);  /* not found */
    
    ob_destroy(ob);
    printf("test_cancel_order PASSED\n");
}

void test_metrics() {
    OrderBook *ob = ob_create(100, 1000);
    
    ob_add_order(ob, 1, 10000 * 100000000LL, 1 * 100000000LL, 0);
    ob_add_order(ob, 2, -(10002 * 100000000LL), 1 * 100000000LL, 0);
    
    Snapshot snap;
    metrics_compute(ob, &snap, 0);
    
    assert(snap.midprice == (10000 + 10002) * 50000000LL);  /* (bid+ask)/2 */
    assert(snap.spread == 2 * 100000000LL);
    
    ob_destroy(ob);
    printf("test_metrics PASSED\n");
}

int main() {
    test_add_order();
    test_cancel_order();
    test_metrics();
    printf("\nAll tests passed!\n");
    return 0;
}
