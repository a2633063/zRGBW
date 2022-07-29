#include "ets_sys.h"
#include "osapi.h"
#include "user_interface.h"
#include "user_config.h"
#include "zlib.h"

#include "user_setting.h"
#include "user_led.h"
#include "user_mqtt.h"

bool json_task_analysis(unsigned char x, cJSON * pJsonRoot, cJSON * pJsonSend);

static os_timer_t _timer_auto_off;
static void _json_timer_auto_off_fun(uint8 * gradient)
{
    os_printf("auto off, gradient:%d\n", *gradient);

    if(on != 0 )
    {
        cJSON *pJsonRoot = cJSON_CreateObject();
        cJSON_AddStringToObject(pJsonRoot, "mac", zlib_wifi_get_mac_str() );
        cJSON_AddNumberToObject(pJsonRoot, "on", 0);
        cJSON_AddNumberToObject(pJsonRoot, "gradient", *gradient);
        user_json_deal_cb(NULL, WIFI_COMM_TYPE_MQTT, pJsonRoot,user_mqtt_get_set_topic());
        cJSON_Delete(pJsonRoot);
    }
    //user_led_set(0, 0, 0, 0, *gradient);
}
/**
 * 函  数  名: _json_timer_fun
 * 函数说明: json处理定時器回调函数,当需要保存flash时,延时2秒再保存数据
 * 参        数: 无
 * 返        回: 无
 */
static os_timer_t _timer_json;
static void _json_timer_fun(void *arg)
{
    zlib_setting_save_flash(SETTING_SAVE_ADDR, &user_config, sizeof(user_config_t));
}

/**
 * 函  数  名: _json_deal_cb
 * 函数说明: json数据处理初始化回调函数,在此函数中处理json
 * 参        数: 无
 * 返        回: 无
 */
void ICACHE_FLASH_ATTR user_json_deal_cb(void *arg, Wifi_Comm_type_t type, cJSON * pJsonRoot, void *p)
{
    bool update_user_config_flag = false;   //标志位,记录最后是否需要更新储存的数据
    uint8_t retained = 0;
    uint8_t i;
    static uint8_t gradient = 0;
    gradient = 0;
    //解析device report
    cJSON *p_cmd = cJSON_GetObjectItem(pJsonRoot, "cmd");
    if(p_cmd && cJSON_IsString(p_cmd) && os_strcmp(p_cmd->valuestring, "device report") == 0)
    {

        os_printf("device report\r\n");
        cJSON *pRoot = cJSON_CreateObject();
        cJSON_AddStringToObject(pRoot, "name", user_config.name);
        cJSON_AddStringToObject(pRoot, "ip", zlib_wifi_get_ip_str());
        cJSON_AddStringToObject(pRoot, "mac", zlib_wifi_get_mac_str());
        cJSON_AddNumberToObject(pRoot, "type", TYPE);
        cJSON_AddStringToObject(pRoot, "type_name", TYPE_NAME);

        char json_temp_str[64] = { 0 };
        os_sprintf(json_temp_str, "[\"%s\",\"%s\",\"%s\",\"%s\"]", user_mqtt_get_state_topic(),
                user_mqtt_get_set_topic(), user_mqtt_get_sensor_topic(), user_mqtt_get_will_topic());
        cJSON_AddItemToObject(pRoot, "topic", cJSON_Parse(json_temp_str));

        char *s = cJSON_Print(pRoot);
        os_printf("pRoot: [%s]\r\n", s);

        zlib_fun_wifi_send(arg, type, user_mqtt_get_state_topic(), s, 1, 0);

        cJSON_free((void *) s);
        cJSON_Delete(pRoot);
        return;
    }

    //解析
    cJSON *p_name = cJSON_GetObjectItem(pJsonRoot, "name");
    cJSON *p_mac = cJSON_GetObjectItem(pJsonRoot, "mac");

    if((p_mac && cJSON_IsString(p_mac) && os_strcmp(p_mac->valuestring, zlib_wifi_get_mac_str()) != 0)
            && (p_mac && cJSON_IsString(p_mac) && os_strcmp(p_mac->valuestring, strlwr(TYPE_NAME)) != 0)
            && (type != WIFI_COMM_TYPE_HTTP && type != WIFI_COMM_TYPE_TCP)) return;

    cJSON *json_send = cJSON_CreateObject();
    //mac字段
    cJSON_AddStringToObject(json_send, "mac", zlib_wifi_get_mac_str());

    //version版本
    cJSON *p_version = cJSON_GetObjectItem(pJsonRoot, "version");
    if(p_version)
    {
        cJSON_AddStringToObject(json_send, "version", VERSION);
    }
    //重启命令
    if(p_cmd && cJSON_IsString(p_cmd) && strcmp(p_cmd->valuestring, "restart") == 0)
    {
        os_printf("cmd:restart");
        zlib_reboot_delay(2000);
        cJSON_AddStringToObject(json_send, "cmd", "restart");
    }

    //解析gpio设置
    cJSON *p_gpio = cJSON_GetObjectItem(pJsonRoot, "gpio");
    if(p_gpio)
    {
        if(cJSON_IsArray(p_gpio) && cJSON_GetArraySize(p_gpio) > 3)
        {
            cJSON *p_gpio_r = cJSON_GetArrayItem(p_gpio, 0);
            cJSON *p_gpio_g = cJSON_GetArrayItem(p_gpio, 1);
            cJSON *p_gpio_b = cJSON_GetArrayItem(p_gpio, 2);
            cJSON *p_gpio_w = cJSON_GetArrayItem(p_gpio, 3);
            if(cJSON_IsNumber(p_gpio_r) && cJSON_IsNumber(p_gpio_g) && cJSON_IsNumber(p_gpio_b)
                    && cJSON_IsNumber(p_gpio_w))
            {
                uint8_t gpio_r = p_gpio_r->valueint;
                uint8_t gpio_g = p_gpio_g->valueint;
                uint8_t gpio_b = p_gpio_b->valueint;
                uint8_t gpio_w = p_gpio_w->valueint;

                do
                {
                    if(gpio_r == gpio_g || gpio_r == gpio_b || gpio_r == gpio_w || gpio_g == gpio_b || gpio_g == gpio_w
                            || gpio_b == gpio_w) break;

                    //记录当前rgbw配置,若配置失败则还原配置
                    uint8_t gpio_r_lase = user_config.gpio[0];
                    uint8_t gpio_g_lase = user_config.gpio[1];
                    uint8_t gpio_b_lase = user_config.gpio[2];
                    uint8_t gpio_w_lase = user_config.gpio[3];

                    user_config.gpio[0] = gpio_r;
                    user_config.gpio[1] = gpio_g;
                    user_config.gpio[2] = gpio_b;
                    user_config.gpio[3] = gpio_w;
                    if(user_led_gpio_config())
                    {   //配置成功
                        update_user_config_flag = true;
                        user_led_set(0, 51, 0, 0, 0);   //配置rgbw gpio成功后,亮绿灯
                    }
                    else
                    {   //配置失败 还原原配置
                        os_printf("user_led_gpio_config fail [%d,%d,%d,%d]", user_config.gpio[0], user_config.gpio[1],
                                user_config.gpio[2], user_config.gpio[3]);
                        user_config.gpio[0] = gpio_r_lase;
                        user_config.gpio[1] = gpio_g_lase;
                        user_config.gpio[2] = gpio_b_lase;
                        user_config.gpio[3] = gpio_w_lase;
                    }
                } while (0);
            }

        }
        char json_temp_str[17] = { 0 };
        os_sprintf(json_temp_str, "[%d,%d,%d,%d]", user_config.gpio[0], user_config.gpio[1], user_config.gpio[2],
                user_config.gpio[3]);
        cJSON_AddItemToObject(json_send, "gpio", cJSON_Parse(json_temp_str));
    }

    //设置自动关闭功能
    cJSON *p_auto_off = cJSON_GetObjectItem(pJsonRoot, "auto_off");
    if(p_auto_off)
    {
        if(cJSON_IsNumber(p_auto_off)) user_config.auto_off = p_auto_off->valueint;
        cJSON_AddNumberToObject(json_send, "auto_off", user_config.auto_off);
    }
    //解析渐变
    cJSON *p_gradient = cJSON_GetObjectItem(pJsonRoot, "gradient");
    if(p_gradient && cJSON_IsNumber(p_gradient) && p_gradient->valueint == 1)
    {
        gradient = 1;
    }

    //解析HSL颜色
    cJSON *p_hsl = cJSON_GetObjectItem(pJsonRoot, "hsl");
    if(p_hsl && cJSON_IsArray(p_hsl) && cJSON_GetArraySize(p_hsl) == 4)
    {
        cJSON *p_hsl_h = cJSON_GetArrayItem(p_hsl, 0);
        cJSON *p_hsl_s = cJSON_GetArrayItem(p_hsl, 1);
        cJSON *p_hsl_l = cJSON_GetArrayItem(p_hsl, 2);
        cJSON *p_hsl_w = cJSON_GetArrayItem(p_hsl, 3);
        if(cJSON_IsNumber(p_hsl_h) && cJSON_IsNumber(p_hsl_s) && cJSON_IsNumber(p_hsl_l) && cJSON_IsNumber(p_hsl_w))
        {
            os_printf("hsl:%d,%d,%d,%d\n", p_hsl_h->valueint, p_hsl_s->valueint, p_hsl_l->valueint, p_hsl_w->valueint);

            HSL2RGB(p_hsl_h->valueint, p_hsl_s->valueint, p_hsl_l->valueint, &r_now, &g_now, &b_now);
            w_now = p_hsl_w->valueint;
            user_led_set(r_now, g_now, b_now, w_now, gradient);
        }
    }
    //解析RGB颜色
    cJSON *p_rgb = cJSON_GetObjectItem(pJsonRoot, "rgb");
    if(p_rgb && cJSON_IsArray(p_rgb) && cJSON_GetArraySize(p_rgb) == 4)
    {
        cJSON *p_rgb_r = cJSON_GetArrayItem(p_rgb, 0);
        cJSON *p_rgb_g = cJSON_GetArrayItem(p_rgb, 1);
        cJSON *p_rgb_b = cJSON_GetArrayItem(p_rgb, 2);
        cJSON *p_rgb_w = cJSON_GetArrayItem(p_rgb, 3);
        if(cJSON_IsNumber(p_rgb_r) && cJSON_IsNumber(p_rgb_g) && cJSON_IsNumber(p_rgb_b) && cJSON_IsNumber(p_rgb_w))
        {
            os_printf("rgb:%d,%d,%d,%d\n", p_rgb_r->valueint, p_rgb_g->valueint, p_rgb_b->valueint, p_rgb_w->valueint);

            user_led_set(p_rgb_r->valueint, p_rgb_g->valueint, p_rgb_b->valueint, p_rgb_w->valueint, gradient);
        }
    }

    //设置开关
    cJSON *p_on = cJSON_GetObjectItem(pJsonRoot, "on");
    if(p_on && cJSON_IsNumber(p_on))
    {
        retained = 1;
        if(p_on->valueint == 0)
            user_led_set(0, 0, 0, 0, gradient);
        else
        {
            user_led_set(r, g, b, w, gradient);
        }
    }

    if(p_hsl || p_rgb || p_on)
    {
        char json_temp_str[24] = { 0 };

        os_sprintf(json_temp_str, "[%d,%d,%d,%d]", r_now, g_now, b_now, w_now);
        cJSON_AddItemToObject(json_send, "rgb", cJSON_Parse(json_temp_str));
        uint16_t h_val;
        uint8_t s_val, l_val;
        RGB2HSL(r_now, g_now, b_now, &h_val, &s_val, &l_val);
        os_sprintf(json_temp_str, "[%d,%d,%d,%d]", h_val, s_val, l_val, w_now);
        cJSON_AddItemToObject(json_send, "hsl", cJSON_Parse(json_temp_str));

        retained = 1;
        cJSON_AddNumberToObject(json_send, "on", on);
        if(p_gradient)
        cJSON_AddNumberToObject(json_send, "gradient", gradient);

        //解析是否有临时自动关闭
        os_timer_disarm(&_timer_auto_off);
        if(on != 0)
        {   //只有在开灯时才处理此命令
            cJSON *p_auto_off_once = cJSON_GetObjectItem(pJsonRoot, "auto_off_once");
            if(p_auto_off_once)
            {
                if(cJSON_IsNumber(p_auto_off_once) && p_auto_off_once->valueint > 0)
                {
                    os_timer_setfn(&_timer_auto_off, (os_timer_func_t *) _json_timer_auto_off_fun, &gradient);
                    os_timer_arm(&_timer_auto_off, (uint32_t )p_auto_off_once->valueint * 1000, false);
                    cJSON_AddNumberToObject(json_send, "auto_off_time", p_auto_off_once->valueint);
                }
            }
            else if(user_config.auto_off > 0)
            {
                os_timer_setfn(&_timer_auto_off, (os_timer_func_t *) _json_timer_auto_off_fun, &gradient);
                os_timer_arm(&_timer_auto_off, (uint32_t )user_config.auto_off * 1000, false);
                cJSON_AddNumberToObject(json_send, "auto_off_time", user_config.auto_off);
            }
        }
    }
    //返回wifi ssid及rssi
    cJSON *p_ssid = cJSON_GetObjectItem(pJsonRoot, "ssid");
    if(p_ssid)
    {
        struct station_config ssidGet;
        if(wifi_station_get_config(&ssidGet))
        {
            cJSON_AddStringToObject(json_send, "ssid", ssidGet.ssid);
            cJSON_AddNumberToObject(json_send, "rssi", ssidGet.threshold.rssi);
        }
        else
        {
            cJSON_AddStringToObject(json_send, "ssid", "get wifi_ssid fail");
        }
    }

    //测试
    cJSON *p_test = cJSON_GetObjectItem(pJsonRoot, "test");
    if(p_test && cJSON_IsNumber(p_test))
    {
        cJSON_AddNumberToObject(json_send, "test", p_test->valueint);
    }

    cJSON *p_setting = cJSON_GetObjectItem(pJsonRoot, "setting");
    if(p_setting)
    {
        //解析ota
        uint8_t userBin = system_upgrade_userbin_check();
        cJSON *p_ota1 = cJSON_GetObjectItem(p_setting, "ota1");
        cJSON *p_ota2 = cJSON_GetObjectItem(p_setting, "ota2");
        if(p_ota1 && userBin == UPGRADE_FW_BIN2 && cJSON_IsString(p_ota1))
        {
            zlib_ota_start(p_ota1->valuestring);
        }
        else if(p_ota2 && userBin == UPGRADE_FW_BIN1 && cJSON_IsString(p_ota2))
        {
            zlib_ota_start(p_ota2->valuestring);
        }

        //设置设备名称
        cJSON *p_setting_name = cJSON_GetObjectItem(p_setting, "name");
        if(p_setting_name && cJSON_IsString(p_setting_name))
        {
            update_user_config_flag = true;
            os_sprintf(user_config.name, p_setting_name->valuestring);
        }

        //设置wifi ssid
        cJSON *p_setting_wifi_ssid = cJSON_GetObjectItem(p_setting, "wifi_ssid");
        cJSON *p_setting_wifi_password = cJSON_GetObjectItem(p_setting, "wifi_password");
        if(p_setting_wifi_ssid && cJSON_IsString(p_setting_wifi_ssid) && p_setting_wifi_password
                && cJSON_IsString(p_setting_wifi_password))
        {
            zlib_wifi_set_ssid_delay(p_setting_wifi_ssid->valuestring, p_setting_wifi_password->valuestring, 1000);
        }

        //设置mqtt ip
        cJSON *p_mqtt_ip = cJSON_GetObjectItem(p_setting, "mqtt_uri");
        if(p_mqtt_ip && cJSON_IsString(p_mqtt_ip))
        {
            update_user_config_flag = true;
            os_sprintf(user_config.mqtt_ip, p_mqtt_ip->valuestring);
        }

        //设置mqtt port
        cJSON *p_mqtt_port = cJSON_GetObjectItem(p_setting, "mqtt_port");
        if(p_mqtt_port && cJSON_IsNumber(p_mqtt_port))
        {
            update_user_config_flag = true;
            user_config.mqtt_port = p_mqtt_port->valueint;
        }

        //设置mqtt user
        cJSON *p_mqtt_user = cJSON_GetObjectItem(p_setting, "mqtt_user");
        if(p_mqtt_user && cJSON_IsString(p_mqtt_user))
        {
            update_user_config_flag = true;
            os_sprintf(user_config.mqtt_user, p_mqtt_user->valuestring);
        }

        //设置mqtt password
        cJSON *p_mqtt_password = cJSON_GetObjectItem(p_setting, "mqtt_password");
        if(p_mqtt_password && cJSON_IsString(p_mqtt_password))
        {
            update_user_config_flag = true;
            os_sprintf(user_config.mqtt_password, p_mqtt_password->valuestring);
        }

        //配置setting返回数据
        cJSON *json_setting_send = cJSON_CreateObject();
        cJSON *p_userbin = cJSON_GetObjectItem(p_setting, "userbin");
        if(p_userbin || p_ota1 || p_ota2)
        {
            cJSON_AddNumberToObject(json_setting_send, "userbin", userBin);
        }
        //返回设备ota
        if(p_ota1) cJSON_AddStringToObject(json_setting_send, "ota1", p_ota1->valuestring);
        if(p_ota2) cJSON_AddStringToObject(json_setting_send, "ota2", p_ota2->valuestring);

        //设置设备名称
        if(p_setting_name)
        cJSON_AddStringToObject(json_setting_send, "name", user_config.name);

        //设置设备wifi
        if(p_setting_wifi_ssid || p_setting_wifi_password)
        {
            cJSON_AddStringToObject(json_setting_send, "wifi_ssid", p_setting_wifi_ssid->valuestring);
            cJSON_AddStringToObject(json_setting_send, "wifi_password", p_setting_wifi_password->valuestring);
        }

        //设置mqtt ip
        if(p_mqtt_ip)
        cJSON_AddStringToObject(json_setting_send, "mqtt_uri", user_config.mqtt_ip);

        //设置mqtt port
        if(p_mqtt_port)
        cJSON_AddNumberToObject(json_setting_send, "mqtt_port", user_config.mqtt_port);

        //设置mqtt user
        if(p_mqtt_user)
        cJSON_AddStringToObject(json_setting_send, "mqtt_user", user_config.mqtt_user);

        //设置mqtt password
        if(p_mqtt_password)
        cJSON_AddStringToObject(json_setting_send, "mqtt_password", user_config.mqtt_password);

        cJSON_AddItemToObject(json_send, "setting", json_setting_send);

        if(p_mqtt_ip && cJSON_IsString(p_mqtt_ip) && p_mqtt_port && cJSON_IsNumber(p_mqtt_port) && p_mqtt_user
                && cJSON_IsString(p_mqtt_user) && p_mqtt_password && cJSON_IsString(p_mqtt_password)
                && !zlib_mqtt_is_connected())
        {
            zlib_reboot_delay(1000);
        }
    }

    cJSON_AddStringToObject(json_send, "name", user_config.name);
    //解析定时任务-----------------------------------------------------------------
    for (i = 0; i < TIME_TASK_NUM; i++)
    {
        if(json_task_analysis(i, pJsonRoot, json_send)) update_user_config_flag = true;
    }

    char *json_str = cJSON_Print(json_send);
    os_printf("json_send: %s\r\n", json_str);
    zlib_fun_wifi_send(arg, type, user_mqtt_get_state_topic(), json_str, 1, retained);
    cJSON_free((void *) json_str);
    cJSON_Delete(json_send);
    if(update_user_config_flag)
    {
        os_timer_disarm(&_timer_json);
        os_timer_setfn(&_timer_json, (os_timer_func_t *) _json_timer_fun, NULL);
        os_timer_arm(&_timer_json, 800, false); //1500毫秒后保存

        //zlib_setting_save_config(&user_config, sizeof(user_config_t));
        update_user_config_flag = false;
    }
}

/**
 * 函  数  名: json_task_analysis
 * 函数说明: 解析处理定时任务json
 * 参        数: x:任务编号
 * 返        回:
 */
bool ICACHE_FLASH_ATTR
json_task_analysis(unsigned char x, cJSON * pJsonRoot, cJSON * pJsonSend)
{
    if(!pJsonRoot) return false;

    uint8_t gradient = 0;
    bool return_flag = false;

    char plug_task_str[] = "task_X";
    plug_task_str[5] = x + '0';

    cJSON *p_plug_task = cJSON_GetObjectItem(pJsonRoot, plug_task_str);
    if(!p_plug_task) return false;

    cJSON *json_plug_task_send = cJSON_CreateObject();

    cJSON *p_plug_task_hour = cJSON_GetObjectItem(p_plug_task, "hour");
    cJSON *p_plug_task_minute = cJSON_GetObjectItem(p_plug_task, "minute");
    cJSON *p_plug_task_repeat = cJSON_GetObjectItem(p_plug_task, "repeat");
    cJSON *p_plug_task_on = cJSON_GetObjectItem(p_plug_task, "on");

    cJSON *p_plug_task_hsl = cJSON_GetObjectItem(p_plug_task, "hsl");
    cJSON *p_plug_task_rgb = cJSON_GetObjectItem(p_plug_task, "rgb");

    //解析渐变
    cJSON *p_plug_task_gradient = cJSON_GetObjectItem(p_plug_task, "gradient");
    if(p_plug_task_gradient && cJSON_IsNumber(p_plug_task_gradient) && p_plug_task_gradient->valueint == 1)
    {
        gradient = 1;
    }
    while (p_plug_task_hour && p_plug_task_minute && p_plug_task_repeat && p_plug_task_on
            && (p_plug_task_hsl || p_plug_task_rgb))
    {
        if(!cJSON_IsNumber(p_plug_task_hour) || !cJSON_IsNumber(p_plug_task_minute)
                || !cJSON_IsNumber(p_plug_task_repeat)) break;
        if(!cJSON_IsNumber(p_plug_task_on)) break;

        uint8_t r_task, g_task, b_task, w_task;
        if(!p_plug_task_rgb && p_plug_task_hsl && cJSON_IsArray(p_plug_task_hsl)
                && cJSON_GetArraySize(p_plug_task_hsl) == 4)
        {   //当前配置为hsl
            cJSON *p_plug_task_hsl_h = cJSON_GetArrayItem(p_plug_task_hsl, 0);
            cJSON *p_plug_task_hsl_s = cJSON_GetArrayItem(p_plug_task_hsl, 1);
            cJSON *p_plug_task_hsl_l = cJSON_GetArrayItem(p_plug_task_hsl, 2);
            cJSON *p_plug_task_hsl_w = cJSON_GetArrayItem(p_plug_task_hsl, 3);
            if(!cJSON_IsNumber(p_plug_task_hsl_h) || !cJSON_IsNumber(p_plug_task_hsl_s)
                    || !cJSON_IsNumber(p_plug_task_hsl_l) || !cJSON_IsNumber(p_plug_task_hsl_w)) break;

            HSL2RGB(p_plug_task_hsl_h->valueint, p_plug_task_hsl_s->valueint, p_plug_task_hsl_l->valueint, &r_task,
                    &g_task, &b_task);
            w_task = p_plug_task_hsl_w->valueint;
        }
        else if(!p_plug_task_hsl && p_plug_task_rgb && cJSON_IsArray(p_plug_task_rgb)
                && cJSON_GetArraySize(p_plug_task_rgb) == 4)
        {   //当前配置为rgb
            cJSON *p_plug_task_rgb_r = cJSON_GetArrayItem(p_plug_task_rgb, 0);
            cJSON *p_plug_task_rgb_g = cJSON_GetArrayItem(p_plug_task_rgb, 1);
            cJSON *p_plug_task_rgb_b = cJSON_GetArrayItem(p_plug_task_rgb, 2);
            cJSON *p_plug_task_rgb_w = cJSON_GetArrayItem(p_plug_task_rgb, 3);
            if(!cJSON_IsNumber(p_plug_task_rgb_r) || !cJSON_IsNumber(p_plug_task_rgb_g)
                    || !cJSON_IsNumber(p_plug_task_rgb_b) || !cJSON_IsNumber(p_plug_task_rgb_w)) break;

            r_task = p_plug_task_rgb_r->valueint;
            g_task = p_plug_task_rgb_g->valueint;
            b_task = p_plug_task_rgb_b->valueint;
            w_task = p_plug_task_rgb_w->valueint;
        }
        else
            break;

        return_flag = true;
        user_config.task[x].hour = p_plug_task_hour->valueint;
        user_config.task[x].minute = p_plug_task_minute->valueint;
        user_config.task[x].repeat = p_plug_task_repeat->valueint;
        user_config.task[x].on = p_plug_task_on->valueint;
        user_config.task[x].r = r_task;
        user_config.task[x].g = g_task;
        user_config.task[x].b = b_task;
        user_config.task[x].w = w_task;
        user_config.task[x].gradient = gradient;

        break;
    }
    cJSON_AddNumberToObject(json_plug_task_send, "on", user_config.task[x].on);
    cJSON_AddNumberToObject(json_plug_task_send, "hour", user_config.task[x].hour);
    cJSON_AddNumberToObject(json_plug_task_send, "minute", user_config.task[x].minute);
    cJSON_AddNumberToObject(json_plug_task_send, "repeat", user_config.task[x].repeat);

    char json_temp_str[24] = { 0 };
    os_sprintf(json_temp_str, "[%d,%d,%d,%d]", user_config.task[x].r, user_config.task[x].g, user_config.task[x].b,
            user_config.task[x].w);
    cJSON_AddItemToObject(json_plug_task_send, "rgb", cJSON_Parse(json_temp_str));
    uint16_t h_val;
    uint8_t s_val, l_val;
    RGB2HSL(user_config.task[x].r, user_config.task[x].g, user_config.task[x].b, &h_val, &s_val, &l_val);
    os_sprintf(json_temp_str, "[%d,%d,%d,%d]", h_val, s_val, l_val, w_now);
    cJSON_AddItemToObject(json_plug_task_send, "hsl", cJSON_Parse(json_temp_str));
    cJSON_AddNumberToObject(json_plug_task_send, "gradient", gradient);

    cJSON_AddItemToObject(pJsonSend, plug_task_str, json_plug_task_send);
    return return_flag;
}

/**
 * 函  数  名: user_json_init
 * 函数说明: json数据处理初始化
 * 参        数: 无
 * 返        回: 无
 */
void ICACHE_FLASH_ATTR user_json_init(void)
{

    zlib_json_init(user_json_deal_cb);
    os_printf("user json init\n");
}
