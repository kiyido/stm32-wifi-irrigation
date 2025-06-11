#include "esp8266.h"
#include "usart2.h"
#include "delay.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "semphr.h"
#include "LCD.h"
#include "mode.h"
#include "dht11.h"
#include "sensor_data.h"
#include "flash.h"

// 声明外部互斥锁句柄
extern SemaphoreHandle_t xMutex_GSensorData;  

// AP模式状态 (AP状态下ESP8266的默认IP为192.168.4.1)
uint8_t ap_mode_active = 0;
// 传感器数据
DHT11_Data_TypeDef tmp_dht11;
u8 tmp_light, tmp_soil;
u16 tmp_light_lux;
// 上位机发送指令中的阈值
int threshold_value;

// 接收 AT 指令返回值辅助函数
u8 ESP8266_SendCmd(char *cmd, char *ack, u16 wait_time)
{
    USART2_SendString((u8*)cmd);
	
	// 查看 USART2 向 ESP8266 发送了什么指令
//	printf("ESP8266_SendCmd: %s\r\n",cmd);
	
    delay_ms(wait_time);

    if (strstr((char*)USART2_RX_BUF, ack) != NULL)
	{
        USART2_RX_STA = 0;
        return 1;
    }
//	printf("Recv: %s\n", USART2_RX_BUF);
    USART2_RX_STA = 0;
    return 0;
}

// 发送多条命令
void ESP8266_SendCmdList(char *cmds[], int count, u16 wait_time)
{
	int i;
    for (i = 0; i < count; i++)
	{
        ESP8266_SendCmd(cmds[i], "OK", wait_time);
    }
}

// 连接其它设备的热点（STA模式）
u8 ESP8266_Connect_STA(void)
{
    char cmd[128];
    char *init_cmds[] = {
        "AT\r\n",
        "ATE0\r\n",
        "AT+CWMODE=1\r\n"
    };
	
	LCD_DrawFont_GBK16(0, 24, RED, WHITE, "Connect2WIFI    ");
    ESP8266_SendCmdList(init_cmds, 3, 300);

	// 尝试连接PC的热点
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", esp8266_sta_ssid, esp8266_sta_pwd);
	
	// 等待 ESP8266 连接目标WIFI，连接成功会返回 WIFI CONNECTED
    return ESP8266_SendCmd(cmd, "WIFI CONNECTED", 3000);
}

// 启动 AP 模式
void ESP8266_Start_AP(void)
{
    char cmd[128];
    char *init_cmds[] = {
        "AT\r\n",
        "ATE0\r\n",
        "AT+CWMODE=2\r\n"
    };

    ESP8266_SendCmdList(init_cmds, 3, 300);

	// 设置WIFI热点名称与密码
	LCD_DrawFont_GBK16(0, 24, RED, WHITE, "SettingAP      ");
    sprintf(cmd, "AT+CWSAP=\"%s\",\"%s\",5,3\r\n", ESP8266_AP_SSID, ESP8266_AP_PWD);
    ESP8266_SendCmd(cmd, "OK", 3000);
	LCD_DrawFont_GBK16(0, 24, RED, WHITE, "AP OK           ");
}

// 启动 TCP Server
void ESP8266_Start_TCP_Server(void)
{
    char *cmds[] = {
        "AT+CIPMUX=1\r\n",
		
		// 设置端口为8080
        "AT+CIPSERVER=1,8080\r\n"
    };
    ESP8266_SendCmdList(cmds, 2, 300);
}

// 向连接的客户端发送数据（默认通道0）
void ESP8266_SendString(char* str)
{
    char cmd[64];
    sprintf(cmd, "AT+CIPSEND=0,%d\r\n", strlen(str));
    if (ESP8266_SendCmd(cmd, ">", 200))
	{
        USART2_SendString((u8*)str);
        delay_ms(50); // 防止粘包
    }
	else 
	{
//		printf("ESP8266_SendString failed\r\n");
    }
}

// 检查是否有设备连接到ESP8266热点
// 注意透传模式下无法响应此函数，会一直判定为连接失败
uint8_t ESP8266_Check_STA_Connected(void) 
{
    USART2_RX_STA = 0; // 清空缓冲区
    USART2_SendString((u8*)"AT+CWLIF\r\n");
    delay_ms(300);
    USART2_RX_BUF[USART2_RX_STA & 0x3FFF] = 0; // 确保字符串结束
//	printf("CWLIF: %s\n", USART2_RX_BUF);
    if (strstr((char*)USART2_RX_BUF, "."))
	{
        // 判断返回中是否有 IP 地址（如192.168.4.2），有点号那就是了
        USART2_RX_STA = 0;
        return 1;
    }

    // 有时会返回 "\r\n\r\nOK\r\n"，确保不会被误判
    USART2_RX_STA = 0;
    return 0;
}

// 退出透传模式并返回是否成功
uint8_t ESP8266_Exit_TransparentMode(void)
{
    delay_ms(1200);              // 停止发数据 >1s
    ESP8266_SendString("+++");   // 退出透传
    delay_ms(1200);              // 等待响应
    ESP8266_SendString("AT\r\n");
    delay_ms(300);

    // 判断退出透传是否成功
    // 简化为固定返回 1（可加接收解析逻辑）
    return 1;
}

void ESP8266_Mode_Init(void) 
{
	LCD_Clear(WHITE);
	LCD_DrawFont_GBK16(0, 24, RED, WHITE, "InitWIFI      ");
	
	// 确保开机时在 AT 命令模式
	ESP8266_Exit_TransparentMode();
	// 断开上次连接的 WIFI
	ESP8266_SendCmd("AT+CWQAP\r\n", "OK", 300);
	
	// ESP8266 一开始尝试连接目标WIFI
    if (ESP8266_Connect_STA()) 
	{
        connection_flag = 1;
        ap_mode_active = 0;
        LCD_DrawFont_GBK16(0, 24, RED, WHITE, "STA OK        ");
		
//		delay_ms(2000);
//		// 连接到目标WIFI成功后，获取并打印 IP 地址
//		USART2_RX_STA = 0;
//		ESP8266_SendCmd("AT+CIFSR\r\n", "OK", 300);
//		USART2_RX_BUF[USART2_RX_STA & 0x3FFF] = 0;
//		// 打印 ESP8266 被分配的 IP 地址
//		printf("ESP8266 IP: %s\n", USART2_RX_BUF);
		
		// 建立 TCP 连接
		ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"192.168.4.1\",8080\r\n", "OK", 300);
		
		// TCP 连接后就进入透传模式
        ESP8266_SendCmd("AT+CIPMODE=1\r\n", "OK", 200);
        delay_ms(200);
        ESP8266_SendCmd("AT+CIPSEND\r\n", ">", 200);  // 等待 ">" 提示符
	// ESP8266 连接失败就自己开热点
    } 
	else 
	{
        connection_flag = 0;
        ap_mode_active = 1;
		LCD_DrawFont_GBK16(0, 24, RED, WHITE, "NoWIFI,RSTforAP ");
		
        // 一定要重启再进入AP模式
        ESP8266_SendCmd("AT+RST\r\n", "ready", 2000);  // 等待模块重启
        ESP8266_Start_AP();
		
		// 启动 TCP 服务器
		ESP8266_Start_TCP_Server();
    }
}

// 数据发送给上位机
void ESP8266_Send_Data(void) 
{
	char buffer[64];
	
	// 读取传感器数据
	if (xSemaphoreTake(xMutex_GSensorData, portMAX_DELAY) == pdTRUE) 
    {
		tmp_dht11.temp_int = g_sensor_data.DHT11_Data.temp_int;
		tmp_dht11.temp_deci = g_sensor_data.DHT11_Data.temp_deci;
		tmp_dht11.humi_int = g_sensor_data.DHT11_Data.humi_int;
		tmp_light = g_sensor_data.light;
		tmp_soil = g_sensor_data.soil;
		tmp_light_lux = g_sensor_data.light_lux;
		
		xSemaphoreGive(xMutex_GSensorData); // 释放锁
	}
	
	// 发送数据给上位机
	if (connection_flag) 
	{
		// 数据帧格式
		sprintf(buffer, "#%d.%d-%d-%d-%d-%d-%d-%d-%u#\n", 
        tmp_dht11.temp_int, 
        tmp_dht11.temp_deci, 
        tmp_dht11.humi_int, 
        tmp_light, 
        tmp_soil,
        mode_flag ? 1 : 0,
        pump_flag ? 1 : 0,
		pump_threshold,
		tmp_light_lux);
        ESP8266_SendString(buffer);
    }
}

// 上位机的指令和 ESP8266 的通知都会在串口2
void ESP8266_Parse_Command(uint8_t* buf) 
{
    if (strstr((const char*)buf, "#AUTO#") != NULL) 
	{
        mode_flag = 1;
    } 
	else if 
	(strstr((const char*)buf, "#MANUAL#") != NULL) 
	{
        mode_flag = 0;
		// 自动模式也可以单独控制水泵，但水泵仍然被自动控制
    } 
	else if (strstr((const char*)buf, "#OPEN#") != NULL) 
	{
        pump_flag = 1;
    } 
	else if (strstr((const char*)buf, "#CLOSE#") != NULL) 
	{
        pump_flag = 0;
	} 
	else if (strstr((const char*)buf, "#THRESHOLD=") != NULL)
	{
        // 提取阈值
        char* ptr = strstr((const char*)buf, "#THRESHOLD=");
        if (ptr != NULL)
		{
            int threshold_value = atoi(ptr + 11);  // 从"#THRESHOLD="后第11位开始是数字，将其转为 int
            if (threshold_value >= 0 && threshold_value <= 100) 
			{
                pump_threshold = (uint8_t)threshold_value;
            }
        }
	// ESP8266 在 AP 状态下当有设备连接或断开时，新版固件会使用异步事件通知
	// ESP8266 在 STA 状态下发现WIFI断开了，也会异步事件通知，并且断开后会自动重连
    } 
	else if (strstr((const char*)buf, "+STA_CONNECTED") ||
			 strstr((const char*)buf, "WIFI CONNECTED")) 
	{
		connection_flag = 1;
		
//		delay_ms(2000);
//		USART2_RX_STA = 0;
//		ESP8266_SendCmd("AT+CIFSR\r\n", "OK", 300);
//		USART2_RX_BUF[USART2_RX_STA & 0x3FFF] = 0;
//		// 打印 ESP8266 被分配的 IP 地址
//		printf("ESP8266 IP: %s\n", USART2_RX_BUF);
		
		// 连接时启用透传
		ESP8266_SendString("AT+CIPMODE=1\r\n");
		delay_ms(200);
		ESP8266_SendString("AT+CIPSEND\r\n");
		delay_ms(200);
	} 
	else if (strstr((const char*)buf, "+STA_DISCONNECTED") || 
			 strstr((const char*)buf, "WIFI DISCONNECT")) 
	{
		connection_flag = 0;
		
		// 断开时退出透传
		ESP8266_Exit_TransparentMode();
	}
	else if (strstr((const char*)buf, "#SSID=") != NULL && 
			 strstr((const char*)buf, "PWD=") != NULL)
    {
        // 解析 SSID 和 PWD
        char* ssid_start = strstr((const char*)buf, "#SSID=") + 6;
        char* pwd_start = strstr((const char*)buf, "PWD=") + 4;
        char* ssid_end = strstr((const char*)buf, "PWD=") - 1;  // SSID 到 PWD 之间的分界点（中间应该有个空格或其它分隔符）

        int ssid_len = ssid_end - ssid_start;
        int pwd_len = 0;

        // 查找结尾 #
        char* pwd_end = strchr((const char*)pwd_start, '#');
        if (pwd_end != NULL)
        {
            pwd_len = pwd_end - pwd_start;
        }
        else
        {
            pwd_len = strlen((const char*)pwd_start);
        }

        if (ssid_len > 0 && ssid_len < sizeof(esp8266_sta_ssid) &&
            pwd_len > 0 && pwd_len < sizeof(esp8266_sta_pwd))
        {
            strncpy(esp8266_sta_ssid, ssid_start, ssid_len);
            esp8266_sta_ssid[ssid_len] = '\0';

            strncpy(esp8266_sta_pwd, pwd_start, pwd_len);
            esp8266_sta_pwd[pwd_len] = '\0';
        }
    }
}
