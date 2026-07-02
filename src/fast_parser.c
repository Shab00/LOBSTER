#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static inline int64_t parse_price(const char *str) {
    int64_t result = 0;
    int64_t decimal_part = 0;
    int decimal_places = 0;
    int is_negative = 0;
    
    if (!str) return 0;
    if (*str == '-') {
        is_negative = 1;
        str++;
    }
    
    while (*str && *str != '.') {
        if (isdigit(*str)) {
            result = result * 10 + (*str - '0');
        }
        str++;
    }
    
    if (*str == '.') {
        str++;
        while (*str && isdigit(*str) && decimal_places < 8) {
            decimal_part = decimal_part * 10 + (*str - '0');
            decimal_places++;
            str++;
        }
    }
    
    while (decimal_places < 8) {
        decimal_part *= 10;
        decimal_places++;
    }
    
    result = result * 100000000 + decimal_part;
    
    return is_negative ? -result : result;
}

int64_t fast_parse_decimal(const char *str) {
    return parse_price(str);
}
