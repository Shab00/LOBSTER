#include "order_book.h"
#include "simulator.h"
#include <stdio.h>
#include <string.h>

int main() {
    const char *test_depth = "{"
        "\"bids\": [[\"10000.00\", \"5.0\"], [\"9999.50\", \"3.0\"]],"
        "\"asks\": [[\"10001.00\", \"2.0\"], [\"10002.00\", \"4.0\"]]}";
    
    OrderBook *ob = ob_create(100, 1000);
    SimulatorContext *ctx = simulator_init("/tmp/test_sim.csv");
    
    if (!ctx) {
        fprintf(stderr, "Failed to init simulator\n");
        ob_destroy(ob);
        return 1;
    }
    
    simulator_process_depth_snapshot(ob, ctx, test_depth, 1000);
    
    for (int i = 0; i < 10; i++) {
        simulator_process_trade(ob, ctx, 10000 * 100000000LL, 1000000, 1000 + i);
    }
    
    simulator_destroy(ctx);
    ob_destroy(ob);
    
    FILE *f = fopen("/tmp/test_sim.csv", "r");
    if (f) {
        char line[256];
        printf("\n=== Output CSV ===\n");
        while (fgets(line, sizeof(line), f)) {
            printf("%s", line);
        }
        fclose(f);
    }
    
    printf("\nSimulator test completed!\n");
    return 0;
}
