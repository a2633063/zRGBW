#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"
#include "zlib.h"
#include "user_mqtt.h"
#include "zlib_rtc.h"
#include "user_json.h"
#include "user_setting.h"
#include "user_led.h"

user_config_t user_config;

uint8_t r = 0;    //r       //记录关闭前的颜色,不能全为0
uint8_t g = 0;    //g
uint8_t b = 0;    //b
uint8_t w = 255;    //w

uint8_t r_now;  //记录当前显示目标颜色,可以为0
uint8_t g_now;
uint8_t b_now;
uint8_t w_now;
int8_t on;    //开关

void ICACHE_FLASH_ATTR _ota_result_cb(state_ota_result_state_t b)
{
    os_printf("_ota_result_cb: %d\r\n", b);
}

void user_init(void)
{
    uint8_t i;
    uint32_t boot_times;
    uart_init(115200, 115200);
    os_printf(" \n \nStart user%d.bin\n", system_upgrade_userbin_check() + 1);
    os_printf("SDK version:%s\n", system_get_sdk_version());
    os_printf("FW version:%s\n", VERSION);

    boot_times = user_setting_init();
    user_led_init();

    zlib_wifi_init(boot_times > 4);

    zlib_web_config_init();
    user_mqtt_init();
    user_json_init();
    user_rtc_init();
//	user_io_init();

    zlib_udp_init(10182);
    zlib_tcp_init(10182);
    zlib_ota_set_result_callback(_ota_result_cb);

    if(wifi_get_opmode() == SOFTAP_MODE)
    {
        user_led_mode_wificonfig();

    }
    else
    {
        user_led_set(0, 0, 0, 255, 0);
    }
}
