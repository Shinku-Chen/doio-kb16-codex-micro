#include QMK_KEYBOARD_H

#include "poc_quota.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef POC_OLED_QUOTA

static bool    quota_valid;
static uint8_t quota_five_hour;
static uint8_t quota_weekly;
static char    quota_credits[8];

static bool find_u32(const char *json, const char *key, uint32_t *out) {
    if (json == NULL) {
        return false;
    }

    char pattern[16];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    const char *cursor = strstr(json, pattern);
    if (cursor == NULL) {
        return false;
    }

    cursor = strchr(cursor, ':');
    if (cursor == NULL) {
        return false;
    }
    ++cursor;
    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }
    if (*cursor == '"') {
        ++cursor;
    }

    char *end = NULL;
    unsigned long value = strtoul(cursor, &end, 10);
    if (end == cursor) {
        return false;
    }
    *out = (uint32_t)value;
    return true;
}

static void find_credits(const char *json, char *out, size_t out_len) {
    if (json == NULL || out_len == 0) {
        return;
    }

    const char *cursor = strstr(json, "\"cr\"");
    if (cursor == NULL) {
        return;
    }
    cursor = strchr(cursor, ':');
    if (cursor == NULL) {
        return;
    }
    ++cursor;
    while (*cursor == ' ' || *cursor == '\t') {
        ++cursor;
    }
    if (*cursor == '"') {
        ++cursor;
    }

    size_t index = 0;
    while (*cursor != '\0' && *cursor != '"' && *cursor != ',' && *cursor != '}' && index + 1 < out_len) {
        out[index++] = *cursor++;
    }
    out[index] = '\0';
}

void poc_quota_reset(void) {
    quota_valid     = false;
    quota_five_hour = 0;
    quota_weekly    = 0;
    quota_credits[0] = '\0';
}

void poc_quota_parse(const char *json) {
    poc_quota_reset();
    if (json == NULL) {
        return;
    }

    uint32_t five_hour = 0;
    uint32_t weekly    = 0;
    bool     has_five  = find_u32(json, "p5", &five_hour);
    bool     has_week  = find_u32(json, "p7", &weekly);

    if (has_five || has_week) {
        quota_valid     = true;
        quota_five_hour = has_five ? (uint8_t)(five_hour > 100 ? 100 : five_hour) : 0;
        quota_weekly    = has_week ? (uint8_t)(weekly > 100 ? 100 : weekly) : 0;
        find_credits(json, quota_credits, sizeof(quota_credits));
    }
}

bool poc_quota_get(uint8_t *five_hour, uint8_t *weekly, char *credits, size_t credits_len) {
    if (!quota_valid) {
        return false;
    }
    if (five_hour != NULL) {
        *five_hour = quota_five_hour;
    }
    if (weekly != NULL) {
        *weekly = quota_weekly;
    }
    if (credits != NULL && credits_len > 0) {
        strncpy(credits, quota_credits, credits_len - 1);
        credits[credits_len - 1] = '\0';
    }
    return true;
}

#endif
