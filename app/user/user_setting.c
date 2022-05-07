#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"
#include "user_config.h"
#include "zlib.h"
#include "user_setting.h"

LOCAL os_timer_t timer_setting;

void ICACHE_FLASH_ATTR user_setting_timer_func(void *arg)
{
    user_setting_set_boot_times(0);
}
void ICACHE_FLASH_ATTR
user_setting_set_boot_times(uint32_t val)
{
    if(val > 200) val = 200;
    spi_flash_erase_sector(SETTING_SAVE_BOOT_TIMES_ADDR);
    spi_flash_write(SETTING_SAVE_BOOT_TIMES_ADDR * 4096, &val, 4);
}

uint32_t ICACHE_FLASH_ATTR
user_setting_get_boot_times(void)
{
    uint32_t val;
    zlib_setting_get_flash(SETTING_SAVE_BOOT_TIMES_ADDR, &val, 4);
    if(val > 200)
    {
        val = 0;
        user_setting_set_boot_times(val);
    }
    return val;
}

uint32_t ICACHE_FLASH_ATTR
user_setting_init(void)
{
    uint8_t i;
    uint32_t boot_times;
    boot_times = user_setting_get_boot_times();
    boot_times++;
    user_setting_set_boot_times(boot_times);
    os_printf("boot_times:%d\n", boot_times);

    zlib_setting_get_flash(SETTING_SAVE_ADDR, &user_config, sizeof(user_config_t));
    if(user_config.version != USER_CONFIG_VERSION)
    {
        uint8_t *str;
        zlib_wifi_mac_init();
        str = zlib_wifi_get_mac_str();
        os_sprintf(user_config.name, DEVICE_NAME, str + 8);
        os_printf("device name:%s\n", user_config.name);
        os_sprintf(user_config.mqtt_ip, "zipzhang.top");
        os_sprintf(user_config.mqtt_user, "z");
        os_sprintf(user_config.mqtt_password, "2633063");
        user_config.mqtt_port = 1883;
        user_config.version = USER_CONFIG_VERSION;
        user_config.gpio[0] = 15;
        user_config.gpio[1] = 5;
        user_config.gpio[2] = 12;
        user_config.gpio[3] = 14;
        user_config.gpio[4] = 255;
        user_config.auto_off = 0;

        for (i = 0; i < TIME_TASK_NUM; i++)
        {
            user_time_task_config_t *task = &user_config.task[i];
            task->on = 0;
            task->hour = 12;
            task->minute = 0;
            task->repeat = 0x7f;
            task->action = 0;
            task->r = 0;
            task->g = 0;
            task->b = 0;
            task->w = 0;
        }
        os_printf("config version error.Restore default settings\n");
        zlib_setting_save_flash(SETTING_SAVE_ADDR, &user_config, sizeof(user_config_t));
    }

    os_printf("setting:\n");
    os_printf("\tdevice name:%s\n", user_config.name);
    os_printf("\auto_off:%d\n", user_config.auto_off);
    os_printf("\tgpio:[%d,%d,%d,%d]\n", user_config.gpio[0], user_config.gpio[1], user_config.gpio[2],
            user_config.gpio[3]);

    os_printf("user setting init\n");

    os_timer_disarm(&timer_setting);
    os_timer_setfn(&timer_setting, (os_timer_func_t *) user_setting_timer_func, NULL);
    os_timer_arm(&timer_setting, 4000, 0);

    return boot_times;
}

