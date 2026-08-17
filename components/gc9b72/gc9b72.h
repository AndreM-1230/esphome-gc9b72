#pragma once

#include <Arduino_GFX_Library.h>
#include "Arduino_GC9B72.h"

#include "esphome/components/display/display_buffer.h"

namespace esphome {
namespace gc9b72 {

class GC9B72Display : public display::DisplayBuffer {
 public:
  GC9B72Display(int clk_pin, int mosi_pin, int cs_pin, int dc_pin, int reset_pin,
                uint8_t rotation, int32_t data_rate);

  void setup() override;
  void update() override;
  void dump_config() override;

  display::DisplayType get_display_type() override {
    return display::DisplayType::DISPLAY_TYPE_COLOR;
  }

  void fill(Color color) override;

 protected:
  void draw_absolute_pixel_internal(int x, int y, Color color) override;
  int get_width_internal() override { return 360; }
  int get_height_internal() override { return 360; }

  int clk_pin_;
  int mosi_pin_;
  int cs_pin_;
  int dc_pin_;
  int reset_pin_;
  uint8_t rotation_;
  int32_t data_rate_;

  Arduino_DataBus *bus_{nullptr};
  Arduino_GFX *gfx_{nullptr};
};

}  // namespace gc9b72
}  // namespace esphome