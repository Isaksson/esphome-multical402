# A component that reads values from Multical 402 from IR.

A configured uart is required.

Configure a list of sensors.

Example:
```yaml
uart:
  id: multical402_uart
  rx_pin: GPIO15
  tx_pin: GPIO13
  baud_rate: 1200

sensor:
  - platform: multical402
    uart_id: multical402_uart
    sensors:
      - address: 0x003c
        name: "Energy"
        unit_of_measurement: "MWh"
        accuracy_decimals: 5
      - address: 0x0050
        name: "Current Power"
        unit_of_measurement: "kW"
        accuracy_decimals: 5
```
