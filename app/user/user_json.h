#ifndef __USER_JSON_H__
#define __USER_JSON_H__

void ICACHE_FLASH_ATTR user_json_init(void);
void ICACHE_FLASH_ATTR user_json_deal_cb(void *arg, Wifi_Comm_type_t type, cJSON * pJsonRoot, void *p);
#endif

