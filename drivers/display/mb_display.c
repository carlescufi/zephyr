/*
 * Copyright (c) 2017 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/*
 * References:
 *
 * https://www.microbit.co.uk/device/screen
 * https://lancaster-university.github.io/microbit-docs/ubit/display/
 */

#include <zephyr.h>
#include <init.h>
#include <drivers/gpio.h>
#include <device.h>
#include <string.h>
#include <sys/printk.h>

#include <display/mb_display.h>

#include "mb_font.h"

#define MODE_MASK    BIT_MASK(16)

#if defined(CONFIG_BOARD_BBC_MICROBIT_V2)

#define GPIO_PORT_COUNT 2
#define LED_GPIO_PORT0 DT_LABEL(DT_NODELABEL(gpio0))
#define LED_GPIO_PORT1 DT_LABEL(DT_NODELABEL(gpio1))

/* Onboard LED Row 2 */
#define LED_ROW1_GPIO_PIN   21
#define LED_ROW1_GPIO_PORT  0

/* Onboard LED Row 2 */
#define LED_ROW2_GPIO_PIN   22
#define LED_ROW2_GPIO_PORT  0

/* Onboard LED Row 3 */
#define LED_ROW3_GPIO_PIN   15
#define LED_ROW3_GPIO_PORT  0

/* Onboard LED Row 4 */
#define LED_ROW4_GPIO_PIN   24
#define LED_ROW4_GPIO_PORT  0

/* Onboard LED Row 5 */
#define LED_ROW5_GPIO_PIN   19
#define LED_ROW5_GPIO_PORT  0

/* Onboard LED Column 1 */
#define LED_COL1_GPIO_PIN   28
#define LED_COL1_GPIO_PORT  0

/* Onboard LED Column 2 */
#define LED_COL2_GPIO_PIN   11
#define LED_COL2_GPIO_PORT  0

/* Onboard LED Column 3 */
#define LED_COL3_GPIO_PIN   31
#define LED_COL3_GPIO_PORT  0

/* Onboard LED Column 4 */
#define LED_COL4_GPIO_PIN   5
#define LED_COL4_GPIO_PORT  1

/* Onboard LED Column 5 */
#define LED_COL5_GPIO_PIN   30
#define LED_COL5_GPIO_PORT  0

#define DISPLAY_ROWS 5
#define DISPLAY_COLS 5

#define ROW_PINS  {{LED_ROW1_GPIO_PIN, LED_ROW1_GPIO_PORT}, \
		   {LED_ROW2_GPIO_PIN, LED_ROW2_GPIO_PORT}, \
		   {LED_ROW3_GPIO_PIN, LED_ROW3_GPIO_PORT}, \
		   {LED_ROW4_GPIO_PIN, LED_ROW4_GPIO_PORT}, \
		   {LED_ROW5_GPIO_PIN, LED_ROW5_GPIO_PORT}}

#define COL_PINS  {{LED_COL1_GPIO_PIN, LED_COL1_GPIO_PORT}, \
		   {LED_COL2_GPIO_PIN, LED_COL2_GPIO_PORT}, \
		   {LED_COL3_GPIO_PIN, LED_COL3_GPIO_PORT}, \
		   {LED_COL4_GPIO_PIN, LED_COL4_GPIO_PORT}, \
		   {LED_COL5_GPIO_PIN, LED_COL5_GPIO_PORT}}
#else

#define GPIO_PORT_COUNT 1
#define LED_GPIO_PORT0 DT_LABEL(DT_NODELABEL(gpio0))

/* Onboard LED Row 1 */
#define LED_ROW1_GPIO_PIN   13

/* Onboard LED Row 2 */
#define LED_ROW2_GPIO_PIN   14

/* Onboard LED Row 3 */
#define LED_ROW3_GPIO_PIN   15

/* Onboard LED Column 1 */
#define LED_COL1_GPIO_PIN   4

/* Onboard LED Column 2 */
#define LED_COL2_GPIO_PIN   5

/* Onboard LED Column 3 */
#define LED_COL3_GPIO_PIN   6

/* Onboard LED Column 4 */
#define LED_COL4_GPIO_PIN   7

/* Onboard LED Column 5 */
#define LED_COL5_GPIO_PIN   8

/* Onboard LED Column 6 */
#define LED_COL6_GPIO_PIN   9

/* Onboard LED Column 7 */
#define LED_COL7_GPIO_PIN   10

/* Onboard LED Column 8 */
#define LED_COL8_GPIO_PIN   11

/* Onboard LED Column 9 */
#define LED_COL9_GPIO_PIN   12

#define DISPLAY_ROWS 3
#define DISPLAY_COLS 9

#endif

#define SCROLL_OFF   0
#define SCROLL_START 1

#define SCROLL_DEFAULT_DURATION_MS 80

struct mb_display {
	const struct device *dev;//[GPIO_PORT_COUNT];  /* GPIO device(s) */

	struct k_timer  timer;       /* Rendering timer */

	uint8_t            img_count;   /* Image count */

	uint8_t            cur_img;     /* Current image or character to show */

	uint8_t            scroll:3,    /* Scroll shift */
			first:1,     /* First frame of a scroll sequence */
			loop:1,      /* Loop to beginning */
			text:1,      /* We're showing a string (not image) */
			img_sep:1;   /* One column image separation */

	/* The following variables track the currently shown image */
	uint8_t            cur;         /* Currently rendered row */
	uint32_t           row[DISPLAY_ROWS]; /* Content (columns) for each row */
	int64_t           expiry;      /* When to stop showing current image */
	int32_t           duration;    /* Duration for each shown image */

	union {
		const struct mb_image *img; /* Array of images to show */
		const char            *str; /* String to be shown */
	};

	/* Buffer for printed strings */
	char            str_buf[CONFIG_MICROBIT_DISPLAY_STR_MAX];
};

struct x_y {
	uint8_t x:4,
	     y:4;
};

/* Where the X,Y coordinates of each row/col are found.
 * The top left corner has the coordinates 0,0.
 */
#if defined(CONFIG_BOARD_BBC_MICROBIT_V2)
#define RC_TO_X(row, col) col
#define RC_TO_Y(row, col) row
static const struct x_y map[DISPLAY_ROWS][DISPLAY_COLS] = {
	{{0, 0}, {1, 0}, {2, 0}, {3, 0}, {4, 0} },
	{{0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1} },
	{{0, 2}, {1, 2}, {2, 2}, {3, 2}, {4, 2} },
	{{0, 3}, {1, 3}, {2, 3}, {3, 3}, {4, 3} },
	{{0, 4}, {1, 4}, {2, 4}, {3, 4}, {4, 4} },
};
#else
static const struct x_y map[DISPLAY_ROWS][DISPLAY_COLS] = {
	{{0, 0}, {2, 0}, {4, 0}, {4, 3}, {3, 3}, {2, 3}, {1, 3}, {0, 3}, {1, 2} },
	{{4, 2}, {0, 2}, {2, 2}, {1, 0}, {3, 0}, {3, 4}, {1, 4}, {0, 0}, {0, 0} },
	{{2, 4}, {4, 4}, {0, 4}, {0, 1}, {1, 1}, {2, 1}, {3, 1}, {4, 1}, {3, 2} },
};
#define RC_TO_X(row, col) map[row][col].x
#define RC_TO_Y(row, col) map[row][col].y
#endif

static inline const struct mb_image *get_font(char ch)
{
	if (ch < MB_FONT_START || ch > MB_FONT_END) {
		return &mb_font[' ' - MB_FONT_START];
	}

	return &mb_font[ch - MB_FONT_START];
}

#define GET_PIXEL(img, x, y) ((img)->row[y] & BIT(x))

#if defined(CONFIG_BOARD_BBC_MICROBIT_V2)
struct pin_port {
	uint8_t pin:5,
		port:3;
};
static const struct pin_port row_pins[DISPLAY_ROWS] = ROW_PINS;
#define ROW_PIN(n) row_pins[n].pin
static const struct pin_port col_pins[DISPLAY_COLS] = COL_PINS;
#define COL_PIN(n) col_pins[n].pin
#else
#define ROW_PIN(n) (LED_ROW1_GPIO_PIN + (n))
#define COL_PIN(n) (LED_COL1_GPIO_PIN + (n))
#endif

/* Precalculate all three rows of an image and start the rendering. */
static void start_image(struct mb_display *disp, const struct mb_image *img)
{
	int row, col;

	for (row = 0; row < DISPLAY_ROWS; row++) {
		disp->row[row] = 0U;

		for (col = 0; col < DISPLAY_COLS; col++) {
			if (GET_PIXEL(img, RC_TO_X(row, col),
					   RC_TO_Y(row, col))) {
				disp->row[row] |= BIT(COL_PIN(col));
			}
		}
	}

	disp->cur = 0U;

	if (disp->duration == SYS_FOREVER_MS) {
		disp->expiry = SYS_FOREVER_MS;
	} else {
		disp->expiry = k_uptime_get() + disp->duration;
	}

	k_timer_start(&disp->timer, K_NO_WAIT, K_MSEC(4));
}

static inline void update_pins(struct mb_display *disp, uint32_t val)
{
	uint32_t pin, prev = (disp->cur + DISPLAY_ROWS - 1) % DISPLAY_ROWS;

	/* Disable the previous row */
	gpio_pin_set_raw(disp->dev, ROW_PIN(prev), 0);

	/* Set the column pins to their correct values */
#if defined(CONFIG_BOARD_BBC_MICROBIT_V2)
	uint8_t i;
	for (i = 0; i < DISPLAY_COLS; i++) {
		pin = col_pins[i].pin;
#else
	for (pin = LED_COL1_GPIO_PIN; pin <= LED_COL9_GPIO_PIN; pin++) {
#endif
		gpio_pin_set_raw(disp->dev, pin, !(val & BIT(pin)));
	}

	/* Enable the new row */
	gpio_pin_set_raw(disp->dev, ROW_PIN(disp->cur), 1);
}

static void reset_display(struct mb_display *disp)
{
	k_timer_stop(&disp->timer);

	disp->str = NULL;
	disp->cur_img = 0U;
	disp->img = NULL;
	disp->img_count = 0U;
	disp->scroll = SCROLL_OFF;
}

static const struct mb_image *current_img(struct mb_display *disp)
{
	if (disp->scroll && disp->first) {
		return get_font(' ');
	}

	if (disp->text) {
		return get_font(disp->str[disp->cur_img]);
	} else {
		return &disp->img[disp->cur_img];
	}
}

static const struct mb_image *next_img(struct mb_display *disp)
{
	if (disp->text) {
		if (disp->first) {
			return get_font(disp->str[0]);
		} else if (disp->str[disp->cur_img]) {
			return get_font(disp->str[disp->cur_img + 1]);
		} else {
			return get_font(' ');
		}
	} else {
		if (disp->first) {
			return &disp->img[0];
		} else if (disp->cur_img < (disp->img_count - 1)) {
			return &disp->img[disp->cur_img + 1];
		} else {
			return get_font(' ');
		}
	}
}

static inline bool last_frame(struct mb_display *disp)
{
	if (disp->text) {
		return (disp->str[disp->cur_img] == '\0');
	} else {
		return (disp->cur_img >= disp->img_count);
	}
}

static inline uint8_t scroll_steps(struct mb_display *disp)
{
	return 5 + disp->img_sep;
}

static void update_scroll(struct mb_display *disp)
{
	if (disp->scroll < scroll_steps(disp)) {
		struct mb_image img;
		int i;

		for (i = 0; i < 5; i++) {
			const struct mb_image *i1 = current_img(disp);
			const struct mb_image *i2 = next_img(disp);

			img.row[i] = ((i1->row[i] >> disp->scroll) |
				      (i2->row[i] << (scroll_steps(disp) -
						      disp->scroll)));
		}

		disp->scroll++;
		start_image(disp, &img);
	} else {
		if (disp->first) {
			disp->first = 0U;
		} else {
			disp->cur_img++;
		}

		if (last_frame(disp)) {
			if (!disp->loop) {
				reset_display(disp);
				return;
			}

			disp->cur_img = 0U;
			disp->first = 1U;
		}

		disp->scroll = SCROLL_START;
		start_image(disp, current_img(disp));
	}
}

static void update_image(struct mb_display *disp)
{
	disp->cur_img++;

	if (last_frame(disp)) {
		if (!disp->loop) {
			reset_display(disp);
			return;
		}

		disp->cur_img = 0U;
	}

	start_image(disp, current_img(disp));
}

static void show_row(struct k_timer *timer)
{
	struct mb_display *disp = CONTAINER_OF(timer, struct mb_display, timer);

	update_pins(disp, disp->row[disp->cur]);
	disp->cur = (disp->cur + 1) % DISPLAY_ROWS;

	if (disp->cur == 0U && disp->expiry != SYS_FOREVER_MS &&
	    k_uptime_get() > disp->expiry) {
		if (disp->scroll) {
			update_scroll(disp);
		} else {
			update_image(disp);
		}
	}
}

static void clear_display(struct k_timer *timer)
{
	struct mb_display *disp = CONTAINER_OF(timer, struct mb_display, timer);

	update_pins(disp, 0x00);
}

static struct mb_display display = {
	.timer = Z_TIMER_INITIALIZER(display.timer, show_row, clear_display),
};

static void start_scroll(struct mb_display *disp, int32_t duration)
{
	/* Divide total duration by number of scrolling steps */
	if (duration) {
		disp->duration = duration / scroll_steps(disp);
	} else {
		disp->duration = SCROLL_DEFAULT_DURATION_MS;
	}

	disp->scroll = SCROLL_START;
	disp->first = 1U;
	disp->cur_img = 0U;
	start_image(disp, get_font(' '));
}

static void start_single(struct mb_display *disp, int32_t duration)
{
	disp->duration = duration;

	if (disp->text) {
		start_image(disp, get_font(disp->str[0]));
	} else {
		start_image(disp, disp->img);
	}
}

void mb_display_image(struct mb_display *disp, uint32_t mode, int32_t duration,
		      const struct mb_image *img, uint8_t img_count)
{
	reset_display(disp);

	__ASSERT(img && img_count > 0, "Invalid parameters");

	disp->text = 0U;
	disp->img_count = img_count;
	disp->img = img;
	disp->img_sep = 0U;
	disp->cur_img = 0U;
	disp->loop = !!(mode & MB_DISPLAY_FLAG_LOOP);

	switch (mode & MODE_MASK) {
	case MB_DISPLAY_MODE_DEFAULT:
	case MB_DISPLAY_MODE_SINGLE:
		start_single(disp, duration);
		break;
	case MB_DISPLAY_MODE_SCROLL:
		start_scroll(disp, duration);
		break;
	default:
		__ASSERT(0, "Invalid display mode");
	}
}

void mb_display_stop(struct mb_display *disp)
{
	reset_display(disp);
}

void mb_display_print(struct mb_display *disp, uint32_t mode,
		      int32_t duration, const char *fmt, ...)
{
	va_list ap;

	reset_display(disp);

	va_start(ap, fmt);
	vsnprintk(disp->str_buf, sizeof(disp->str_buf), fmt, ap);
	va_end(ap);

	if (disp->str_buf[0] == '\0') {
		return;
	}

	disp->str = disp->str_buf;
	disp->text = 1U;
	disp->img_sep = 1U;
	disp->cur_img = 0U;
	disp->loop = !!(mode & MB_DISPLAY_FLAG_LOOP);

	switch (mode & MODE_MASK) {
	case MB_DISPLAY_MODE_DEFAULT:
	case MB_DISPLAY_MODE_SCROLL:
		start_scroll(disp, duration);
		break;
	case MB_DISPLAY_MODE_SINGLE:
		start_single(disp, duration);
		break;
	default:
		__ASSERT(0, "Invalid display mode");
	}
}

struct mb_display *mb_display_get(void)
{
	return &display;
}

static int mb_display_init(const struct device *dev)
{
	ARG_UNUSED(dev);

	display.dev = device_get_binding(DT_LABEL(DT_NODELABEL(gpio0)));

	__ASSERT(dev, "No GPIO device found");

#if defined(CONFIG_BOARD_BBC_MICROBIT_V2)
	uint8_t i;

	for (i = 0; i < DISPLAY_ROWS; i++) {
		gpio_pin_configure(display.dev, row_pins[i].pin, GPIO_OUTPUT);
	}

	for (i = 0; i < DISPLAY_COLS; i++) {
		gpio_pin_configure(display.dev, col_pins[i].pin, GPIO_OUTPUT);
	}
#else
	gpio_pin_configure(display.dev, LED_ROW1_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_ROW2_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_ROW3_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL1_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL2_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL3_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL4_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL5_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL6_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL7_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL8_GPIO_PIN, GPIO_OUTPUT);
	gpio_pin_configure(display.dev, LED_COL9_GPIO_PIN, GPIO_OUTPUT);
#endif

	return 0;
}

SYS_INIT(mb_display_init, POST_KERNEL, CONFIG_KERNEL_INIT_PRIORITY_DEVICE);
