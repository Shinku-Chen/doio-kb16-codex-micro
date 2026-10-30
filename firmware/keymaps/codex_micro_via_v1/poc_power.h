#pragma once

#include <stdbool.h>

void poc_power_init(void);
void poc_power_note_host_activity(void);
void poc_power_note_lighting(bool on);
bool poc_power_should_sleep(void);
