#include "mode.h"
#include "flash.h"
#include <string.h>

// ����һ����Ч��־
#define CONFIG_FLAG_VALUE  0xA5A5A5A5

// ������flash��ϵͳ״̬������Ŀ��WIFI��SSID��PWD��ˮ����ֵ
System_Config_t g_sys_config;

uint8_t mode_flag = 0;
uint8_t pump_flag = 0;
uint8_t connection_flag = 0;
uint8_t pump_threshold = 8;
char esp8266_sta_ssid[32] = "ESP8266_AP";
char esp8266_sta_pwd[32] = "3211382183";

// ϵͳ��ʼ��ʱ���ָ�����
void Load_Config(void)
{
	// ��ȡFlash������ݵ�g_sys_config
    Flash_Read_SystemConfig(&g_sys_config);

	// ��ȡ�����ݺ��ж�flag
    if (g_sys_config.flag != CONFIG_FLAG_VALUE) {
        // ��һ��ʹ�ã��� Flash ������Ч������Ĭ��ֵ
        strcpy(g_sys_config.sta_ssid, "ESP8266_AP");
        strcpy(g_sys_config.sta_pwd, "3211382183");
        g_sys_config.pump_threshold = 50;
        g_sys_config.flag = CONFIG_FLAG_VALUE;

        // ����Ĭ�����õ� Flash
        // Flash_Write_SystemConfig(&g_sys_config);
    }

    // Ӧ�õ�ϵͳ
    strcpy(esp8266_sta_ssid, g_sys_config.sta_ssid);
    strcpy(esp8266_sta_pwd, g_sys_config.sta_pwd);
    pump_threshold = g_sys_config.pump_threshold;
}

// �洢ϵͳ���õ�Flash
void Save_Config(void)
{
    g_sys_config.flag = CONFIG_FLAG_VALUE;  // д����Ч���
	
	// Ҫ�ѵ�ǰ��ϵͳ�����ȱ��浽g_sys_config�ṹ�壬Ȼ��д��Flash
	g_sys_config.pump_threshold = pump_threshold;
	strcpy(g_sys_config.sta_ssid, esp8266_sta_ssid);
	strcpy(g_sys_config.sta_pwd, esp8266_sta_pwd);
	
    Flash_Write_SystemConfig(&g_sys_config); // SSID��PWD��pump_threshold
}
