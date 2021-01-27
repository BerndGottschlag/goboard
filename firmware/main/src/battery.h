
#ifndef BATTERY_H_INCLUDED
#define BATTERY_H_INCLUDED

void battery_init(void);
int battery_is_low(void);
void battery_prepare_system_off(void);

#endif

