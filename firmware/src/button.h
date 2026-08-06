/* Checklist row buttons — one tactile button per row (spec extension: see plan for the
 * physical-button checklist feature). Debounced GPIO interrupts feed a pending bitmask
 * that both wakes the DEEP_SLEEP loop early (main.c) and backs the button_event GATT
 * characteristic (ble_service.c). */

#ifndef BUTTON_H_
#define BUTTON_H_

#include <stdint.h>

#include <zephyr/kernel.h>

#define BUTTON_COUNT 5

/* Configures all row-button GPIOs as inputs with edge interrupts. wake_sem is given
 * (k_sem_give) from the ISR on every accepted (post-debounce) press, so main.c's
 * DEEP_SLEEP sleep call can be interrupted early instead of waiting for the next RTC
 * timeout. Returns 0 on success. */
int button_init(struct k_sem *wake_sem);

/* Atomically reads and clears the pending-press bitmask: bit i (0-4) set means row i's
 * button was pressed since the last call. Backs the button_event characteristic's read
 * callback directly — always fresh, no cached setter needed (unlike battery/status). */
uint8_t button_consume_pending_mask(void);

#endif /* BUTTON_H_ */
