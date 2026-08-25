# ESPHome GC9B72 Display Component

External display component for **ESPHome** with support for round **360×360 SPI displays based on the GalaxyCore GC9B72 controller**.

This component allows GC9B72 displays to be used directly with ESPHome's display system, including support for lambdas, fonts, images, shapes, text, and other standard ESPHome display features.

The project is based on the excellent [Arduino_GC9B72](https://github.com/MaliosDark/Arduino_GC9B72) driver by [MaliosDark](https://github.com/MaliosDark), which provides the initialization sequence and low-level support for the GC9B72 controller.

The original Arduino driver has been adapted to work as a native ESPHome external component.

## Supported Display

This component is intended for round TFT displays with:

* Controller: **GalaxyCore GC9B72**
* Resolution: **360×360**
* Display size: **2.1 inch**
* Interface: **4-wire SPI**

Typical PCB markings may include:

```text
VER:TFT 2.1 0_10
Driver IC: GC9B72
Resolution: 360x360
```

> [!WARNING]
> This component is specifically designed for the **GC9B72** controller. Similar-looking displays using GC9A01, GC9C01, ST7789, ST77916, or other controllers require different drivers.

---

# Display Pinout

Typical display pinout:

| Display Pin | Description         |
| ----------- | ------------------- |
| GND         | Ground              |
| VCC         | 3.3V power          |
| SCL         | SPI Clock           |
| SDA         | SPI MOSI            |
| RST         | Reset               |
| DC          | Data / Command      |
| CS          | Chip Select         |
| BL          | Backlight           |
| SDO         | SPI MISO / Readback |
| TE          | Tearing Effect      |

Only the following pins are normally required:

```text
GND
VCC
SCL
SDA
RST
DC
CS
BL
```

`SDO` and `TE` can usually remain unconnected.

> [!IMPORTANT]
> Use **3.3V logic**. Do not connect the display directly to 5V unless your specific display board explicitly includes a voltage regulator and level shifting.

---

# Installation

Add the repository as an ESPHome external component:

```yaml
external_components:
  - source:
      type: git
      url: https://github.com/AndreM-1230/esphome-gc9b72
      ref: main
    components: [gc9b72]
```

---

# Example Configuration

Example configuration for an ESP32-S3:

```yaml
esphome:
  name: gc9b72-display

esp32:
  board: esp32-s3-devkitc-1
  framework:
    type: arduino

external_components:
  - source:
      type: git
      url: https://github.com/AndreM-1230/esphome-gc9b72
      ref: main
    components: [gc9b72]

display:
  - platform: gc9b72
    id: my_display

    clk_pin: 5
    mosi_pin: 6
    cs_pin: 12
    dc_pin: 11
    reset_pin: 10

    rotation: 0

    data_rate: 20000000

    lambda: |-
      it.fill(Color::BLACK);

      it.print(
        180,
        180,
        id(my_font),
        Color::WHITE,
        TextAlign::CENTER,
        "Hello!"
      );

font:
  - file: "gfonts://Roboto"
    id: my_font
    size: 40
```

Change the GPIO pins to match your own wiring.

---

# Configuration Variables

## `clk_pin`

**Required.**

The SPI clock pin connected to the display's `SCL` pin.

Example:

```yaml
clk_pin: 5
```

---

## `mosi_pin`

**Required.**

The SPI MOSI pin connected to the display's `SDA` pin.

Example:

```yaml
mosi_pin: 6
```

---

## `cs_pin`

**Required.**

Chip Select pin.

Example:

```yaml
cs_pin: 12
```

---

## `dc_pin`

**Required.**

Data/Command pin.

Example:

```yaml
dc_pin: 11
```

---

## `reset_pin`

**Required.**

Display reset pin.

Example:

```yaml
reset_pin: 10
```

---

## `rotation`

Optional display rotation.

```yaml
rotation: 0
```

Supported values:

```text
0
90
180
270
```

---

## `data_rate`

Optional SPI clock speed.

Default:

```text
20000000
```

Example:

```yaml
data_rate: 20000000
```

If you experience display artifacts, noise, or unstable output when using long wires, try reducing the SPI clock speed:

```yaml
data_rate: 10000000
```

---

## `refresh`

Controls how often the display lambda is executed.

For displays that need frequent updates:

```yaml
refresh: 1s
```

For applications where the display is updated manually or continuously:

```yaml
refresh: 0s
```

---

# Example Display Lambda

The component integrates with the standard ESPHome display API.

For example:

```yaml
display:
  - platform: gc9b72
    id: my_display

    clk_pin: 5
    mosi_pin: 6
    cs_pin: 12
    dc_pin: 11
    reset_pin: 10

    lambda: |-
      it.fill(Color::BLACK);

      it.circle(
        180,
        180,
        170,
        Color::WHITE
      );

      it.print(
        180,
        180,
        id(my_font),
        Color::WHITE,
        TextAlign::CENTER,
        "GC9B72"
      );
```

All standard ESPHome drawing functions can be used, including:

* `it.print()`
* `it.printf()`
* `it.line()`
* `it.rectangle()`
* `it.filled_rectangle()`
* `it.circle()`
* `it.filled_circle()`
* `it.image()`
* `it.fill()`

---

# Backlight

The display backlight is connected to the `BL` pin.

The GC9B72 component does not automatically manage the backlight pin, so it can be configured separately in ESPHome.

Example:

```yaml
output:
  - platform: ledc
    pin: 17 # BLK
    id: backlight_pwm

light:
  - platform: monochromatic
    output: backlight_pwm
    name: "Display Backlight"
    restore_mode: ALWAYS_ON
```

---

# Troubleshooting

## Black Screen

Check the following:

* The display is powered correctly.
* The display uses **3.3V logic**.
* The `BL` backlight pin is enabled.
* `SCL` and `SDA` are connected correctly.
* `DC`, `CS`, and `RST` match the ESPHome configuration.
* Your display actually uses the **GC9B72** controller.

The easiest way to verify the backlight is to temporarily connect `BL` to 3.3V.

---

## Garbage or Incorrect Colors

Make sure that your display is actually using the GC9B72 controller.

Many visually identical round displays use other controllers such as:

* GC9A01
* GC9C01
* ST7789
* ST77916

These controllers require different initialization sequences.

If the image contains artifacts or unstable pixels, try lowering the SPI speed:

```yaml
data_rate: 10000000
```

---

# Credits

This project is based on:

* [Arduino_GC9B72](https://github.com/MaliosDark/Arduino_GC9B72) by [MaliosDark](https://github.com/MaliosDark)

The original project provides the Arduino driver and the GC9B72 initialization sequence used as the basis for this ESPHome component.

The initialization sequence in the original driver is based on the GC9B72 reference driver from the xboot project.

This repository adapts the GC9B72 driver for direct use with the ESPHome display framework.

---

# License

This project is based on software released under the MIT License.

Please see the repository license for details.
