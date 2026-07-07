#include <stdio.h>
#include <assert.h>
#include "order_book.h"

#define MAX_LEVELS 100
#define MAX_ORDERS 1000

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  %s... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

int main(void) {
    printf("=== Execute Trade Tests ===\n\n");

    TEST("Complete fill of single order");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 500000000, 0);
        int filled = ob_execute_trade(ob, -1, 500000000, 0);
        if (filled != 500000000) FAIL("wrong fill qty");
        else if (ob->bid_count != 0) FAIL("bid level not removed");
        else if (ob->total_bid_volume != 0) FAIL("volume not zero");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Partial fill leaves remainder");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 1000000000, 0);
        int filled = ob_execute_trade(ob, -1, 300000000, 0);
        if (filled != 300000000) FAIL("wrong fill qty");
        else if (ob->bid_count != 1) FAIL("level removed prematurely");
        else if (ob->bids[0].total_volume != 700000000) FAIL("wrong remaining volume");
        else if (ob->total_bid_volume != 700000000) FAIL("wrong total volume");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Fill walks multiple price levels");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 300000000, 0);
        ob_add_order(ob, 2,  99000000, 500000000, 0);
        int filled = ob_execute_trade(ob, -1, 600000000, 0);
        if (filled != 600000000) FAIL("wrong fill qty");
        else if (ob->bid_count != 1) FAIL("wrong level count");
        else if (ob->bids[0].price != 99000000) FAIL("wrong remaining best bid price");
        else if (ob->bids[0].total_volume != 200000000) FAIL("wrong remaining volume at second level");
        else if (ob->total_bid_volume != 200000000) FAIL("wrong total volume");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Empty book returns 0");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        int filled = ob_execute_trade(ob, -1, 100000000, 0);
        if (filled != 0) FAIL("should return 0 on empty book");
        else PASS();
        ob_destroy(ob);
    }

    TEST("Trade larger than available liquidity");
    {
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 100000000, 200000000, 0);
        int filled = ob_execute_trade(ob, -1, 500000000, 0);
        if (filled != 200000000) FAIL("should only fill what's available");
        else if (ob->bid_count != 0) FAIL("level should be cleared");
        else PASS();
        ob_destroy(ob);
    }

    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
