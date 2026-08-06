#include "button.h"

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/atomic.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(button, CONFIG_LOG_DEFAULT_LEVEL);

/* Wired up in boards/xiao_ble.overlay's `zephyr,user` node as row0-button-gpios..
 * row4-button-gpios (D4/D5/D6/D7/D9) — see that file's header comment for pin sourcing
 * and why GPIO interrupts are enough to wake this firmware without any devicetree
 * "wakeup-source" property. */
#define BUTTON_NODE DT_PATH(zephyr_user)

/* ~50ms is generous for a mechanical tactile switch's contact bounce and short enough
 * that a deliberate double-press still registers as two presses. */
#define DEBOUNCE_MS 50

static const struct gpio_dt_spec buttons[BUTTON_COUNT] = {
	GPIO_DT_SPEC_GET(BUTTON_NODE, row0_button_gpios),
	GPIO_DT_SPEC_GET(BUTTON_NODE, row1_button_gpios),
	GPIO_DT_SPEC_GET(BUTTON_NODE, row2_button_gpios),
	GPIO_DT_SPEC_GET(BUTTON_NODE, row3_button_gpios),
	GPIO_DT_SPEC_GET(BUTTON_NODE, row4_button_gpios),
};

static struct gpio_callback button_cb[BUTTON_COUNT];
static atomic_t pending_mask = ATOMIC_INIT(0);
static int64_t last_press_ms[BUTTON_COUNT];
static struct k_sem *wake_sem_ref;

static void on_button_pressed(const struct device *port, struct gpio_callback *cb, uint32_t pins)
{
	ARG_UNUSED(port);
	ARG_UNUSED(pins);

	/* Each button gets its own callback struct (rather than one shared callback
	 * decoding a pin bitmask), so the matching struct's own array position tells us
	 * which row fired — simpler than reconciling pin numbers across gpio0 and gpio1
	 * (D4/D5 are on gpio0, D6/D7/D9 are on gpio1). */
	int index = (int)(cb - button_cb);

	if (index < 0 || index >= BUTTON_COUNT) {
		return;
	}

	int64_t now = k_uptime_get();
	if (now - last_press_ms[index] < DEBOUNCE_MS) {
		return;
	}
	last_press_ms[index] = now;

	atomic_or(&pending_mask, BIT(index));
	if (wake_sem_ref != NULL) {
		k_sem_give(wake_sem_ref);
	}
}

int button_init(struct k_sem *wake_sem)
{
	wake_sem_ref = wake_sem;

	for (int i = 0; i < BUTTON_COUNT; i++) {
		if (!gpio_is_ready_dt(&buttons[i])) {
			LOG_ERR("button %d: GPIO not ready", i);
			return -ENODEV;
		}

		int err = gpio_pin_configure_dt(&buttons[i], GPIO_INPUT);
		if (err) {
			LOG_ERR("button %d: configure failed (%d)", i, err);
			return err;
		}

		err = gpio_pin_interrupt_configure_dt(&buttons[i], GPIO_INT_EDGE_TO_ACTIVE);
		if (err) {
			LOG_ERR("button %d: interrupt configure failed (%d)", i, err);
			return err;
		}

		gpio_init_callback(&button_cb[i], on_button_pressed, BIT(buttons[i].pin));
		err = gpio_add_callback(buttons[i].port, &button_cb[i]);
		if (err) {
			LOG_ERR("button %d: add callback failed (%d)", i, err);
			return err;
		}
	}

	return 0;
}

uint8_t button_consume_pending_mask(void)
{
	return (uint8_t)atomic_set(&pending_mask, 0);
}
