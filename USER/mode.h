#ifndef __MODE_H
#define __MODE_H

#include "sys.h"

// 水泵模式
extern uint8_t mode_flag; // 1自动 0手动
// 水泵是否开启
extern uint8_t pump_flag; // 1开启 0关闭
// ESP8266是否与设备连接
extern uint8_t connection_flag; // 1已连接 0未连接
// 自动模式水泵触发阈值
extern uint8_t pump_threshold; // 低于此值开启水泵
// 目标wifi的名称和密码
extern char esp8266_sta_ssid[32];
extern char esp8266_sta_pwd[32];

void Load_Config(void);
void Save_Config(void);

#endif
