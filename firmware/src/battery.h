/* Battery monitoring via ADC on a VBAT divider (spec 4.5). */

#ifndef BATTERY_H_
#define BATTERY_H_

#include <stdint.h>

int battery_init(void);

/* Reads the VBAT divider and returns battery_mv directly, plus battery_pct derived from
 * a simple linear map between empty/full millivolt thresholds. Swap the linear map for
 * a real discharge-curve lookup once real battery data exists, or a fuel-gauge IC
 * (MAX17048) per spec 4.5's suggested upgrade path. Returns 0 on success. */
int battery_read(uint8_t *out_battery_pct, uint16_t *out_battery_mv);

#endif /* BATTERY_H_ */
