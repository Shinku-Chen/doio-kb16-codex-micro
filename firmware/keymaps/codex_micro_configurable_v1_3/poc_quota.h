#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void poc_quota_reset(void);
void poc_quota_parse(const char *json);
bool poc_quota_get(uint8_t *five_hour, uint8_t *weekly, char *credits, size_t credits_len);
