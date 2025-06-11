#include "mode.h"
#include "flash.h"
#include <string.h>

// 定义一个有效标志
#define CONFIG_FLAG_VALUE  0xA5A5A5A5

// 保存在flash的系统状态，包括目标WIFI的SSID和PWD，水泵阈值
System_Config_t g_sys_config;

uint8_t mode_flag = 0;
uint8_t pump_flag = 0;
uint8_t connection_flag = 0;
uint8_t pump_threshold = 8;
char esp8266_sta_ssid[32] = "ESP8266_AP";
char esp8266_sta_pwd[32] = "3211382183";

// 系统初始化时，恢复配置
void Load_Config(void)
{
	// 读取Flash里的数据到g_sys_config
    Flash_Read_SystemConfig(&g_sys_config);

	// 获取到数据后判断flag
    if (g_sys_config.flag != CONFIG_FLAG_VALUE) {
        // 第一次使用，或 Flash 数据无效，加载默认值
        strcpy(g_sys_config.sta_ssid, "ESP8266_AP");
        strcpy(g_sys_config.sta_pwd, "3211382183");
        g_sys_config.pump_threshold = 50;
        g_sys_config.flag = CONFIG_FLAG_VALUE;

        // 保存默认配置到 Flash
        // Flash_Write_SystemConfig(&g_sys_config);
    }

    // 应用到系统
    strcpy(esp8266_sta_ssid, g_sys_config.sta_ssid);
    strcpy(esp8266_sta_pwd, g_sys_config.sta_pwd);
    pump_threshold = g_sys_config.pump_threshold;
}

// 存储系统配置到Flash
void Save_Config(void)
{
    g_sys_config.flag = CONFIG_FLAG_VALUE;  // 写入有效标记
	
	// 要把当前的系统配置先保存到g_sys_config结构体，然后写进Flash
	g_sys_config.pump_threshold = pump_threshold;
	strcpy(g_sys_config.sta_ssid, esp8266_sta_ssid);
	strcpy(g_sys_config.sta_pwd, esp8266_sta_pwd);
	
    Flash_Write_SystemConfig(&g_sys_config); // SSID，PWD，pump_threshold
}
