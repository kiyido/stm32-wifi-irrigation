#ifndef __ADC_H
#define __ADC_H	
#include "sys.h"
//////////////////////////////////////////////////////////////////////////////////	 
//������ֻ��ѧϰʹ�ã�δ��������ɣ��������������κ���;
//Mini STM32������
//ADC ��������			   
//����ԭ��@ALIENTEK
//������̳:www.openedv.com
//�޸�����:2010/6/7 
//�汾��V1.0
//��Ȩ���У�����ؾ���
//Copyright(C) ����ԭ�� 2009-2019
//All rights reserved					   
//////////////////////////////////////////////////////////////////////////////////	 
 
void Adc1_Init(void); //ADCͨ����ʼ��
u16  Get_Adc1(u8 ch); //���ĳ��ͨ��ֵ  

void Adc2_Init(void); //ADCͨ����ʼ��
u16  Get_Adc2(u8 ch); //���ĳ��ͨ��ֵ

void Adc3_Init(void); 				//ADC3��ʼ��
u16  Get_Adc3(u8 ch); 				//���ADC3ĳ��ͨ��ֵ  

u8 Soil_Get_Val(void);
#endif 
