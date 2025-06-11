#ifndef __SOIL_H
#define __SOIL_H	
#include "sys.h"


void Adc_Init_Soil(void);
u16 Get_Adc_Average_Soil(u8 ch,u8 times); 
u8 Soil_Get_Val(void); 

#endif 
