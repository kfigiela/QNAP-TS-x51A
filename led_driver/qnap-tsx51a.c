/*
 * Support for LEDs available on the QNAP TS-x51A NAS.
 *
 * Copyright (C) 2020 Kamil Figiela <kamil.figiela@gmail.com>
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/leds.h>
#include <asm/io.h>
#include <linux/platform_device.h>
#include <linux/delay.h>


#define IT8528_CHECK_RETRIES 400
#define IT8528_INPUT_BUFFER_FULL 2
#define IT8528_OUTPUT_BUFFER_FULL 1

u_int8_t sio_read(u_int8_t reg) {
    // Read a value from a IT8528 register
    outb(reg, 0x2E);  // address port
    return inb(0x2F); // data port
}

bool ensure_it8528(void) {
    // Check if the Super I/O component is an IT8528
    // Access Super I/O configuration registers
    u_int8_t chipid1 = sio_read(0x20);
    u_int8_t chipid2 = sio_read(0x21);
    if (chipid1 == 0x85 && chipid2 == 0x28) {
        printk(KERN_ERR "QNAP LED: IT8528 found!\n");
        return true;
    } else {
        printk(KERN_ERR "QNAP LED: IT8528 not found!\n");
        return false;
    }
}

int8_t it8528_check_ready(u_int8_t port, u_int8_t bit_value) {
    // Ensure that the port is ready to be used

    int retries = IT8528_CHECK_RETRIES;
    int value = 0;

    // Poll the bit value
    do {
        value = inb(port);
        usleep_range(0x32, 0x50);

        if ((value & bit_value) == 0) {
           return 0;
        }
    }
    while (retries--);

    return -1;
}

int8_t it8528_set_byte(u_int8_t command0, u_int8_t command1, u_int8_t value) {
    // Write  byte

    int8_t ret_value;

    ret_value = it8528_check_ready(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret_value != 0) {
        return ret_value;
    }
    outb(0x88, 0x6C);

    ret_value = it8528_check_ready(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret_value != 0) {
        return ret_value;
    }
    outb(command0 | 0x80, 0x68);

    ret_value = it8528_check_ready(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret_value != 0) {
        return ret_value;
    }
    outb(command1, 0x68);

    ret_value = it8528_check_ready(0x6C, IT8528_INPUT_BUFFER_FULL);
    if (ret_value != 0) {
        return ret_value;
    }
    outb(value, 0x68);

    return 0;
}

bool status_green = false;
bool status_red = false;

static void qnap_led_set_qnap_status(void) {
    if (!status_green && !status_red)
        it8528_set_byte(1, 0x55, 0);
    else if (status_green && status_red)
        it8528_set_byte(1, 0x55, 5); // blink/alternate between the two colors
    else if(status_green && !status_red)
        it8528_set_byte(1, 0x55, 1);
    else if(!status_green && status_red)
        it8528_set_byte(1, 0x55, 2);
}

static int qnap_led_set_qnap_green_status(struct led_classdev *led_cdev, enum led_brightness value) {
    status_green = (value > 0);
    qnap_led_set_qnap_status();
    return 0;
}
static int qnap_led_set_qnap_red_status(struct led_classdev *led_cdev, enum led_brightness value) {
    status_red = (value > 0);
    qnap_led_set_qnap_status();
    return 0;
}

static int qnap_led_set_qnap_red_panic_status(struct led_classdev *led_cdev, enum led_brightness value) {
    if (value > 0) it8528_set_byte(1, 0x55, 4);
    return 0;
}

static int qnap_led_set_qnap_blue_usb(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, 0x54, value > 0 ? 2 : 0);
    return 0;
}
static int qnap_led_set_hdd1_red_error(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5c : 0x5d), 1);
    return 0;
}
static int qnap_led_set_hdd2_red_error(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5c : 0x5d), 2);
    return 0;
}
static int qnap_led_set_hdd3_red_error(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5c : 0x5d), 3);
    return 0;
}
static int qnap_led_set_hdd4_red_error(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5c : 0x5d), 4);
    return 0;
}
static int qnap_led_set_hdd1_green_status(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5a : 0x5b), 1);
    return 0;
}
static int qnap_led_set_hdd2_green_status(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5a : 0x5b), 2);
    return 0;
}
static int qnap_led_set_hdd3_green_status(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5a : 0x5b), 3);
    return 0;
}
static int qnap_led_set_hdd4_green_status(struct led_classdev *led_cdev, enum led_brightness value) {
    it8528_set_byte(1, (value > 0 ? 0x5a : 0x5b), 4);
    return 0;
}

static void qnap_led_initalize(void) {
    it8528_set_byte(1, 0x55, 1); // status
    it8528_set_byte(1, 0x54, 0); // USB
    // disk error
    it8528_set_byte(1, 0x5d, 1);
    it8528_set_byte(1, 0x5d, 2);
    it8528_set_byte(1, 0x5d, 3);
    it8528_set_byte(1, 0x5d, 4);
    // disk status
    it8528_set_byte(1, 0x5b, 1);
    it8528_set_byte(1, 0x5b, 2);
    it8528_set_byte(1, 0x5b, 3);
    it8528_set_byte(1, 0x5b, 4);
}

static struct led_classdev qnap_tsx51a_led[] = {
    {
        .name = "qnap:green:status",
        .brightness_set_blocking = qnap_led_set_qnap_green_status,
        .max_brightness = 1,
    },
    {
        .name = "qnap:red:status",
        .brightness_set_blocking = qnap_led_set_qnap_red_status,
        .max_brightness = 1
    },
    {
        .name = "qnap:red:panic",
        .brightness_set_blocking = qnap_led_set_qnap_red_panic_status,
        .max_brightness = 1,
        .flags = LED_PANIC_INDICATOR | LED_SYSFS_DISABLE
    },
    {
        .name = "qnap:blue:usb",
        .brightness_set_blocking = qnap_led_set_qnap_blue_usb,
        .max_brightness = 1,
    },
    {
        .name = "hdd1:red:error",
        .brightness_set_blocking = qnap_led_set_hdd1_red_error,
        .max_brightness = 1,
    },
    {
        .name = "hdd2:red:error",
        .brightness_set_blocking = qnap_led_set_hdd2_red_error,
        .max_brightness = 1,
    },
    {
        .name = "hdd3:red:error",
        .brightness_set_blocking = qnap_led_set_hdd3_red_error,
        .max_brightness = 1,
    },
    {
        .name = "hdd4:red:error",
        .brightness_set_blocking = qnap_led_set_hdd4_red_error,
        .max_brightness = 1,
    },
    {
        .name = "hdd1:green:status",
        .brightness_set_blocking = qnap_led_set_hdd1_green_status,
        .max_brightness = 1,
    },
    {
        .name = "hdd2:green:status",
        .brightness_set_blocking = qnap_led_set_hdd2_green_status,
        .max_brightness = 1,
    },
    {
        .name = "hdd3:green:status",
        .brightness_set_blocking = qnap_led_set_hdd3_green_status,
        .max_brightness = 1,
    },
    {
        .name = "hdd4:green:status",
        .brightness_set_blocking = qnap_led_set_hdd4_green_status,
        .max_brightness = 1,
    },
};


static struct platform_device qnap_tsx51_leds_dev = {
	.name = "leds-qnap",
	.id = -1,
};

static int __init qnap_tsx51a_init(void)
{
   int ret, i;
   printk(KERN_INFO "QNAP LED: Initializing the driver\n");

    if(ensure_it8528() == false) {
        return -ENXIO;
    }

    qnap_led_initalize();
    platform_device_register(&qnap_tsx51_leds_dev);

    for (i = 0; i < ARRAY_SIZE(qnap_tsx51a_led); i++) {
        ret = devm_led_classdev_register(&qnap_tsx51_leds_dev.dev, &qnap_tsx51a_led[i]);
        if (ret < 0)
            return ret;
    }
    return 0;
}

static void __exit qnap_tsx51a_exit(void)
{
    int i;
    for (i = 0; i < ARRAY_SIZE(qnap_tsx51a_led); i++) {
        devm_led_classdev_unregister(&qnap_tsx51_leds_dev.dev, &qnap_tsx51a_led[i]);
    }
    platform_device_unregister(&qnap_tsx51_leds_dev);
}

module_init(qnap_tsx51a_init);
module_exit(qnap_tsx51a_exit);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("QNAP TS-x51A NAS led driver");
MODULE_AUTHOR("Kamil Figiela <kamil.figiela@gmail.com>");
