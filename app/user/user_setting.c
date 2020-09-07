#include "ets_sys.h"
#include "osapi.h"
#include "os_type.h"
#include "mem.h"
#include "user_interface.h"

#include "user_config.h"
#include "user_setting.h"

#include "user_wifi.h"

LOCAL os_timer_t timer_setting;

void ICACHE_FLASH_ATTR user_setting_timer_func(void *arg) {
	user_setting_set_boot_times(0);
}
void ICACHE_FLASH_ATTR
user_setting_set_boot_times(uint32 val) {
	if (val > 200)
		val = 200;
	spi_flash_erase_sector(SETTING_SAVE_BOOT_TIMES_ADDR);
	spi_flash_write(SETTING_SAVE_BOOT_TIMES_ADDR * 4096, &val, 4);
}

uint32 ICACHE_FLASH_ATTR
user_setting_get_boot_times(void) {
	uint32 val;
	spi_flash_read(SETTING_SAVE_BOOT_TIMES_ADDR * 4096, &val, 4);
	if (val > 200) {
		val=0;
		user_setting_set_boot_times(val);
	}
	return val;
}

/*
 * 掉电读写设置参数
 * 修改设置后保存设置,上电后读取设置参数
 */

void ICACHE_FLASH_ATTR
user_setting_init(void) {


	int16_t i, j;

	boot_times =user_setting_get_boot_times();
	boot_times++;
	user_setting_set_boot_times(boot_times);

	user_setting_get_config();

	os_printf("Boot times:%d\r\n", boot_times);
	os_printf("Config version:%d\r\n", user_config.version);
	os_printf("Device name:\"%s\"\r\n", user_config.name);
	os_printf("MQTT Service ip:\"%s:%d\"\r\n", user_config.mqtt_ip, user_config.mqtt_port);
	os_printf("MQTT Service user:\"%s\"\r\n", user_config.mqtt_user);
	os_printf("MQTT Service password:\"%s\"\r\n", user_config.mqtt_password);

//	    for ( i = 0; i < PLUG_NUM; i++ )
//	    {
//	    	os_printf("plug_%d:\r\n",i);
//	    	os_printf("\tname:%s:\r\n",user_config.plug[i].name);
//	        for ( j = 0; j < PLUG_TIME_TASK_NUM; j++ )
//	        {
//	        	os_printf("\t\ton:%d\t %02d:%02d repeat:0x%X\r\n",user_config.plug[i].task[j].on,
//	                user_config.plug[i].task[j].hour,user_config.plug[i].task[j].minute,
//	                user_config.plug[i].task[j].repeat);
//	        }
//	    }

	os_timer_disarm(&timer_setting);
	os_timer_setfn(&timer_setting, (os_timer_func_t *) user_setting_timer_func, NULL);
	os_timer_arm(&timer_setting, 2500, 0);
}

void ICACHE_FLASH_ATTR
user_setting_set_config(void) {

	uint16_t i, j;
	uint32_t length = sizeof(user_config_t);
	if (length % 4 != 0)
		length += 4 - length % 4;	// 4 字节对齐。
	uint8_t *p = (uint8_t *) os_malloc(length);
	os_memcpy(p, &user_config, length);
	spi_flash_erase_sector(SETTING_SAVE_ADDR);
	spi_flash_write(SETTING_SAVE_ADDR * 4096, (uint32 *) p, length);

	os_free(p);
}

void ICACHE_FLASH_ATTR
user_setting_get_config(void) {
	uint32 val;

	uint16_t i, j;
	uint32_t length = sizeof(user_config_t);
	if (length % 4 != 0)
		length += 4 - length % 4;	// 4 字节对齐。
	uint8_t *p = (uint8_t *) os_malloc(length);

	spi_flash_read(SETTING_SAVE_ADDR * 4096, (uint32 *) p, length);

	os_memcpy(&user_config, p, length);
	os_free(p);

//	os_printf("user_config.name[0]:0x%02x 0x%02x 0x%02x\r\n", user_config.name[0],user_config.name[1],user_config.name[2]);
	if (user_config.name[0] == 0xff && user_config.name[1] == 0xff && user_config.name[2] == 0xff || user_config.version != USER_CONFIG_VERSION) {

		wifi_get_macaddr(STATION_IF, hwaddr);
		os_sprintf(user_config.name, DEVICE_NAME, hwaddr[4], hwaddr[5]);
		os_sprintf(user_config.mqtt_ip, "");
		os_sprintf(user_config.mqtt_user, "");
		os_sprintf(user_config.mqtt_password, "");
		user_config.mqtt_port = 1883;
		user_config.version = USER_CONFIG_VERSION;

			user_config.on = 1;

			for (j = 0; j < TIME_TASK_NUM; j++) {
				user_config.task[j].hour = 0;
				user_config.task[j].minute = 0;
				user_config.task[j].repeat = 127;
				user_config.task[j].on = 0;
				user_config.task[j].action = 1;
				user_config.task[j].w = 0;
				user_config.task[j].r = 0;
				user_config.task[j].g = 0;
				user_config.task[j].b = 0;
			}

		user_setting_set_config();
	}
}

