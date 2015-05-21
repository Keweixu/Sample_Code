/**
 * @file   main_sensor.h
 * @author Kewei Xu, based off of file test_DS18B20.h by Bob Reese
 *
 * @brief  This file describes the parameters for the main sensor for the Egburt
 *         to measure blood pressure, pulse rate, and oxy saturation
 *
 */

#ifndef __MAIN_SENSOR_
#define __MAIN_SENSOR_

//these contain definitions for many of the types used below
#include "config.h"
#include "sirConfigDefaults.h"
#include "firmwareDefaults.h"
#include "sensor.h"

//unit conversion macros
#define CELSIUS2UKELVIN(x)  ((x*1000000L) + 274150000L)
#define UKELVIN2CELSIUS(x)  ((x - 274150000L)/1000000L)

#define FAREN2UKELVIN(x) (((((x*1000000L)-(32*1000000L))*5)/9)+ 274150000L)

//values that are used in configuration files. this section is optional but keeps things neat
//when reusing a sensor across multiple configurations as some settings have default values
//or values that should not need to be changed. the configuration file can simply reference
//these values instead of explicitly defining values. see included configuration files for more
//info on what these fields do
#define MAIN_SENSOR_NAME       					"mainSensor"
#define MAIN_SENSOR_VERSION							"1.0"
#define MAIN_SENSOR_HAS_INTERRUPTS						0
#define MAIN_SENSOR_MIN_VALUE                  CELSIUS2UKELVIN(-55)
#define MAIN_SENSOR_MAX_VALUE                  CELSIUS2UKELVIN(125)
#define MAIN_SENSOR_SUBID						SIR_DEFAULT_SENSOR_SUBID
#define MAIN_SENSOR_HIGH_NOTICE					    0 // fill later, before testing
#define MAIN_SENSOR_HIGH_WARN					    0
#define MAIN_SENSOR_HIGH_CRIT				    	0
#define MAIN_SENSOR_HIGH_CLEAR_TRIGGER			    0
#define MAIN_SENSOR_LOW_NOTICE					    0
#define MAIN_SENSOR_LOW_WARN				    	0
#define MAIN_SENSOR_LOW_CRIT				    	0
#define MAIN_SENSOR_LOW_CLEAR_TRIGGER		    	0

#ifdef TEST  //for unit tests
#define MAIN_SENSOR_STATISTICS_INTERVAL			1
#define MAIN_SENSOR_SAMPLETIME		    			20
#else
#define MAIN_SENSOR_STATISTICS_INTERVAL			0   //disabled as this is a hardware test
#define MAIN_SENSOR_SAMPLETIME			    10
#endif

#define MAIN_SENSOR_UPDATE_THRESHOLD		    	0 //disable
#define MAIN_SENSOR_SD_UPDATE_THRESHOLD			    0 //disable

#define MAIN_SENSOR_ABNORMAL_HIGH_CONDITION                    (uint16_t)CONDITION_NONE
#define MAIN_SENSOR_ABNORMAL_HIGH_THRESHOLD                    1
#define MAIN_SENSOR_ABNORMAL_HIGH_TIME_MINUTES                 60

#define MAIN_SENSOR_ABNORMAL_HIGH_CONDITION_CLEAR              (uint16_t)CONDITION_NONE
#define MAIN_SENSOR_ABNORMAL_HIGH_THRESHOLD_CLEAR              1
#define MAIN_SENSOR_ABNORMAL_HIGH_TIME_MINUTES_CLEAR           60

#define MAIN_SENSOR_ABNORMAL_LOW_CONDITION          (uint16_t)CONDITION_NONE
#define MAIN_SENSOR_ABNORMAL_LOW_THRESHOLD          1
#define MAIN_SENSOR_ABNORMAL_LOW_TIME_MINUTES       60

#define MAIN_SENSOR_ABNORMAL_LOW_CONDITION_CLEAR    (uint16_t)CONDITION_NONE
#define MAIN_SENSOR_ABNORMAL_LOW_THRESHOLD_CLEAR    1
#define MAIN_SENSOR_ABNORMAL_LOW_TIME_MINUTES_CLEAR 60

#define MAIN_SENSOR_START_DISABLE_HOUR      0
#define MAIN_SENSOR_START_DISABLE_MINUTE   0
#define MAIN_SENSOR_END_DISABLE_HOUR        0
#define MAIN_SENSOR_END_DISABLE_MINUTE      0

#define MAIN_SENSOR_DISABLE_ALERT_MESSAGE         1
#define MAIN_SENSOR_DISABLE_UPDATE_MESSAGE        1
#define MAIN_SENSOR_DISABLE_STATISTICS_MESSAGE        1



#define MAIN_SENSOR_TYPE						(uint16_t)SENSOR_TYPE_PRESSURE
#define MAIN_SENSOR_PRIMITIVE_DTYPE             (uint16_t)SENSOR_PRIM_DTYPE_UINT    //units in uKelvin

#define MAIN_SENSOR_IFACE_TYPE                  (uint16_t)INTERFACE_ASYNC_SERIAL
#define MAIN_SENSOR_IFACE_VCC_5V                1
#define MAIN_SENSOR_IFACE_VCCIO_5V              1

//sensor specifics 
#define STX 0xFD //start text
#define ETX 0xFE //end text 
#define DELIM 0x3B//delimeter
#define CR 0x0D //carriage Return

//This defines a structure for storing configuration values (such as the ones listed above).
//For most sensors (this one included), the standard SipSensorConfig (physical sensor config)
//is sufficient. This structure will be pointed to the actual sensor configuration defined
//in EEPROM so that the configuration values can be easily accessed in the code.
typedef struct {
	SipSensorConfig super;              //this is a Sip-connected sensor
}mainSensorConfig, *mainSensorConfigPtr __attribute__ ((aligned (1)));

//This defines the constructor for the sensor object which will acquire memory and initialize
//the object. Simply follow the same format as this (as well as in the actual definition)
//in your own code.
ActiveObjectPtr mainSensorCtor(mainSensorConfigPtr config);

//This defines a structure for accessing active object related data. For most sensors
//(this one included), the standard SensorActiveObject is sufficient.
typedef struct mainSensorActiveObject *mainSensorActiveObjectPtr;

//Unused. The standard SensorStateHandler type in sensor.h is sufficient
typedef void (*mainSensorStateHandler)(mainSensorActiveObjectPtr, EventConstPtr );

//This processes the blood pressure data


//see above testDS18B20ActiveObjectPtr
typedef struct mainSensorActiveObject {
	SensorActiveObject super;
} mainSensorActiveObject, *mainSensorActiveObjectPtr;


#endif /* __MAIN_SENSOR_ */
