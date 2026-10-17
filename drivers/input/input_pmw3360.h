/*
 * Copyright (c) 2025 George Norton
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/spi.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/sensor.h>

#ifdef __cplusplus
extern "C" {
#endif

// Indicates the direction of a SPI transaction
#define PMW3360_SPI_READ 0
#define PMW3360_SPI_WRITE 0x80

// Timing values from the datasheet
#define PMW3360_T_SRAD 160
#define PMW3360_T_SCLK_NCS_READ 120
#define PMW3360_T_SCLK_NCS_WRITE 35
#define PMW3360_T_SRR 20  // Same as T_SRW
#define PMW3360_T_SWR 180 // Same as T_SWW
#define PMW3360_T_SRAD_MOTBR 35

// Select register addresses and values.
//
// NOTE: this is the PMW3389 register map. It is almost identical to the
// PMW3360, but there are two differences which matter here:
//  - 0x0D is Ripple_Control on the PMW3389. On the PMW3360 it is Control,
//    which carries the invert-X/invert-Y/swap-XY bits. The PMW3389 has no
//    equivalent register, so orientation is handled in software instead.
//  - CPI is set through the 16-bit Resolution_L/Resolution_H pair in 50 CPI
//    steps. On the PMW3360 it is a single Config1 register in 100 CPI steps.
#define PMW3360_REG_PRODUCT_ID          0x00
#   define PMW3389_PRODUCT_ID           0x47
#define PMW3360_REG_REVISION_ID         0x01
#define PMW3360_REG_MOTION              0x02
#   define PMW3360_MOTION_MOT           0x80
#define PMW3360_REG_DELTA_X_L           0x03
#define PMW3360_REG_DELTA_X_H           0x04
#define PMW3360_REG_DELTA_Y_L           0x05
#define PMW3360_REG_DELTA_Y_H           0x06
#define PMW3360_REG_RIPPLE_CONTROL      0x0D
#define PMW3389_REG_RESOLUTION_L        0x0E
#define PMW3389_REG_RESOLUTION_H        0x0F
#   define PMW3389_CPI_STEP             50
#   define PMW3389_CPI_MIN              50
#   define PMW3389_CPI_MAX              16000
#define PMW3360_REG_CONFIG_2            0x10
#   define PMW3360_CONFIG_2_RTP_MOD     0x04
#   define PWM3360_CONFIG_2_REST_EN     0x20
#define PMW3360_REG_ANGLE_TUNE          0x11
#define PMW3360_REG_POWER_UP            0x3A
#define PMW3360_REG_INVERSE_PRODUCT_ID  0x3F
#define PMW3360_REG_MOTION_BURST        0x50
#define PMW3360_REG_LIFT_CONFIG         0x63
#   define PMW3360_LIFT_CONFIG_2MM      0x02
#   define PMW3360_LIFT_CONFIG_3MM      0x03

// Fixed point scale used by the software rotation. Deltas are multiplied by
// sin/cos values scaled by this factor, then divided back down.
#define PMW3360_TRIG_SCALE 4096

// Runtime configurable attributes
enum pmw3360_attributes {
    PMW3360_ATTR_CPI
};

// The motion burst report, this can be exteneded to read more
// data, but we dont use it, so we dont bother reading it.
struct motion_burst {
    uint8_t motion;
    uint8_t observation;
    uint8_t delta_x_l;
    uint8_t delta_x_h;
    uint8_t delta_y_l;
    uint8_t delta_y_h;
/*  uint8_t squal;
    uint8_t raw_data_sum;
    uint8_t maximum_raw_data;
    uint8_t minimum_raw_data;
    uint8_t shutter_upper;
    uint8_t shutter_lower; */
} __packed;

// The drivers private data
struct pmw3360_data {
    const struct device *dev;
    struct k_mutex mutex; // Prevent set_attr from changing things while we are initializing
    bool ready;
    uint16_t cpi;
    bool motion_burst_active;
    bool polling_mode;
    uint32_t last_poll_cycles;
    // Software rotation state. sin/cos are scaled by PMW3360_TRIG_SCALE and
    // computed once during initialization. The residuals carry the fractional
    // part of a count over to the next report so that slow movement across a
    // rotated axis is not lost to rounding.
    int16_t sin_rotation;
    int16_t cos_rotation;
    int32_t residual_x;
    int32_t residual_y;
#if defined(CONFIG_INPUT_PIXART_PMW3360_USE_OWN_THREAD)
    struct k_work_q driver_work_queue;
#endif
    struct k_work motion_work;
    struct k_work_delayable init_work;
    struct gpio_callback irq_gpio_cb; // motion pin irq callback
};

// The drivers configuration struture - populated by the device tree
struct pmw3360_config {
    struct spi_dt_spec spi;
    struct gpio_dt_spec cs_gpio;
    struct gpio_dt_spec irq_gpio;
    struct gpio_dt_spec reset_gpio;
    uint16_t cpi;
    uint16_t rotation; // Degrees, 0 - 359
    int8_t angle_tune;
    bool lift_height_3mm;
    uint32_t polling_interval;
    bool invert_x;
    bool invert_y;
};

#ifdef __cplusplus
}
#endif
