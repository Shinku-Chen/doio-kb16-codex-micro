#pragma once

#include <stdbool.h>
#include <stdint.h>

void    poc_via_init(void);
uint8_t poc_via_layer_count(void);
uint8_t poc_via_layer_offset(void);
bool    poc_via_layer_allowed(uint8_t via_layer);
