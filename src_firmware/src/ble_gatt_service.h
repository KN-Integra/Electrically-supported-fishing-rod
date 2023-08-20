#ifndef BLUETOOTH_APP_H
#define BLUETOOTH_APP_H

#include <zephyr/types.h>

void estimates_notify(uint8_t* estimate_buff , size_t num_of_bytes);
void hrs_notify(void);
int bt_innit(void);


#endif /* BLUETOOTH_APP_H */