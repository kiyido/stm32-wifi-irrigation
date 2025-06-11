#ifndef __SENSOR_DATA_H
#define __SENSOR_DATA_H

#include "sys.h"
#include "dht11.h"

// 所有传感器数据
typedef struct {
	DHT11_Data_TypeDef DHT11_Data;
	uint8_t light;
	uint16_t light_lux;
	uint8_t soil;
} SensorData_t;

extern SensorData_t g_sensor_data;

#endif
