#include "gc9b72.h"
#include "Arduino_GC9B72.h"

#include "esphome/core/log.h"
#include "esphome/components/display/display_color_utils.h"

namespace esphome {
namespace gc9b72 {

static const char *const TAG = "gc9b72";

GC9B72Display::GC9B72Display(
    int clk_pin,
    int mosi_pin,
    int cs_pin,
    int dc_pin,
    int reset_pin,
    uint8_t rotation,
    int32_t data_rate)
    : clk_pin_(clk_pin),
      mosi_pin_(mosi_pin),
      cs_pin_(cs_pin),
      dc_pin_(dc_pin),
      reset_pin_(reset_pin),
      rotation_(rotation),
      data_rate_(data_rate) {}

void GC9B72Display::setup() {
  /*
   * IMPORTANT:
   *
   * ESPHome DisplayBuffer normally uses one byte per color byte.
   *
   * We intentionally use ONE BYTE PER PIXEL here (RGB332)
   * instead of RGB565.
   *
   * 360 * 360 = 129600 bytes
   *
   * Old implementation:
   * 360 * 360 * 2 = 259200 bytes
   *
   * This saves ~127 KB of RAM.
   */
  this->init_internal_(360UL * 360UL);

  if (this->buffer_ == nullptr) {
    ESP_LOGE(TAG, "Framebuffer allocation failed");
    this->mark_failed();
    return;
  }

  /*
   * SPI bus.
   */
  this->bus_ = new Arduino_ESP32SPI(
      this->dc_pin_,
      this->cs_pin_,
      this->clk_pin_,
      this->mosi_pin_,
      GFX_NOT_DEFINED);

  if (this->bus_ == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate SPI bus");
    this->mark_failed();
    return;
  }

  /*
   * GC9B72 360x360.
   */
  this->gfx_ = new Arduino_GC9B72(
      this->bus_,
      this->reset_pin_,
      this->rotation_,
      false,
      360,
      360);

  if (this->gfx_ == nullptr) {
    ESP_LOGE(TAG, "Failed to allocate GC9B72 driver");
    this->mark_failed();
    return;
  }

  if (!this->gfx_->begin(this->data_rate_)) {
    ESP_LOGE(TAG, "GC9B72 initialization failed");
    this->mark_failed();
    return;
  }

  /*
   * Clear framebuffer.
   */
  this->fill(Color(0, 0, 0, 0));

  /*
   * Clear physical display.
   */
  this->gfx_->fillScreen(0x0000);

  ESP_LOGI(
      TAG,
      "GC9B72 initialized: 360x360, rotation=%u, SPI=%ld Hz",
      this->rotation_,
      static_cast<long>(this->data_rate_));

  ESP_LOGI(
      TAG,
      "Framebuffer: 129600 bytes RGB332");
}

void GC9B72Display::update() {
  if (this->is_failed() ||
      this->gfx_ == nullptr ||
      this->buffer_ == nullptr) {
    return;
  }

  /*
   * Run the ESPHome lambda.
   *
   * All calls like:
   *
   *   it.print()
   *   it.line()
   *   it.filled_rectangle()
   *
   * continue to work normally.
   */
  this->do_update_();

  /*
   * Send the framebuffer line by line.
   *
   * ESPHome framebuffer:
   *     RGB332, 1 byte / pixel
   *
   * GC9B72:
   *     RGB565, 2 bytes / pixel
   *
   * We convert each line on the fly.
   */
  for (int y = 0; y < 360; y++) {
    const uint8_t *src =
        &this->buffer_[static_cast<uint32_t>(y) * 360UL];

    for (int x = 0; x < 360; x++) {
      this->line_buffer_[x] =
          this->rgb332_to_rgb565_(src[x]);
    }

    this->gfx_->draw16bitBeRGBBitmap(
        0,
        y,
        this->line_buffer_,
        360,
        1);
  }
}

void GC9B72Display::fill(Color color) {
  if (this->buffer_ == nullptr) {
    return;
  }

  const uint8_t packed = this->color_to_rgb332_(color);

  for (uint32_t i = 0; i < 360UL * 360UL; i++) {
    this->buffer_[i] = packed;
  }
}

void GC9B72Display::draw_absolute_pixel_internal(
    int x,
    int y,
    Color color) {

  if (this->buffer_ == nullptr ||
      x < 0 ||
      x >= 360 ||
      y < 0 ||
      y >= 360) {
    return;
  }

  const uint32_t pos =
      static_cast<uint32_t>(y) * 360UL +
      static_cast<uint32_t>(x);

  this->buffer_[pos] =
      this->color_to_rgb332_(color);
}

/*
 * Convert ESPHome 8-bit RGB channels to RGB332.
 *
 * RRR GGG BB
 */
uint8_t GC9B72Display::color_to_rgb332_(Color color) {
  const uint8_t r = color.r >> 5;
  const uint8_t g = color.g >> 5;
  const uint8_t b = color.b >> 6;

  return static_cast<uint8_t>(
      (r << 5) |
      (g << 2) |
      b);
}

/*
 * Convert RGB332 -> RGB565.
 */
uint16_t GC9B72Display::rgb332_to_rgb565_(uint8_t color) {
  const uint8_t r3 =
      (color >> 5) & 0x07;

  const uint8_t g3 =
      (color >> 2) & 0x07;

  const uint8_t b2 =
      color & 0x03;

  /*
   * Expand:
   *
   * R 3-bit -> R 5-bit
   * G 3-bit -> G 6-bit
   * B 2-bit -> B 5-bit
   */
  const uint8_t r5 =
      static_cast<uint8_t>(
          (r3 << 2) | (r3 >> 1));

  const uint8_t g6 =
      static_cast<uint8_t>(
          (g3 << 3) | g3);

  const uint8_t b5 =
      static_cast<uint8_t>(
          (b2 << 3) | (b2 << 1) | (b2 >> 1));

  return static_cast<uint16_t>(
      (static_cast<uint16_t>(r5) << 11) |
      (static_cast<uint16_t>(g6) << 5) |
      b5);
}

void GC9B72Display::dump_config() {
  ESP_LOGCONFIG(TAG, "GC9B72 360x360");

  ESP_LOGCONFIG(
      TAG,
      "  SPI CLK: GPIO%d",
      this->clk_pin_);

  ESP_LOGCONFIG(
      TAG,
      "  SPI MOSI: GPIO%d",
      this->mosi_pin_);

  ESP_LOGCONFIG(
      TAG,
      "  CS: GPIO%d",
      this->cs_pin_);

  ESP_LOGCONFIG(
      TAG,
      "  DC: GPIO%d",
      this->dc_pin_);

  ESP_LOGCONFIG(
      TAG,
      "  RESET: GPIO%d",
      this->reset_pin_);

  ESP_LOGCONFIG(
      TAG,
      "  Rotation: %u",
      this->rotation_);

  ESP_LOGCONFIG(
      TAG,
      "  SPI speed: %ld Hz",
      static_cast<long>(this->data_rate_));

  ESP_LOGCONFIG(
      TAG,
      "  Framebuffer: RGB332, 129600 bytes");
}

}  // namespace gc9b72
}  // namespace esphome