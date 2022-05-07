#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__

//#define USE_OPTIMIZE_PRINTF

#include "os_type.h"
#include "user_setting.h"
#define BCDtoDEC(x) ( ((x)>>4)*10+(x)%0x10  )           //BCD码转换为十进制表示方式
#define DECtoBCD(x) ( (((x)/10)<<4)+(x)%10  )           //十进制转换为BCD码表示方式

#define ZLIB_WIFI_CALLBACK_REPEAT  (0)  //wifi连接过程中 是否重复调用回调函数
#define ZLIB_WIFI_MDSN_ENABLI  (1)      //是否启用mdns,默认不启用mdns

#define ZLIB_UDP_REPLY_PORT  (10181)  //udp回复默认端口号
#define ZLIB_WIFI_CONNECTED_STATE_LED  (1)  //wifi连接后 wifi状态指示灯io口电平
//#define ZLIB_WIFI_STATE_LED_IO_MUX     PERIPHS_IO_MUX_GPIO2_U
//#define ZLIB_WIFI_STATE_LED_IO_NUM     2
//#define ZLIB_WIFI_STATE_LED_IO_FUNC    FUNC_GPIO2

#define ZLIB_WEB_CONFIG_ONLY  (1)  //若使用了自定义webserver 此应该设置为0 若仅用web配网 此置1 且调用zlib_web_wifi_init即可
#define ZLIB_SETTING_SAVE_ADDR  (SETTING_SAVE_ADDR) 	//设置储存地址


#define VERSION "v0.1.0"

#define TYPE 8

#define TYPE_NAME "zRGBW"
#define DEVICE_NAME "zRGBW_%s"


#define USER_CONFIG_VERSION 2

#define SETTING_MQTT_STRING_LENGTH_MAX  64      //必须 4 字节对齐。
#define NAME_LENGTH 32		//插座名称字符串最大长度


#define TIME_TASK_NUM 5    //每个插座最多5组定时任务

typedef struct {
    int8_t hour;      //小时
    int8_t minute;    //分钟
    uint8_t repeat; //bit7:一次   bit6-0:周日-周一
    int8_t on;    //定时任务开关
    int8_t action;    //开关动作,未使用,直接使用rgbw作为开关状态
    uint8_t r;    //r
    uint8_t g;    //g
    uint8_t b;    //b
    uint8_t w;    //w
    uint8_t gradient;   //渐变效果
} user_time_task_config_t;

//用户保存参数结构体
typedef struct
{
    char version;    //配置文件版本
    uint8_t name[NAME_LENGTH];
    uint8_t mqtt_ip[SETTING_MQTT_STRING_LENGTH_MAX];   //mqtt service ip
    uint16_t mqtt_port;        //mqtt service port
    uint8_t mqtt_user[SETTING_MQTT_STRING_LENGTH_MAX];     //mqtt service user
    uint8_t mqtt_password[SETTING_MQTT_STRING_LENGTH_MAX];     //mqtt service user
    uint8_t on;    //预留,未使用
    uint8_t gpio[5];        //rgbw  预留rgbww
    uint16_t auto_off;  //每次打开后自动关闭时间,单位s. 为0时关闭此功能
    user_time_task_config_t task[TIME_TASK_NUM];
} user_config_t;


extern user_config_t user_config;


extern uint8_t r;    //r
extern uint8_t g;    //g
extern uint8_t b;    //b
extern uint8_t w;    //w

extern uint8_t r_now;   //记录当前显示颜色,可以为0
extern uint8_t g_now;
extern uint8_t b_now;
extern uint8_t w_now;

extern int8_t on;    //开关

#endif

