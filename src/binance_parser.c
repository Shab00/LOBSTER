#include "order_book.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <cjson/cJSON.h>

int parse_binance_depth(OrderBook *ob, const char *json_str) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) return -1;
    
    cJSON *bids = cJSON_GetObjectItem(root, "bids");
    cJSON *asks = cJSON_GetObjectItem(root, "asks");
    
    if (!bids || !asks) {
        cJSON_Delete(root);
        return -1;
    }
    
    order_id_t order_id = 1;
    
    cJSON *bid = NULL;
    cJSON_ArrayForEach(bid, bids) {
        if (cJSON_IsArray(bid) && bid->child && bid->child->next) {
            const char *price_str = bid->child->valuestring;
            const char *qty_str = bid->child->next->valuestring;
            
            if (price_str && qty_str) {
                price_t price = (price_t)(atof(price_str) * 100000000.0);
                volume_t qty = (volume_t)(atof(qty_str) * 100000000.0);
                ob_add_order(ob, order_id++, price, qty, 0);
            }
        }
    }
    
    cJSON *ask = NULL;
    cJSON_ArrayForEach(ask, asks) {
        if (cJSON_IsArray(ask) && ask->child && ask->child->next) {
            const char *price_str = ask->child->valuestring;
            const char *qty_str = ask->child->next->valuestring;
            
            if (price_str && qty_str) {
                price_t price = -(price_t)(atof(price_str) * 100000000.0);
                volume_t qty = (volume_t)(atof(qty_str) * 100000000.0);
                ob_add_order(ob, order_id++, price, qty, 0);
            }
        }
    }
    
    cJSON_Delete(root);
    return 0;
}

int parse_binance_depth_file(OrderBook *ob, const char *filename) {
    FILE *f = fopen(filename, "r");
    if (!f) return -1;
    
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    
    char *buffer = malloc(size + 1);
    if (!buffer) {
        fclose(f);
        return -1;
    }
    
    size_t read = fread(buffer, 1, size, f);
    fclose(f);
    
    if (read != (size_t)size) {
        free(buffer);
        return -1;
    }
    
    buffer[size] = '\0';
    int result = parse_binance_depth(ob, buffer);
    free(buffer);
    return result;
}
