/* Battery monitoring via ADC on a VBAT divider (spec 4.5), plus charge status read
 * straight off the onboard charger IC's status pin (boards/xiao_ble.overlay's
 * charge-status-gpios) — no ADC or averaging involved for that half, just a GPIO level. */

#ifndef BATTERY_H_
#define BATTERY_H_

#include <stdbool.h>
#include <stdint.h>

int battery_init(void);

/* Reads the VBAT divider and returns battery_mv directly, plus battery_pct derived from
 * a simple linear map between empty/full millivolt thresholds. Swap the linear map for
 * a real discharge-curve lookup once real battery data exists, or a fuel-gauge IC
 * (MAX17048) per spec 4.5's suggested upgrade path.
 *
 * *out_charging comes from the charger IC's status pin, not derived from battery_mv —
 * on boards without charge-status-gpios in their overlay it's always reported false
 * rather than left uninitialized, so callers don't need a feature check of their own.
 * Returns 0 on success. */
int battery_read(uint8_t *out_battery_pct, uint16_t *out_battery_mv, bool *out_charging);

#endif /* BATTERY_H_ */
