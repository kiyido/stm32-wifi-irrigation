#include <stdio.h>
#include "sys.h"
#include "delay.h" // FreeRTOS �Ѻ���^_^
#include "usart.h"
#include "usart2.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "led.h"
#include "lcd.h"
#include "Picture.h"
#include "dht11.h"
#include "light.h"
#include "soil.h"
#include "key.h"
#include "pump.h"
#include "esp8266.h"

#include "mode.h"
#include "sensor_data.h"

// �������ȼ�
#define START_TASK_PRIO     1
#define TEST01_TASK_PRIO    3
#define TEST02_TASK_PRIO    3
#define TEST03_TASK_PRIO    2
#define TEST04_TASK_PRIO    3
#define TEST05_TASK_PRIO    4
#define TEST06_TASK_PRIO    2

// �����ջ��С����λ���֣�4�ֽ�/�֣�
#define START_STK_SIZE      128
#define TEST01_STK_SIZE     128
#define TEST02_STK_SIZE     128
#define TEST03_STK_SIZE     128
#define TEST04_STK_SIZE     64
#define TEST05_STK_SIZE     128
#define TEST06_STK_SIZE     64

// ������
TaskHandle_t StartTask_Handler;
TaskHandle_t TEST01Task_Handler;
TaskHandle_t TEST02Task_Handler;
TaskHandle_t TEST03Task_Handler;
TaskHandle_t TEST04Task_Handler;
TaskHandle_t TEST05Task_Handler;
TaskHandle_t TEST06Task_Handler;

// ȫ�ֻ��������
SemaphoreHandle_t xMutex_GSensorData;

// ����������
void start_task(void *pvParameters);
void test01_task(void *pvParameters); // ����С������˸
void test02_task(void *pvParameters); // ��ʾ���������
void test03_task(void *pvParameters); // �������ݸ���λ��
void test04_task(void *pvParameters); // ���ذ���ɨ��
void test05_task(void *pvParameters); // ���������ݸ���
void test06_task(void *pvParameters); // ������λ��ָ�����״̬����

int main(void) 
{
    delay_init();
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    USART1_Init(115200);
	USART2_Init(115200);
	
	// ��Flash�м���ϵͳ����
	Load_Config();
    
    LCD_Init();
	LCD_BLK = 0; // ����
    LED_GPIO_Init(); 
    DHT11_GPIO_Config();
    Adc_Init_Light();
	Adc_Init_Soil();
	KEY_GPIO_Init();
	PUMP_GPIO_Init();
	ESP8266_Mode_Init(); // ��Ƕ���ػ�����

    // ������ʼ����
    xTaskCreate(start_task, "start_task", START_STK_SIZE, NULL, START_TASK_PRIO, &StartTask_Handler);
    vTaskStartScheduler();  // ���� FreeRTOS ������
}

// ��ʼ���񣺴������������ɾ������
void start_task(void *pvParameters) 
{
    taskENTER_CRITICAL();
    xTaskCreate(test01_task, "test01_task", TEST01_STK_SIZE, NULL, TEST01_TASK_PRIO, &TEST01Task_Handler);
    xTaskCreate(test02_task, "test02_task", TEST02_STK_SIZE, NULL, TEST02_TASK_PRIO, &TEST02Task_Handler);
    xTaskCreate(test03_task, "test03_task", TEST03_STK_SIZE, NULL, TEST03_TASK_PRIO, &TEST03Task_Handler);
    xTaskCreate(test04_task, "test04_task", TEST04_STK_SIZE, NULL, TEST04_TASK_PRIO, &TEST04Task_Handler);
    xTaskCreate(test05_task, "test05_task", TEST05_STK_SIZE, NULL, TEST05_TASK_PRIO, &TEST05Task_Handler);
    xTaskCreate(test06_task, "test06_task", TEST06_STK_SIZE, NULL, TEST06_TASK_PRIO, &TEST06Task_Handler);
	xMutex_GSensorData = xSemaphoreCreateMutex();
	if (xMutex_GSensorData == NULL) {
		// ������ʧ��
		printf("xSemaphoreCreateMutexErr\r\n");
		while(1);
	}
    vTaskDelete(NULL);
    taskEXIT_CRITICAL();
}

// LED ������˸
void test01_task(void *pvParameters) 
{
    while (1)
	{
        LED = !LED;
		
        delay_ms(1000);
    }
}

// LCD ��ʾ
void test02_task(void *pvParameters) 
{
	// �������棬��ֹ����2�ڵ�������;�У����ݱ�����5�۸�
	DHT11_Data_TypeDef tmp_dht11;
	u8 tmp_light, tmp_soil;
	u16 tmp_light_lux;
	u8 tmp_pump;
	
	char Temp_str[32]; // ��Ÿ�ʽ������ַ���
	
	// ��������
    LCD_Clear(WHITE);
    LCD_Showimage(0, 0, 128, 128, gImage_01);
//	LCD_DrawFont_GBK24(0, 108, BLACK, WHITE, "STM32F103RCT6");
    delay_ms(3000);
    LCD_Clear(WHITE);
	
	// ��Ϣ���
	LCD_DrawFont_GBK24(0, 0, BLACK, WHITE, "��ӭʹ��");
	LCD_DrawFont_GBK16(0, 40, RED, WHITE, "�¶�:");
	LCD_DrawFont_GBK16(0, 56, RED, WHITE, "ʪ��:");
	LCD_DrawFont_GBK16(0, 72, RED, WHITE, "����:");
	LCD_DrawFont_GBK16(0, 88, RED, WHITE, "����:");
	LCD_DrawFont_GBK16(0, 104, RED, WHITE, "ģʽ");
	LCD_DrawFont_GBK16(68, 104, RED, WHITE, "ˮ��");
	
	while(1) {
		// ����������
		if (xSemaphoreTake(xMutex_GSensorData, portMAX_DELAY) == pdTRUE)
		{
			// һ���Ա����µ�ǰ���д���������
			tmp_dht11.temp_int = g_sensor_data.DHT11_Data.temp_int;
			tmp_dht11.temp_deci = g_sensor_data.DHT11_Data.temp_deci;
			tmp_dht11.humi_int = g_sensor_data.DHT11_Data.humi_int;
			tmp_light = g_sensor_data.light;
			tmp_soil = g_sensor_data.soil;
			tmp_light_lux = g_sensor_data.light_lux;
			tmp_pump = pump_threshold;
			
			xSemaphoreGive(xMutex_GSensorData); // �ͷ���
		}
		// �����Զ�ģʽ����ˮ��
		if (mode_flag) 
		{
            if (tmp_soil < pump_threshold) 
			{
                pump_flag = 1;
            } 
			else 
			{
                pump_flag = 0;
            }
        }
		
		sprintf(Temp_str, "%d ", tmp_pump);
		LCD_DrawFont_GBK16(96, 0, BLUE, WHITE, (u8*)Temp_str); // ��ֵ
		
		sprintf(Temp_str, "%d.%d  ", tmp_dht11.temp_int, tmp_dht11.temp_deci);
		LCD_DrawFont_GBK16(48, 40, BLUE, WHITE, (u8*)Temp_str); // �¶�
		
		sprintf(Temp_str, "%d ", tmp_dht11.humi_int);
		LCD_DrawFont_GBK16(48, 56, BLUE, WHITE, (u8*)Temp_str); // ʪ��

		if(tmp_light_lux>=10000) // ��ֹ���
			sprintf(Temp_str, "%d@%u", tmp_light, tmp_light_lux);
		else if(tmp_light_lux<100)
			sprintf(Temp_str, "%d@%u    ", tmp_light, tmp_light_lux);
		else
			sprintf(Temp_str, "%d@%u  ", tmp_light, tmp_light_lux);
		LCD_DrawFont_GBK16(48, 72, BLUE, WHITE, (u8*)Temp_str); // ��Թ�����Luxֵ
		
		sprintf(Temp_str, "%d ", tmp_soil); // ��һ���ո񣬸��Ǿɵĵڶ�λ
		LCD_DrawFont_GBK16(48, 88, BLUE, WHITE, (u8*)Temp_str); // ����
		
		// ����������״̬���£�ģʽ��ˮ��״̬��ʾ����
		if(connection_flag) 
		{
			LCD_DrawFont_GBK16(0, 24, WHITE, GREEN, "   Connected    ");
		} 
		else 
		{
			LCD_DrawFont_GBK16(0, 24, WHITE, RED, "  Disconnected  ");
		}
		if(pump_flag) 
		{
			LCD_DrawFont_GBK16(102, 104, BLUE, WHITE, "��");
			PUMP_ON(); // ��ˮ��
		} 
		else 
		{
			LCD_DrawFont_GBK16(102, 104, BLUE, WHITE, "��");
			PUMP_OFF(); // �ر�ˮ��
		}
		if(mode_flag) 
		{
			LCD_DrawFont_GBK16(34, 104, BLUE, WHITE, "�Զ�");
		} 
		else 
		{
			LCD_DrawFont_GBK16(34, 104, BLUE, WHITE, "�ֶ�");
		}
		
		// ����ϵͳ���õ�Flash
		Save_Config();
		
		delay_ms(1000);
	}
}

// ESP8266���������ݸ���λ��
void test03_task(void *pvParameters) 
{
    while (1) 
	{
		// �������������ݸ���λ��
		if(connection_flag) 
		{
			ESP8266_Send_Data();
		}

		delay_ms(500);
    }
}

// ���ذ���ɨ��
void test04_task(void *pvParameters) 
{
	// ÿ��ִֻ��һ�β�������ֹʹ����ϼ�ʱ���������ܱ�����
	u8 combo_triggered = 0;
	// ��ֹʹ����ϼ����ɿ�ǰ�ü�ʱ�������ܱ�����
	u8 combo_active = 0;
	
	// ������μ�⵽����״̬һ�º󣬲�ȷ��״̬�仯��������Ϊ�Ƕ���
	// ��ǰȥ������ (3 * 10ms = 30ms)
    while (1) 
	{
        key1.now = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_9);
        key2.now = GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_8);

		// ɨ�谴����ȷ��״̬
        scan_key(&key1, key1.now);
        scan_key(&key2, key2.now);
		
        // ��ϼ�1���Ȱ� KEY1���ٰ� KEY2 ̧��ʱ����
        if (key2.last == 0 && key2.stable == 1 && key1.stable == 0) 
        {
			if(pump_threshold < 95)
				pump_threshold += 5;
			else if (pump_threshold >= 95)
				pump_threshold = 100;
			combo_triggered = 1;
			combo_active = 1;
        }

        // ��ϼ�2���Ȱ� KEY2���ٰ� KEY1 ̧��ʱ����
        if (key1.last == 0 && key1.stable == 1 && key2.stable == 0) 
        {
			if(pump_threshold > 5)
				pump_threshold -= 5;
			else if (pump_threshold <= 5)
				pump_threshold = 0;
			combo_triggered = 1;
			combo_active = 1;
        }
		
		// ��ϼ�ȫ���ɿ�������ִ�е�������
		if (combo_active == 0) 
		{
			// �ϴ�Ϊ���£����ȷ�����ɿ���̧�𴥷�
			if (key1.last == 0 && key1.stable == 1 && combo_triggered == 0) 
			{
				mode_flag = !mode_flag;
			}
			if (key2.last == 0 && key2.stable == 1 && combo_triggered == 0) 
			{
				pump_flag = !pump_flag;
			}
		}

		// ���а������ɿ�����ִ�����
		if (key1.stable == 1 && key2.stable == 1)
		{
			combo_active = 0;
		}
		// ��¼�ϴ�״̬
        key1.last = key1.stable;
        key2.last = key2.stable;
		
		combo_triggered = 0;

        delay_ms(10);
    }
}

// ���������ݸ��µ�ȫ�ֱ���
void test05_task(void *pvParameters) 
{
    while (1) 
	{
		// д��������
        if (xSemaphoreTake(xMutex_GSensorData, portMAX_DELAY) == pdTRUE) 
		{
			Read_DHT11(&g_sensor_data.DHT11_Data);
			g_sensor_data.light = Lsens_Get_Val();
			g_sensor_data.light_lux = Lsens_Get_Lux();
			g_sensor_data.soil = Soil_Get_Val();
			
			xSemaphoreGive(xMutex_GSensorData); // �ͷ���
		}
        delay_ms(1000);
    }
}

// ESP8266�������λ�������ִ�У��������״̬������ connection_flag ״̬
void test06_task(void *pvParameters) 
{
	while(1) 
	{
		if (USART2_RX_STA & 0x8000) 
		{ // �ж��Ƿ�������
			USART2_RX_BUF[USART2_RX_STA & 0x7FFF] = 0; // ��ӽ����� '\0'
//			printf("TIPTIP:%s\r\n",USART2_RX_BUF);
			ESP8266_Parse_Command(USART2_RX_BUF); // ��������
			USART2_RX_STA = 0; // ���״̬��׼����һ֡
		}

		delay_ms(50);
	}
}
