import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart, sensor
from esphome.const import (
    CONF_ID, 
    CONF_NAME,
    CONF_SENSORS,
    CONF_UNIT_OF_MEASUREMENT, 
    CONF_ADDRESS,
)
DEPENDENCIES = ['uart']

multical402_ns = cg.esphome_ns.namespace('multical402')
Multical402 = multical402_ns.class_('Multical402', cg.Component, sensor.Sensor, uart.UARTDevice)

CONFIG_SCHEMA = uart.UART_DEVICE_SCHEMA.extend(
    {
        cv.GenerateID(): cv.declare_id(Multical402),
        cv.Required(CONF_SENSORS): cv.ensure_list(
            sensor.SENSOR_SCHEMA.extend(
                {
                    cv.Required(CONF_ADDRESS): cv.hex_int,
                    cv.Required(CONF_NAME): cv.string,
                    cv.Required(CONF_UNIT_OF_MEASUREMENT): cv.string,
                }
            )
        ),
    }
)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    for conf in config[CONF_SENSORS]:
        sens = await sensor.new_sensor(conf)
        address = conf[CONF_ADDRESS]
        cg.add(var.add_sensor(address, sens))
    uart_component = await cg.get_variable(config[uart.CONF_UART_ID])
    cg.add(var.set_uart(uart_component))