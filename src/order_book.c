#include "order_book.h"
#include <stdlib.h>
#include <string.h>

#define INITIAL_MAP_CAPACITY 1024

static size_t hash_order_id(order_id_t id, size_t capacity) {
    return (id * 11400714819323198485ULL) % capacity;
}

static OrderMap* order_map_create(size_t capacity) {
    OrderMap *map = malloc(sizeof(OrderMap));
    map->entries = calloc(capacity, sizeof(OrderEntry));
    map->capacity = capacity;
    map->size = 0;
    return map;
}

static void order_map_destroy(OrderMap *map) {
    if (!map) return;
    free(map->entries);
    free(map);
}

static int order_map_insert(OrderMap *map, order_id_t id, OrderNode *node) {
    if (map->size >= map->capacity * 0.75) {
        size_t new_capacity = map->capacity * 2;
        OrderEntry *new_entries = calloc(new_capacity, sizeof(OrderEntry));
        for (size_t i = 0; i < map->capacity; i++) {
            if (map->entries[i].key != 0) {
                size_t new_idx = hash_order_id(map->entries[i].key, new_capacity);
                while (new_entries[new_idx].key != 0) {
                    new_idx = (new_idx + 1) % new_capacity;
                }
                new_entries[new_idx] = map->entries[i];
            }
        }
        free(map->entries);
        map->entries = new_entries;
        map->capacity = new_capacity;
    }
    
    size_t idx = hash_order_id(id, map->capacity);
    while (map->entries[idx].key != 0 && map->entries[idx].key != id) {
        idx = (idx + 1) % map->capacity;
    }
    if (map->entries[idx].key == 0) map->size++;
    map->entries[idx].key = id;
    map->entries[idx].value = node;
    return 0;
}

static OrderNode* order_map_lookup(OrderMap *map, order_id_t id) {
    size_t idx = hash_order_id(id, map->capacity);
    while (map->entries[idx].key != 0) {
        if (map->entries[idx].key == id) {
            return map->entries[idx].value;
        }
        idx = (idx + 1) % map->capacity;
    }
    return NULL;
}

static int order_map_remove(OrderMap *map, order_id_t id) {
    size_t idx = hash_order_id(id, map->capacity);
    while (map->entries[idx].key != 0) {
        if (map->entries[idx].key == id) {
            map->entries[idx].key = 0;
            map->entries[idx].value = NULL;
            map->size--;
            return 0;
        }
        idx = (idx + 1) % map->capacity;
    }
    return -1;
}

OrderBook* ob_create(size_t max_price_levels, size_t max_orders) {
    OrderBook *ob = malloc(sizeof(OrderBook));
    ob->bids = calloc(max_price_levels, sizeof(PriceLevel));
    ob->asks = calloc(max_price_levels, sizeof(PriceLevel));
    ob->bid_count = 0;
    ob->ask_count = 0;
    ob->max_levels = max_price_levels;
    ob->order_map = order_map_create(max_orders > INITIAL_MAP_CAPACITY ? max_orders : INITIAL_MAP_CAPACITY);
    ob->best_bid = 0;
    ob->best_ask = 0;
    ob->total_bid_volume = 0;
    ob->total_ask_volume = 0;
    return ob;
}

void ob_destroy(OrderBook *ob) {
    if (!ob) return;
    for (size_t i = 0; i < ob->bid_count; i++) {
        OrderNode *node = ob->bids[i].head;
        while (node) {
            OrderNode *next = node->next;
            free(node);
            node = next;
        }
    }
    for (size_t i = 0; i < ob->ask_count; i++) {
        OrderNode *node = ob->asks[i].head;
        while (node) {
            OrderNode *next = node->next;
            free(node);
            node = next;
        }
    }
    free(ob->bids);
    free(ob->asks);
    order_map_destroy(ob->order_map);
    free(ob);
}

int ob_add_order(OrderBook *ob, order_id_t id, price_t price, volume_t qty, uint64_t ts) {
    if (qty <= 0 || id == 0) return -1;
    
    if (order_map_lookup(ob->order_map, id)) return -1;
    
    OrderNode *node = malloc(sizeof(OrderNode));
    if (!node) return -1;
    
    node->order.id = id;
    node->order.price = price;
    node->order.quantity = qty;
    node->order.filled = 0;
    node->order.timestamp_ns = ts;
    node->next = NULL;
    node->prev = NULL;
    
    int is_bid = (price > 0);
    PriceLevel *levels = is_bid ? ob->bids : ob->asks;
    size_t *count = is_bid ? &ob->bid_count : &ob->ask_count;
    
    price_t abs_price = is_bid ? price : -price;
    
    int found = -1;
    for (size_t i = 0; i < *count; i++) {
        if (levels[i].price == abs_price) {
            found = i;
            break;
        }
    }
    
    if (found >= 0) {
        if (levels[found].tail) {
            levels[found].tail->next = node;
            node->prev = levels[found].tail;
        } else {
            levels[found].head = node;
        }
        levels[found].tail = node;
        levels[found].total_volume += qty;
    } else {
        if (*count >= ob->max_levels) {
            free(node);
            return -1;
        }
        
        size_t insert_idx = 0;
        if (is_bid) {
            for (size_t i = 0; i < *count; i++) {
                if (abs_price > levels[i].price) {
                    insert_idx = i;
                    break;
                }
                insert_idx = i + 1;
            }
        } else {
            for (size_t i = 0; i < *count; i++) {
                if (abs_price < levels[i].price) {
                    insert_idx = i;
                    break;
                }
                insert_idx = i + 1;
            }
        }
        
        for (size_t i = *count; i > insert_idx; i--) {
            levels[i] = levels[i - 1];
        }
        
        levels[insert_idx].price = abs_price;
        levels[insert_idx].head = node;
        levels[insert_idx].tail = node;
        levels[insert_idx].total_volume = qty;
        (*count)++;
    }
    
    order_map_insert(ob->order_map, id, node);
    
    if (is_bid) {
        ob->total_bid_volume += qty;
        ob->best_bid = ob->bids[0].price;
    } else {
        ob->total_ask_volume += qty;
        ob->best_ask = ob->asks[0].price;
    }
    
    return 0;
}

int ob_cancel_order(OrderBook *ob, order_id_t id) {
    OrderNode *node = order_map_lookup(ob->order_map, id);
    if (!node) return -1;
    
    price_t price = node->order.price;
    volume_t qty = node->order.quantity - node->order.filled;
    
    int is_bid = (price > 0);
    PriceLevel *levels = is_bid ? ob->bids : ob->asks;
    size_t *count = is_bid ? &ob->bid_count : &ob->ask_count;
    price_t abs_price = is_bid ? price : -price;
    
    int level_idx = -1;
    for (size_t i = 0; i < *count; i++) {
        if (levels[i].price == abs_price) {
            level_idx = i;
            break;
        }
    }
    
    if (level_idx < 0) return -1;
    
    if (node->prev) {
        node->prev->next = node->next;
    } else {
        levels[level_idx].head = node->next;
    }
    
    if (node->next) {
        node->next->prev = node->prev;
    } else {
        levels[level_idx].tail = node->prev;
    }
    
    levels[level_idx].total_volume -= qty;
    
    if (levels[level_idx].total_volume == 0) {
        for (size_t i = level_idx; i < *count - 1; i++) {
            levels[i] = levels[i + 1];
        }
        (*count)--;
    }
    
    if (is_bid) {
        ob->total_bid_volume -= qty;
        ob->best_bid = (*count > 0) ? ob->bids[0].price : 0;
    } else {
        ob->total_ask_volume -= qty;
        ob->best_ask = (*count > 0) ? ob->asks[0].price : 0;
    }
    
    order_map_remove(ob->order_map, id);
    free(node);
    
    return 0;
}

int ob_modify_order(OrderBook *ob, order_id_t id, volume_t new_qty) {
    OrderNode *node = order_map_lookup(ob->order_map, id);
    if (!node || new_qty < node->order.filled) return -1;
    
    volume_t old_qty = node->order.quantity;
    volume_t delta = new_qty - old_qty;
    node->order.quantity = new_qty;
    
    int is_bid = (node->order.price > 0);
    if (is_bid) {
        ob->total_bid_volume += delta;
    } else {
        ob->total_ask_volume += delta;
    }
    
    return 0;
}

int ob_execute_trade(OrderBook *ob, price_t price, volume_t qty, uint64_t ts) {
    (void)ob; (void)price; (void)qty; (void)ts;
    /* TODO: implement matching engine */
    return 0;
}

price_t ob_get_best_bid(OrderBook *ob) {
    return ob->bid_count > 0 ? ob->bids[0].price : 0;
}

price_t ob_get_best_ask(OrderBook *ob) {
    return ob->ask_count > 0 ? ob->asks[0].price : 0;
}

volume_t ob_get_bid_volume_at(OrderBook *ob, price_t price) {
    for (size_t i = 0; i < ob->bid_count; i++) {
        if (ob->bids[i].price == price) return ob->bids[i].total_volume;
    }
    return 0;
}

volume_t ob_get_ask_volume_at(OrderBook *ob, price_t price) {
    for (size_t i = 0; i < ob->ask_count; i++) {
        if (ob->asks[i].price == price) return ob->asks[i].total_volume;
    }
    return 0;
}
