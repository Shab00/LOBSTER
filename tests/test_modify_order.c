#include <stdio.h>
#include "order_book.h"

#define MAX_LEVELS 100
#define MAX_ORDERS 1000

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  %s... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

int main(void) {
    printf("=== Modify Order Tests ===\n\n");

    TEST("Modify quantity up");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 500000000, 0);
        int r = ob_modify_order(ob, 1, 800000000);
        if (r != 0) FAIL("modify returned error");
        else if (ob->total_bid_volume != 800000000) FAIL("wrong total volume");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Modify quantity down");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 500000000, 0);
        int r = ob_modify_order(ob, 1, 200000000);
        if (r != 0) FAIL("modify returned error");
        else if (ob->total_bid_volume != 200000000) FAIL("wrong total volume");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Modify to zero cancels order");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 500000000, 0);
        int r = ob_modify_order(ob, 1, 0);
        if (r != 0) FAIL("modify to zero should succeed");
        else if (ob->bid_count != 0) FAIL("bid level should be removed");
        else if (ob->total_bid_volume != 0) FAIL("volume should be zero");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Modify non-existent order fails");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        int r = ob_modify_order(ob, 999, 100000000);
        if (r == 0) FAIL("should fail on non-existent order");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Modify below filled quantity fails");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 500000000, 0);
        ob_execute_trade(ob, -1, 300000000, 0);
        int r = ob_modify_order(ob, 1, 100000000);
        if (r == 0) FAIL("should fail when new_qty < filled");
        else PASS();
        ob_destroy(ob);
    }

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
