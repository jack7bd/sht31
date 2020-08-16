// SPDX-License-Identifier: GPL-2.0
/**
 * SHT31 Humidity and Temperature Sensor driver
 *
 * Copyright 2020 Pawel Skrzypiec <pawel.skrzypiec@agh.edu.pl>
 */

#include <linux/delay.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/gpio/consumer.h>

#define SHT31_MEAS_EXP_PERIOD_S 1

struct sht31_measurement {
    unsigned long exp_time;
    int temp;
    int hum;
};

struct sht31 {
    struct i2c_client *dev;
    struct gpio_desc *rst;
    struct mutex meas_mutex;
    struct sht31_measurement meas;
};

static int sht31_read(struct sht31 *sht31)
{
    int err;
    uint8_t cmd[] = {0x24, 0x00};
    uint8_t buf[6];

    err = i2c_master_send(sht31->dev, cmd, sizeof(cmd));
    if (err < 0) {
        dev_err(&sht31->dev->dev, "failed to send command\n");
        return err;
    }

    msleep(15);

    err = i2c_master_recv(sht31->dev, buf, sizeof(buf));
    if (err < 0) {
        dev_err(&sht31->dev->dev, "failed to read measurement result\n");
        return err;
    }

    sht31->meas.temp = 175 * (buf[0]<<8 | buf[1]) / 65535 - 45;
    sht31->meas.hum = 100 * (buf[3]<<8 | buf[4]) / 65535;
    sht31->meas.exp_time = jiffies + SHT31_MEAS_EXP_PERIOD_S * HZ;
    return 0;
}

static ssize_t sht31_temp_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sht31 *sht31 = (struct sht31 *)dev->driver_data;
    int temp;

    mutex_lock(&sht31->meas_mutex);
    if (time_after(jiffies, sht31->meas.exp_time))
        sht31_read(sht31);
    temp = sht31->meas.temp;
    mutex_unlock(&sht31->meas_mutex);

    return sprintf(buf, "%d\n", temp);
}

static DEVICE_ATTR(temperature, S_IRUSR, sht31_temp_show, NULL);

static ssize_t sht31_hum_show(struct device *dev, struct device_attribute *attr, char *buf)
{
    struct sht31 *sht31 = (struct sht31 *)dev->driver_data;
    int hum;

    mutex_lock(&sht31->meas_mutex);
    if (time_after(jiffies, sht31->meas.exp_time))
        sht31_read(sht31);
    hum = sht31->meas.hum;
    mutex_unlock(&sht31->meas_mutex);

    return sprintf(buf, "%d\n", hum);
}

static DEVICE_ATTR(humidity, S_IRUSR, sht31_hum_show, NULL);

static int sht31_probe(struct i2c_client *dev, const struct i2c_device_id *i2c_device_id)
{
    int err;
    struct sht31 *sht31;

    sht31 = devm_kzalloc(&dev->dev, sizeof(sht31), GFP_KERNEL);
    if (!sht31) {
        dev_err(&dev->dev, "failed to allocate struct sht31\n");
        err = -ENOMEM;
        goto out_ret_err;
    }

    sht31->dev = dev;
    i2c_set_clientdata(dev, sht31);

    sht31->rst = devm_gpiod_get(&dev->dev, "rst", 0);
    if (IS_ERR(sht31->rst)) {
        dev_err(&dev->dev, "failed to allocate rst\n");
        err = PTR_ERR(sht31->rst);
        goto out_ret_err;
    }

    err = device_create_file(&dev->dev, &dev_attr_temperature);
    if (err) {
        dev_err(&dev->dev, "failed to create temperature attr file\n");
        goto out_ret_err;
    }

    err = device_create_file(&dev->dev, &dev_attr_humidity);
    if (err) {
        dev_err(&dev->dev, "failed to create humidity attr file\n");
        goto out_remove_temp_attr_file;
    }

    mutex_init(&sht31->meas_mutex);

    gpiod_set_value(sht31->rst, 1);
    ndelay(350);
    gpiod_set_value(sht31->rst, 0);

    dev_info(&dev->dev, "probed\n");
    return 0;

out_remove_temp_attr_file:
    device_remove_file(&dev->dev, &dev_attr_temperature);
out_ret_err:
    return err;
}

static int sht31_remove(struct i2c_client *dev)
{
    struct sht31 *sht31 = i2c_get_clientdata(dev);

    mutex_destroy(&sht31->meas_mutex);
    device_remove_file(&dev->dev, &dev_attr_humidity);
    device_remove_file(&dev->dev, &dev_attr_temperature);

    dev_info(&dev->dev, "removed\n");
    return 0;
}

static const struct i2c_device_id sht31_id[] = {
    { .name = "sht31" },
    { }
};
MODULE_DEVICE_TABLE(i2c, sht31_id);

static struct i2c_driver sht31_driver = {
    .id_table = sht31_id,
    .probe = sht31_probe,
    .remove = sht31_remove,
    .driver = {
        .name = "sht31",
        .owner = THIS_MODULE,
    },
};
module_i2c_driver(sht31_driver);

MODULE_DESCRIPTION("Simple driver for SHT31 Humidity and Temperature Sensor");
MODULE_AUTHOR("Pawel Skrzypiec <pawel.skrzypiec@agh.edu.pl>");
MODULE_LICENSE("GPL v2");
