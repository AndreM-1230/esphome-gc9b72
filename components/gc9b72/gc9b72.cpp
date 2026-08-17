#include "gc9b72.h"

#include "esphome/core/log.h"
#include "esphome/components/display/display_color_utils.h"

namespace esphome {
namespace gc9b72 {

static const char *const TAG = "gc9b72";

GC9B72Display::GC9B72Display(int clk_pin, int mosi_pin, int cs_pin, int dc_pin,
                             int reset_pin, uint8_t rotation, int32_t data_rate)
    : clk_pin_(clk_pin),
      mosi_pin_(mosi_pin),
      cs_pin_(cs_pin),
      dc_pin_(dc_pin),
      reset_pin_(reset_pin),
      rotation_(rotation),
      data_rate_(data_rate) {}

void GC9B72Display::setup() {
  // 360x360 RGB565 framebuffer: 259200 bytes.
  this->init_internal_(360UL * 360UL * 2UL);

  if (this->buffer_ == nullptr) {
    this->mark_failed();
    return;
  }

  // Same working bus configuration as the verified Arduino sketch.
  this->bus_ = new Arduino_ESP32SPI(
      this->dc_pin_,
      this->cs_pin_,
      this->clk_pin_,
      this->mosi_pin_,
      GFX_NOT_DEFINED);

  // Rotation is handled by the GC9B72/Arduino_GFX driver.
  this->gfx_ = new Arduino_GC9B72(
      this->bus_,
      this->reset_pin_,
      this->rotation_,
      false,
      360,
      360);

  if (this->gfx_ == nullptr || !this->gfx_->begin(this->data_rate_)) {
    ESP_LOGE(TAG, "GC9B72 initialization failed");
    this->mark_failed();
    return;
  }

  this->fill(Color(0, 0, 0, 0));
  ESP_LOGI(TAG, "GC9B72 initialized: 360x360, rotation=%u, SPI=%ld Hz",
           this->rotation_, static_cast<long>(this->data_rate_));
}

void GC9B72Display::update() {
  if (this->is_failed() || this->gfx_ == nullptr || this->buffer_ == nullptr) {
    return;
  }

  // Run the normal ESPHome display lambda into the framebuffer.
  this->do_update_();

  // ESPHome stores RGB565 as big-endian bytes. Arduino_GFX has a dedicated
  // big-endian bitmap path, so no per-pixel byte swapping is needed here.
  this->gfx_->draw16bitBeRGBBitmap(
      0, 0,
      reinterpret_cast<uint16_t *>(this->buffer_),
      360, 360);
}

void GC9B72Display::fill(Color color) {
  if (this->buffer_ == nullptr) {
    return;
  }

  const uint16_t c = display::ColorUtil::color_to_565(color);
  const uint8_t hi = static_cast<uint8_t>(c >> 8);
  const uint8_t lo = static_cast<uint8_t>(c & 0xFF);

  for (uint32_t i = 0; i < 360UL * 360UL; i++) {
    this->buffer_[i * 2] = hi;
    this->buffer_[i * 2 + 1] = lo;
  }
}

void GC9B72Display::draw_absolute_pixel_internal(int x, int y, Color color) {
  if (this->buffer_ == nullptr ||
      x < 0 || x >= 360 ||
      y < 0 || y >= 360) {
    return;
  }

  const uint16_t c = display::ColorUtil::color_to_565(color);
  const uint32_t pos = (static_cast<uint32_t>(y) * 360UL +
                        static_cast<uint32_t>(x)) * 2UL;

  this->buffer_[pos] = static_cast<uint8_t>(c >> 8);
  this->buffer_[pos + 1] = static_cast<uint8_t>(c & 0xFF);
}

void GC9B72Display::dump_config() {
  ESP_LOGCONFIG(TAG, "GC9B72 360x360");
  ESP_LOGCONFIG(TAG, "  SPI CLK: GPIO%d", this->clk_pin_);
  ESP_LOGCONFIG(TAG, "  SPI MOSI: GPIO%d", this->mosi_pin_);
  ESP_LOGCONFIG(TAG, "  CS: GPIO%d", this->cs_pin_);
  ESP_LOGCONFIG(TAG, "  DC: GPIO%d", this->dc_pin_);
  ESP_LOGCONFIG(TAG, "  RESET: GPIO%d", this->reset_pin_);
  ESP_LOGCONFIG(TAG, "  Rotation: %u", this->rotation_);
  ESP_LOGCONFIG(TAG, "  SPI speed: %ld Hz", static_cast<long>(this->data_rate_));
}

}  // namespace gc9b72
}  // namespace esphome
