#include "ets_sys.h"
#include "uart.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"
#include "zlib.h"
#include "zlib_rtc.h"
#include "user_setting.h"
#include "user_mqtt.h"
#include "user_json.h"

void _user_rtc_fun(const struct_time_t time)
{
    static uint32_t size_last;
    uint32_t size;
    size = system_get_free_heap_size();
    if(size_last != size)
    {
        size_last = size;
        os_printf("system_get_free_heap_size:%d\n", size);
    }

    static uint8_t minute_last = 80;
    if(minute_last != time.minute)
    {
        minute_last = time.minute;
        LOGI("%04d/%02d/%02d %d %02d:%02d:%02d\n", time.year, time.month, time.day, time.week, time.hour, time.minute,
                time.second);

        //定时任务处理
        uint8_t i;
        bool update_user_config_flag = false;
        for (i = 0; i < TIME_TASK_NUM; i++)
        {
            if(!user_config.task[i].on) continue;
            uint8_t repeat = user_config.task[i].repeat;
            if(    //符合条件则执行定时任务:时分符合设定值, 重复符合设定值
            time.minute == user_config.task[i].minute && time.hour == user_config.task[i].hour
                    && ((repeat == 0x00) || repeat & (1 << (time.week - 1))))
            {

                cJSON *pJsonRoot = cJSON_CreateObject();
                cJSON_AddStringToObject(pJsonRoot, "mac", zlib_wifi_get_mac_str() );

                char json_temp_str[17] = { 0 };
                os_sprintf(json_temp_str, "[%d,%d,%d,%d]", user_config.task[i].r,user_config.task[i].g,user_config.task[i].b,user_config.task[i].w);
                cJSON_AddItemToObject(pJsonRoot, "rgb", cJSON_Parse(json_temp_str));

                cJSON_AddNumberToObject(pJsonRoot, "gradient", user_config.task[i].gradient);
                user_json_deal_cb(NULL, WIFI_COMM_TYPE_MQTT, pJsonRoot,user_mqtt_get_set_topic());
                cJSON_Delete(pJsonRoot);


                if(repeat == 0x00 || (repeat & 0x80))
                {
                    user_config.task[i].on = 0;
                    zlib_setting_save_flash(SETTING_SAVE_ADDR, &user_config, sizeof(user_config_t));
                }



                break;
            }
        }
    }

}
/**
 * 函  数  名: user_rtc_init
 * 函数说明: rtc初始化
 * 参        数: 无
 * 返        回: 无
 */
void ICACHE_FLASH_ATTR user_rtc_init(void)
{
    zlib_rtc_set_timezone(8);
    zlib_rtc_init();
    zlib_rtc_set_recall_callback(_user_rtc_fun);
    os_printf("user rtc init\n");
}

