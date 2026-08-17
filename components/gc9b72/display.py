import esphome.codegen as cg
import esphome.config_validation as cv

from esphome.components import display
from esphome.const import CONF_ID, CONF_ROTATION

gc9b72_ns = cg.esphome_ns.namespace("gc9b72")

GC9B72Display = gc9b72_ns.class_(
    "GC9B72Display",
    display.DisplayBuffer,
)

CONF_CLK_PIN = "clk_pin"
CONF_MOSI_PIN = "mosi_pin"
CONF_CS_PIN = "cs_pin"
CONF_DC_PIN = "dc_pin"
CONF_RESET_PIN = "reset_pin"
CONF_DATA_RATE = "data_rate"

CONFIG_SCHEMA = (
    display.FULL_DISPLAY_SCHEMA.extend(
        {
            cv.GenerateID(): cv.declare_id(GC9B72Display),

            cv.Required(CONF_CLK_PIN): cv.int_,
            cv.Required(CONF_MOSI_PIN): cv.int_,

            cv.Required(CONF_CS_PIN): cv.int_,
            cv.Required(CONF_DC_PIN): cv.int_,
            cv.Required(CONF_RESET_PIN): cv.int_,

            cv.Optional(CONF_DATA_RATE, default=20000000): cv.int_range(
                min=1000000,
                max=80000000,
            ),
        }
    )
    .extend(cv.polling_component_schema("1s"))
    .extend(cv.COMPONENT_SCHEMA)
)


async def to_code(config):
    # Получаем числовое значение поворота через встроенный helper ESPHome (вернет 0, 90, 180 или 270, либо индекс)
    rotation = display.vcall_rotation(config)

    var = cg.new_Pvariable(
        config[CONF_ID],
        config[CONF_CLK_PIN],
        config[CONF_MOSI_PIN],
        config[CONF_CS_PIN],
        config[CONF_DC_PIN],
        config[CONF_RESET_PIN],
        config[CONF_ROTATION], # Передаем исходное значение в C++ конструктор
        config[CONF_DATA_RATE],
    )

    await display.register_display(var, config)

    if display.CONF_LAMBDA in config:
        lambda_ = await cg.process_lambda(
            config[display.CONF_LAMBDA],
            [(display.DisplayRef, "it")],
            return_type=cg.void,
        )
        cg.add(var.set_writer(lambda_))