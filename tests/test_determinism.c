#include <stdio.h>
#include <string.h>
#include "order_book.h"
#include "metrics.h"
#include "determinism.h"

#define MAX_LEVELS 5000
#define MAX_ORDERS 100000
#define REPLAYS 10

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  %s... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

static uint64_t run_replay(void) {
    OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
    
    /* Add known bid orders */
    ob_add_order(ob, 1,  100000000, 500000000, 1000);
    ob_add_order(ob, 2,  100000000, 300000000, 1001);
    ob_add_order(ob, 3,   99000000, 700000000, 1002);
    ob_add_order(ob, 4,   98000000, 200000000, 1003);
    
    /* Add known ask orders (negative prices) */
    ob_add_order(ob, 5, -101000000, 400000000, 1004);
    ob_add_order(ob, 6, -101000000, 200000000, 1005);
    ob_add_order(ob, 7, -102000000, 600000000, 1006);
    ob_add_order(ob, 8, -103000000, 300000000, 1007);
    
    /* Execute some trades */
    ob_execute_trade(ob,  1, 200000000, 2000);  /* buy 2 against asks */
    ob_execute_trade(ob, -1, 400000000, 2001);  /* sell 4 against bids */
    
    /* Cancel an order */
    ob_cancel_order(ob, 4);
    
    /* Compute metrics */
    Snapshot snap;
    metrics_compute(ob, &snap, 3000);
    
    /* Hash both state and snapshot */
    uint64_t state_hash = ob_hash_state(ob);
    uint64_t snap_hash = ob_hash_snapshot(&snap);
    
    ob_destroy(ob);
    
    return state_hash ^ snap_hash;
}

int main(void) {
    printf("=== Determinism Tests ===\n\n");
    
    TEST("Identical replays produce identical hashes");
    {
        uint64_t hashes[REPLAYS];
        
        for (int i = 0; i < REPLAYS; i++) {
            hashes[i] = run_replay();
        }
        
        int all_match = 1;
        for (int i = 1; i < REPLAYS; i++) {
            if (hashes[i] != hashes[0]) {
                all_match = 0;
                printf("FAIL: replay %d hash %llu != replay 0 hash %llu\n", 
                       i, (unsigned long long)hashes[i], (unsigned long long)hashes[0]);
                break;
            }
        }
        
        if (all_match) {
            printf("PASS (all %d hashes = %llu)\n", REPLAYS, (unsigned long long)hashes[0]);
            tests_passed++;
        } else {
            tests_failed++;
        }
    }
    
    TEST("Different order books produce different hashes");
    {
        uint64_t hash1 = run_replay();
        
        OrderBook *ob = ob_create(MAX_LEVELS, MAX_ORDERS);
        ob_add_order(ob, 1, 200000000, 100000000, 1000);
        ob_add_order(ob, 2, 200000000, 500000000, 1001);
        ob_add_order(ob, 3, 190000000, 300000000, 1002);
        ob_add_order(ob, 4, 180000000, 800000000, 1003);
        ob_add_order(ob, 5, -201000000, 600000000, 1004);
        ob_add_order(ob, 6, -201000000, 100000000, 1005);
        ob_add_order(ob, 7, -202000000, 200000000, 1006);
        ob_add_order(ob, 8, -203000000, 900000000, 1007);
        /* No trades, no cancels — completely different operations */
        Snapshot snap;
        metrics_compute(ob, &snap, 3000);
        uint64_t hash2 = ob_hash_state(ob) ^ ob_hash_snapshot(&snap);
        ob_destroy(ob);
        
        if (hash1 != hash2) PASS();
        else FAIL("different states should not hash the same");
    }
    printf("\n=== Results: %d passed, %d failed ===\n", tests_passed, tests_failed);
    return tests_failed > 0 ? 1 : 0;
}
