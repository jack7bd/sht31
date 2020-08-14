# sht31
*sht31* is the simple Raspberry Pi 3 I2C driver for SHT31 Humidity and Temperature Sensor.

**User space interface**

The driver exports to the Raspberry Pi 3 user space the following files:
* _temperature_ (R): returns the temperature measured by sensor,
* _humidity_ (R): returns the humidity measured by sensor.

```bash
root@raspberrypi:~# ls /sys/bus/i2c/devices/1-0044/
driver  humidity  modalias  name  of_node  power  subsystem  temperature  uevent
```
