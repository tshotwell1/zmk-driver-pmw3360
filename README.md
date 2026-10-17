# PMW3389 optical mouse sensor driver for ZMK

A ZMK driver for the Pixart PMW3389 optical mouse sensor. This sensor has relativley high power consumption, its not recommended for wireless builds.

This started as a PMW3360 driver. The two sensors are near identical, but this
fork targets the PMW3389 specifically, which differs in two ways that matter:

- CPI is set through the 16-bit `Resolution_L`/`Resolution_H` register pair in
  50 CPI steps (50 - 16000), rather than the PMW3360's single `Config1`
  register in 100 CPI steps (100 - 12000).
- Register `0x0D` is `Ripple_Control`, not the PMW3360's `Control` register. The
  PMW3389 has no orientation register at all, so orientation is handled in
  software by this driver.

The devicetree `compatible` is still `pixart,pmw3360`, and the Kconfig symbols
are still named `..._PMW3360_...`, so existing configurations do not need to
change.

## Features

- Configurable CPI
- Configurable orientation, at any whole-degree angle
- Can run in polled mode (don't define irq-gpios) or interrupt mode (recommended if you have a motion pin wired up)
- Can run on either the system work queue, or a private one.

## Orientation

If the sensor is not mounted square to the keyboard, set `rotation` to the
angle it is turned by, in whole degrees:

```
rotation = <45>;
```

The driver rotates each motion report by that angle before reporting it, so the
cursor moves the way your hand does. A few notes:

- The rotation is applied *before* `invert-x` / `invert-y`.
- If the motion comes out mirrored rather than corrected, use `360 - angle`
  (so `<315>` instead of `<45>`). Which one you want depends on which way the
  sensor is turned, so it is usually quicker to try than to reason about.
- `rotate-90`, `rotate-180` and `rotate-270` still work, as aliases for the
  equivalent `rotation` values. Multiples of 90 degrees are exact.
- The maths is fixed point, and the fractional part of each report is carried
  over to the next one, so slow movement is not lost to rounding. At angles
  that are not multiples of 90 degrees you are mixing two axes, so expect a
  count or so of jitter on a perfectly straight drag. This is inherent to
  rotating integer deltas, not a bug.
- `angle-tune` does the same job inside the sensor, for free, but only covers
  +/-30 degrees. Use it if your angle is within that range and you want to save
  the handful of cycles per report.

## Config options

- `CONFIG_INPUT_PIXART_PMW3360_USE_OWN_THREAD` - process events on a private work queue. This will consume additional memory, but should improve responsiveness (default: y).
- `CONFIG_INPUT_PIXART_PMW3360_THREAD_PRIORITY` - the priority of the drivers work queue thread. Lower values take precedence (default: 5).
- `CONFIG_INPUT_PIXART_PMW3360_THREAD_STACK_SIZE` - the amount of memory to reserve for the drivers work queue thread (default: 768).

## Sample configuration

Use pinctrl to configure your SPI pins (Chip select, Clock, MOSI and MISO).

```
&pinctrl {
	spi0_default: spi0_default {
		group1 {
			pinmux = <SPI0_CSN_P21>, <SPI0_SCK_P22>, <SPI0_TX_P23>;
		};
		group2 {
			pinmux = <SPI0_RX_P20>;
			input-enable;
		};
	};
};
```

Assign the driver to the SPI bus, configure the CPI and orientation here. If you do not have a motion pin wired up, remove `irq-gpios` to run in polled mode.

```
&spi0 {
	status = "okay";
	cs-gpios = <&gpio0 21 GPIO_ACTIVE_LOW>;
	mouse: mouse@0 {
		compatible = "pixart,pmw3360";
		status = "okay";
		reg = <0>;
		spi-max-frequency = <2000000>;

		// pmw3360 driver parameters
		irq-gpios = <&gpio0 27 (GPIO_ACTIVE_LOW | GPIO_PULL_UP)>;
		cpi = <600>;
		rotation = <45>;
	};
};
```

Create and input listener to process the mouse events.

```
/ {
	mouse_listener {
		compatible = "zmk,input-listener";
		device = <&mouse>;
	};
};
```