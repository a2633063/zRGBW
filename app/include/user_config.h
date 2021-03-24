#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "os_type.h"

#define VERSION "v0.0.2"

#define TYPE 8
#define TYPE_NAME "zRGBW"

#define DEVICE_NAME "zRGBW_%02X%02X"
#define MDNS_DEVICE_NAME "zRGBW_%s"

#define USER_CONFIG_VERSION 1

#define SETTING_MQTT_STRING_LENGTH_MAX  64      //必须 4 字节对齐。
#define NAME_LENGTH 32		//插座名称字符串最大长度

#define TIME_TASK_NUM 5    //每个插座最多5组定时任务

typedef struct {
	int8_t hour;      //小时
	int8_t minute;    //分钟
	uint8_t repeat; //bit7:一次   bit6-0:周日-周一
	int8_t on;    //开关
	int8_t action;    //动作
	uint8_t r;    //r
	uint8_t g;    //g
	uint8_t b;    //b
	uint8_t w;    //w
} user_plug_task_config_t;



//用户保存参数结构体
typedef struct {
	char version;
	uint8_t name[NAME_LENGTH];
	uint8_t mqtt_ip[SETTING_MQTT_STRING_LENGTH_MAX];   //mqtt service ip
	uint16_t mqtt_port;        //mqtt service port
	uint8_t mqtt_user[SETTING_MQTT_STRING_LENGTH_MAX];     //mqtt service user
	uint8_t mqtt_password[SETTING_MQTT_STRING_LENGTH_MAX];     //mqtt service user

	uint8_t on;    //记录当前开关
	user_plug_task_config_t task[TIME_TASK_NUM];

} user_config_t;

extern char rtc_init;
extern user_config_t user_config;

extern uint8_t boot_times;
extern uint8_t r;    //r
extern uint8_t g;    //g
extern uint8_t b;    //b
extern uint8_t w;    //w

extern uint8_t r_now;	//记录当前显示颜色,可以为0
extern uint8_t g_now;
extern uint8_t b_now;
extern uint8_t w_now;

extern int8_t on;    //开关
#endif

