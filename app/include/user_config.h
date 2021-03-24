#ifndef __USER_CONFIG_H__
#define __USER_CONFIG_H__
#include "os_type.h"

#define VERSION "v0.0.2"

#define TYPE 8
#define TYPE_NAME "zRGBW"

#define DEVICE_NAME "zRGBW_%02X%02X"
#define MDNS_DEVICE_NAME "zRGBW_%s"

#define USER_CONFIG_VERSION 1

#define SETTING_MQTT_STRING_LENGTH_MAX  64      //���� 4 �ֽڶ��롣
#define NAME_LENGTH 32		//���������ַ�����󳤶�

#define TIME_TASK_NUM 5    //ÿ���������5�鶨ʱ����

typedef struct {
	int8_t hour;      //Сʱ
	int8_t minute;    //����
	uint8_t repeat; //bit7:һ��   bit6-0:����-��һ
	int8_t on;    //����
	int8_t action;    //����
	uint8_t r;    //r
	uint8_t g;    //g
	uint8_t b;    //b
	uint8_t w;    //w
} user_plug_task_config_t;



//�û���������ṹ��
typedef struct {
	char version;
	uint8_t name[NAME_LENGTH];
	uint8_t mqtt_ip[SETTING_MQTT_STRING_LENGTH_MAX];   //mqtt service ip
	uint16_t mqtt_port;        //mqtt service port
	uint8_t mqtt_user[SETTING_MQTT_STRING_LENGTH_MAX];     //mqtt service user
	uint8_t mqtt_password[SETTING_MQTT_STRING_LENGTH_MAX];     //mqtt service user

	uint8_t on;    //��¼��ǰ����
	user_plug_task_config_t task[TIME_TASK_NUM];

} user_config_t;

extern char rtc_init;
extern user_config_t user_config;

extern uint8_t boot_times;
extern uint8_t r;    //r
extern uint8_t g;    //g
extern uint8_t b;    //b
extern uint8_t w;    //w

extern uint8_t r_now;	//��¼��ǰ��ʾ��ɫ,����Ϊ0
extern uint8_t g_now;
extern uint8_t b_now;
extern uint8_t w_now;

extern int8_t on;    //����
#endif

