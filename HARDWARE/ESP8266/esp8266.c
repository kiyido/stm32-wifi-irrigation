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

// �����ⲿ���������
extern SemaphoreHandle_t xMutex_GSensorData;  

// APģʽ״̬ (AP״̬��ESP8266��Ĭ��IPΪ192.168.4.1)
uint8_t ap_mode_active = 0;
// ����������
DHT11_Data_TypeDef tmp_dht11;
u8 tmp_light, tmp_soil;
u16 tmp_light_lux;
// ��λ������ָ���е���ֵ
int threshold_value;

// ���� AT ָ���ֵ��������
u8 ESP8266_SendCmd(char *cmd, char *ack, u16 wait_time)
{
    USART2_SendString((u8*)cmd);
	
	// �鿴 USART2 �� ESP8266 ������ʲôָ��
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

// ���Ͷ�������
void ESP8266_SendCmdList(char *cmds[], int count, u16 wait_time)
{
	int i;
    for (i = 0; i < count; i++)
	{
        ESP8266_SendCmd(cmds[i], "OK", wait_time);
    }
}

// ���������豸���ȵ㣨STAģʽ��
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

	// ��������PC���ȵ�
    sprintf(cmd, "AT+CWJAP=\"%s\",\"%s\"\r\n", esp8266_sta_ssid, esp8266_sta_pwd);
	
	// �ȴ� ESP8266 ����Ŀ��WIFI�����ӳɹ��᷵�� WIFI CONNECTED
    return ESP8266_SendCmd(cmd, "WIFI CONNECTED", 3000);
}

// ���� AP ģʽ
void ESP8266_Start_AP(void)
{
    char cmd[128];
    char *init_cmds[] = {
        "AT\r\n",
        "ATE0\r\n",
        "AT+CWMODE=2\r\n"
    };

    ESP8266_SendCmdList(init_cmds, 3, 300);

	// ����WIFI�ȵ�����������
	LCD_DrawFont_GBK16(0, 24, RED, WHITE, "SettingAP      ");
    sprintf(cmd, "AT+CWSAP=\"%s\",\"%s\",5,3\r\n", ESP8266_AP_SSID, ESP8266_AP_PWD);
    ESP8266_SendCmd(cmd, "OK", 3000);
	LCD_DrawFont_GBK16(0, 24, RED, WHITE, "AP OK           ");
}

// ���� TCP Server
void ESP8266_Start_TCP_Server(void)
{
    char *cmds[] = {
        "AT+CIPMUX=1\r\n",
		
		// ���ö˿�Ϊ8080
        "AT+CIPSERVER=1,8080\r\n"
    };
    ESP8266_SendCmdList(cmds, 2, 300);
}

// �����ӵĿͻ��˷������ݣ�Ĭ��ͨ��0��
void ESP8266_SendString(char* str)
{
    char cmd[64];
    sprintf(cmd, "AT+CIPSEND=0,%d\r\n", strlen(str));
    if (ESP8266_SendCmd(cmd, ">", 200))
	{
        USART2_SendString((u8*)str);
        delay_ms(50); // ��ֹճ��
    }
	else 
	{
//		printf("ESP8266_SendString failed\r\n");
    }
}

// ����Ƿ����豸���ӵ�ESP8266�ȵ�
// ע��͸��ģʽ���޷���Ӧ�˺�������һֱ�ж�Ϊ����ʧ��
uint8_t ESP8266_Check_STA_Connected(void) 
{
    USART2_RX_STA = 0; // ��ջ�����
    USART2_SendString((u8*)"AT+CWLIF\r\n");
    delay_ms(300);
    USART2_RX_BUF[USART2_RX_STA & 0x3FFF] = 0; // ȷ���ַ�������
//	printf("CWLIF: %s\n", USART2_RX_BUF);
    if (strstr((char*)USART2_RX_BUF, "."))
	{
        // �жϷ������Ƿ��� IP ��ַ����192.168.4.2�����е���Ǿ�����
        USART2_RX_STA = 0;
        return 1;
    }

    // ��ʱ�᷵�� "\r\n\r\nOK\r\n"��ȷ�����ᱻ����
    USART2_RX_STA = 0;
    return 0;
}

// �˳�͸��ģʽ�������Ƿ�ɹ�
uint8_t ESP8266_Exit_TransparentMode(void)
{
    delay_ms(1200);              // ֹͣ������ >1s
    ESP8266_SendString("+++");   // �˳�͸��
    delay_ms(1200);              // �ȴ���Ӧ
    ESP8266_SendString("AT\r\n");
    delay_ms(300);

    // �ж��˳�͸���Ƿ�ɹ�
    // ��Ϊ�̶����� 1���ɼӽ��ս����߼���
    return 1;
}

void ESP8266_Mode_Init(void) 
{
	LCD_Clear(WHITE);
	LCD_DrawFont_GBK16(0, 24, RED, WHITE, "InitWIFI      ");
	
	// ȷ������ʱ�� AT ����ģʽ
	ESP8266_Exit_TransparentMode();
	// �Ͽ��ϴ����ӵ� WIFI
	ESP8266_SendCmd("AT+CWQAP\r\n", "OK", 300);
	
	// ESP8266 һ��ʼ��������Ŀ��WIFI
    if (ESP8266_Connect_STA()) 
	{
        connection_flag = 1;
        ap_mode_active = 0;
        LCD_DrawFont_GBK16(0, 24, RED, WHITE, "STA OK        ");
		
//		delay_ms(2000);
//		// ���ӵ�Ŀ��WIFI�ɹ��󣬻�ȡ����ӡ IP ��ַ
//		USART2_RX_STA = 0;
//		ESP8266_SendCmd("AT+CIFSR\r\n", "OK", 300);
//		USART2_RX_BUF[USART2_RX_STA & 0x3FFF] = 0;
//		// ��ӡ ESP8266 ������� IP ��ַ
//		printf("ESP8266 IP: %s\n", USART2_RX_BUF);
		
		// ���� TCP ����
		ESP8266_SendCmd("AT+CIPSTART=\"TCP\",\"192.168.4.1\",8080\r\n", "OK", 300);
		
		// TCP ���Ӻ�ͽ���͸��ģʽ
        ESP8266_SendCmd("AT+CIPMODE=1\r\n", "OK", 200);
        delay_ms(200);
        ESP8266_SendCmd("AT+CIPSEND\r\n", ">", 200);  // �ȴ� ">" ��ʾ��
	// ESP8266 ����ʧ�ܾ��Լ����ȵ�
    } 
	else 
	{
        connection_flag = 0;
        ap_mode_active = 1;
		LCD_DrawFont_GBK16(0, 24, RED, WHITE, "NoWIFI,RSTforAP ");
		
        // һ��Ҫ�����ٽ���APģʽ
        ESP8266_SendCmd("AT+RST\r\n", "ready", 2000);  // �ȴ�ģ������
        ESP8266_Start_AP();
		
		// ���� TCP ������
		ESP8266_Start_TCP_Server();
    }
}

// ���ݷ��͸���λ��
void ESP8266_Send_Data(void) 
{
	char buffer[64];
	
	// ��ȡ����������
	if (xSemaphoreTake(xMutex_GSensorData, portMAX_DELAY) == pdTRUE) 
    {
		tmp_dht11.temp_int = g_sensor_data.DHT11_Data.temp_int;
		tmp_dht11.temp_deci = g_sensor_data.DHT11_Data.temp_deci;
		tmp_dht11.humi_int = g_sensor_data.DHT11_Data.humi_int;
		tmp_light = g_sensor_data.light;
		tmp_soil = g_sensor_data.soil;
		tmp_light_lux = g_sensor_data.light_lux;
		
		xSemaphoreGive(xMutex_GSensorData); // �ͷ���
	}
	
	// �������ݸ���λ��
	if (connection_flag) 
	{
		// ����֡��ʽ
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

// ��λ����ָ��� ESP8266 ��֪ͨ�����ڴ���2
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
		// �Զ�ģʽҲ���Ե�������ˮ�ã���ˮ����Ȼ���Զ�����
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
        // ��ȡ��ֵ
        char* ptr = strstr((const char*)buf, "#THRESHOLD=");
        if (ptr != NULL)
		{
            int threshold_value = atoi(ptr + 11);  // ��"#THRESHOLD="���11λ��ʼ�����֣�����תΪ int
            if (threshold_value >= 0 && threshold_value <= 100) 
			{
                pump_threshold = (uint8_t)threshold_value;
            }
        }
	// ESP8266 �� AP ״̬�µ����豸���ӻ�Ͽ�ʱ���°�̼���ʹ���첽�¼�֪ͨ
	// ESP8266 �� STA ״̬�·���WIFI�Ͽ��ˣ�Ҳ���첽�¼�֪ͨ�����ҶϿ�����Զ�����
    } 
	else if (strstr((const char*)buf, "+STA_CONNECTED") ||
			 strstr((const char*)buf, "WIFI CONNECTED")) 
	{
		connection_flag = 1;
		
//		delay_ms(2000);
//		USART2_RX_STA = 0;
//		ESP8266_SendCmd("AT+CIFSR\r\n", "OK", 300);
//		USART2_RX_BUF[USART2_RX_STA & 0x3FFF] = 0;
//		// ��ӡ ESP8266 ������� IP ��ַ
//		printf("ESP8266 IP: %s\n", USART2_RX_BUF);
		
		// ����ʱ����͸��
		ESP8266_SendString("AT+CIPMODE=1\r\n");
		delay_ms(200);
		ESP8266_SendString("AT+CIPSEND\r\n");
		delay_ms(200);
	} 
	else if (strstr((const char*)buf, "+STA_DISCONNECTED") || 
			 strstr((const char*)buf, "WIFI DISCONNECT")) 
	{
		connection_flag = 0;
		
		// �Ͽ�ʱ�˳�͸��
		ESP8266_Exit_TransparentMode();
	}
	else if (strstr((const char*)buf, "#SSID=") != NULL && 
			 strstr((const char*)buf, "PWD=") != NULL)
    {
        // ���� SSID �� PWD
        char* ssid_start = strstr((const char*)buf, "#SSID=") + 6;
        char* pwd_start = strstr((const char*)buf, "PWD=") + 4;
        char* ssid_end = strstr((const char*)buf, "PWD=") - 1;  // SSID �� PWD ֮��ķֽ�㣨�м�Ӧ���и��ո�������ָ�����

        int ssid_len = ssid_end - ssid_start;
        int pwd_len = 0;

        // ���ҽ�β #
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
