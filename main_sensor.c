/**
* @file   main_sensor.c
* @author Kewei Xu, based off of file test_DS18B20.c by Bob Reese
*
* @brief  This file describes the functions for the main sensor for the Egburt
*         to measure blood pressure, pulse rate, and oxy saturation
*
*/

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

//these includes will most likely contain everything that you will need to
//work with a sensor object
#include "config.h"
#include "sirConfigDefaults.h"
#include "event.h"
#include "eventpool.h"
#include "eventqueue.h"
#include "activeobject.h"
#include "scheduler.h"
#include "timer.h"
#include "logger.h"
#include "util.h"
#include "configutil.h"
#include "main_sensor.h"
#include "firmwareDefaults.h"
#include "sip_api.h"
#include "ao_sipcomm.h"
#include "ao_mcp.h"


//local state definitions for handleSampleFSM
enum {
	MAIN_SENSOR_INIT,
	MAIN_SENSOR_SAMPLE,
	SAMPLE_ACK

};

//DO NOT CHANGE
//definition of FSM used for sensor object control
// me - ptr to destination object (this MAIN_SENSOR object). used to access config, ao fields, etc.
// event - ptr to event. this contains a signal (input), a sender ao, and possibly data from sensors
static void handleSampleFSM(mainSensorActiveObjectPtr me, EventPtr event);

int bloodPressureEvent(int counter,mainSensorActiveObjectPtr me, EventPtr event);

int incrementCounter(int counter);


// DO NOT CHANGE
//this is called whenever the sensor FSM is ready sample, this will point event handling
//to a local FSM in this object
void mainSensorSampleCallback(SensorActiveObjectPtr me){
	sensorStateTransition(me, (SensorStateHandler )handleSampleFSM, MAIN_SENSOR_INIT, STATE_TRAN_SIG);
}

// DO NOT CHANGE
//This function allocates the active object and event queues, and does any needed initialization
ActiveObjectPtr mainSensorCtor(mainSensorConfigPtr config){
	mainSensorActiveObjectPtr me;
	EventConstPtr *myQueues;


	me = (mainSensorActiveObjectPtr) configutilCalloc(1, sizeof (mainSensorActiveObject) );
	myQueues = (EventConstPtr *) configutilCalloc(FIRMWARE_DEFAULT_POLLED_SENSOR_QUEUE_SIZE,sizeof(EventConstPtr *));
	SensorCtor(&me->super,  myQueues, FIRMWARE_DEFAULT_POLLED_SENSOR_QUEUE_SIZE, (SensorSuperConfigPtr) config);
	me->super.sampleCallback = mainSensorSampleCallback;

	return &me->super.ao_super;
}

//this function contains the main control logic for this sensor object. it is called whenever it is
//time for the sensor to be sampled
static void handleSampleFSM(mainSensorActiveObjectPtr me, EventPtr event)
{
	char buffer[64];

	//there is a global 30s timeout for sensor objects. a TIMEOUT_SIG is sent when this time elapses
	//and the sensor object has not finished. if the timeout expires, an error is logged and control is
	//returned to sensor.c
	if(event->signal == TIMEOUT_SIG) {
		sprintf(buffer,"MAIN_SENSOR TIMEOUT, Port: %d SubID: %d",me->super.config->sid.portId, me->super.config->sid.subId);
		LOGGER(LOG_ERROR, SUBSYSTEM_ID_SENSOR_AO, 0, buffer);
		//this clears any existing signals in this sensor object's queue. at this point, we are giving up
		//so we do not want any remaining signals as this would cause confusion
		eventQueueFlush(&(me->super.ao_super.eventQueue));\
			//use this function call to return control to the parent sensor.c
			sensorStateTransition(&(me->super), sensorHandleDefaultState, SENSOR_STATE_SAMPLE_MAIN_RETURN, STATE_TRAN_SIG);
		return;
	}

	switch(me->super.state) {
		//this is our initial state
	case MAIN_SENSOR_INIT:
		{
			if (event->signal == STATE_TRAN_SIG) {
				LOGGER(LOG_INFO, SUBSYSTEM_ID_SENSOR_AO, 0, "Initializing NIBP");
				bspPrint("Initializing NIBP\r\n");
				SipCommSetEventReturn(EVENT_TYPE_SIMPLE,ACK_SIG);
				buffer[0] = 0;	//read first byte
				SIP_SensorUartWrite(me->super.config->sid, buffer, 1);
			} else if (event->signal == ACK_SIG) {
			  	LOGGER(LOG_INFO, SUBSYSTEM_ID_SENSOR_AO, 0, "Writing request to sensor");
				bspPrint("Writing request to sensor\r\n"); 
			  	me->super.state = MAIN_SENSOR_SAMPLE;
			  	SipCommSetEventReturn(EVENT_TYPE_SIMPLE, STATE_TRAN_SIG);
			  	buffer[0] = STX;
				buffer[1] = '0';
				buffer[2] = '1';
				buffer[3] = DELIM;
				buffer[4] = DELIM;
				buffer[5] = 'D';
				buffer[6] = '7';
				buffer[7] = ETX;//read first byte
			  	SIP_SensorUartWrite(me->super.config->sid, buffer, 8); //send 8 bytes

			} else if (event->signal == TIMEOUT_SIG) {
					//FIRMWARE_SENSOR_SAMPLE_TIMEOUT from sensor.c
					sprintf(buffer,"EARTEMP timeout, EARTEMP_WAKEUP, Port: %d SubID: %d",me->super.config->sid.portId, me->super.config->sid.subId);
					eventQueueFlush(&(me->super.ao_super.eventQueue));
					LOGGER(LOG_ERROR, SUBSYSTEM_ID_SENSOR_AO, 0, buffer);
					sensorStateTransition(&(me->super), sensorHandleDefaultState, SENSOR_STATE_SAMPLE_MAIN_RETURN, STATE_TRAN_SIG);

			} else {
				UNEXPECTED_SIGNAL("ear_temp.c:handleSampleFSM", "EAR_TEMP_WAKEUP",event->signal);
			}
		}

	case  MAIN_SENSOR_SAMPLE:
		{
			//this is our initial signal. here we send a command to the sensor to perform a reading
			if (event->signal == STATE_TRAN_SIG) {
				//declare this to make things a tiny bit more readable :)
				delayMs(15000);
				mainSensorConfigPtr myconfig = (mainSensorConfigPtr)me->super.config;
				LOGGER(LOG_INFO, SUBSYSTEM_ID_SENSOR_AO, 0, "Reading data from MAIN_SENSOR");
				bspPrint("Reading data from MAIN_SENSOR");
				//when we send a command to the sensor, it will return an event once is has finished
				//this function sets the return type of the event as well as the returned signal
				//EVENT_TYPE_U8ARRAY - data is returned as an array of bytes (uint8's)
				//ACK_SIG - this FSM will receive an ACK_SIG when the sensor has finished
				SipCommSetEventReturn(EVENT_TYPE_U8ARRAY,ACK_SIG);


				//the main sensor is a Serial (UART)
				SIP_SensorUartRead(me->super.config->sid);



			} else if(event->signal == ACK_SIG) 
			{
				SensorData pulse;
				SensorData sp02;

				int counter = 0;
				if (counter <64)
				{
					//we have received an ACK_SIG in this state which means that the sensor has finished sampling!
					//we grab the next 2 hex
					if (((ArrayEventPtr)event)->data.u8[counter] == STX) // Blood pressure
					{
						counter = bloodPressureEvent(counter, me, event);
					}

					else if(((ArrayEventPtr)event)->data.u8[counter] == 0xFA)   // pulse rate
					{
						counter = incrementCounter(counter);                    // counter++ with bounds check
						if (((ArrayEventPtr)event)->data.u8[counter] == STX)   // If blood pressure interrupts
						{
							counter = bloodPressureEvent(counter, me, event);   // get new counter (already incremented to new data) after uploading blood pressure event data
						}
						pulse.u = ((ArrayEventPtr)event)->data.u8[counter];
						sprintf(buffer,"Pulse Data, Port: %d Data: %d",me->super.config->sid.portId, pulse.u);
						bspPrint(buffer);
						me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), pulse , SENSOR_PRIM_DTYPE_UINT);

					}
					else if (((ArrayEventPtr)event)->data.u8[counter] == STX)  // Sp02
					{
						counter = incrementCounter(counter);                    // counter++ with bounds check
						if (((ArrayEventPtr)event)->data.u8[counter] == STX)   // If blood pressure interrupts
						{
							counter = bloodPressureEvent(counter, me, event);   // new counter value
						}
						sp02.u = ((ArrayEventPtr)event)->data.u8[counter];
						sprintf(buffer,"SpO2 Data, Port: %d Data: %d",me->super.config->sid.portId, sp02.u);
						bspPrint(buffer);
						me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), sp02, SENSOR_PRIM_DTYPE_UINT);
					}
					// Going to ignore waveform, not too useful 
					/*else if (((ArrayEventPtr)event)->data.u8[counter] == 0xF8)  // pulse WAVE - 
					{
					counter = incrementCounter(counter);                    // counter++ with bounds check
					if (((ArrayEventPtr)event)->data.u8[counter] == STX)   // If blood pressure interrupts
					{
					counter = bloodPressureEvent(counter, me, event);   // new counter value
					}
					me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super),((ArrayEventPtr)event)->data.u8[counter], SENSOR_PRIM_DTYPE_UINT);
					}*/
					else if ((((ArrayEventPtr)event)->data.u8[counter] == 0xFB) ||  // information
						(((ArrayEventPtr)event)->data.u8[counter] == 0xFC) ||   // quality
						(((ArrayEventPtr)event)->data.u8[counter] == 0xF4))     // gain
					{
						counter = incrementCounter(counter);
						// IGNORE THESE FOR NOW

					}
					else // junk data, or incomplete data
					{
						counter = incrementCounter(counter);

					}



				}
				else
				{
					UNEXPECTED_SIGNAL("test_MAIN_SENSOR.c:handleSampleFSM", "MAIN_SENSOR_SAMPLE",event->signal);

				}


			}


			else {
				UNEXPECTED_SIGNAL("test_MAIN_SENSOR.c:handleSampleFSM", "MAIN_SENSOR_SAMPLE",event->signal);
			}
		}
		break;

	case  SAMPLE_ACK:
		if(event->signal == ACK_SIG)
		{
			//ackCount holds the number of analytics that data has been sent to. we receive
			//one ACK_SIG when one analytic has received the data, and we need to wait for all of
			//the analytics to respond, so we loop in this state until all analytics have ACK'd
			me->super.ackCount--;
			if (me->super.ackCount == 0){
				LOGGER(LOG_INFO, SUBSYSTEM_ID_SENSOR_AO, 0, "mainSensor finished sampling");
				//this function returns control back to the parent sensor.c; call this when the sampling process has completed
				sensorStateTransition(&(me->super), sensorHandleDefaultState, SENSOR_STATE_SAMPLE_MAIN_RETURN, STATE_TRAN_SIG);
			}
		}else {
			UNEXPECTED_SIGNAL("test_MAIN_SENSOR.c:handleSampleFSM", "SAMPLE_ACK",event->signal);
		}
		break;

	default:
		break;
	}

}

int bloodPressureEvent(int counter, mainSensorActiveObjectPtr me, EventPtr event)   // C can't pass by ref so I am just returning counter
{
	SensorData bloodStatus;     // 'S'
	SensorData bloodCycle;      // 'C'
	SensorData bloodMessage;    // 'M'
	SensorData bloodPressureSystole;   // 'P'
	SensorData bloodPressureDiastole;  // 'P'
	SensorData bloodPressureMean;      // 'P' 
	SensorData bloodHeartRate;   // 'R'
	SensorData bloodNextEvent;   // 'T'

	counter = incrementCounter(counter);


	/*unsigned char bloodArray[64];
	for (int a = 0; a< 64; a++) // fill all values as X, so we know where to stop 
	{
	bloodArray[a] = 'X';   
	}

	int i = 0;
	while ((((ArrayEventPtr)event)->data.u8[counter] != ETX) && (counter < 64))    // Looks for blood pressure end of text (ETX) or end of data at 64
	{                                                                               // counter left on the ETX 

	bloodArray[i] = ((ArrayEventPtr)event)->data.u8[counter+1];
	counter = incrementCounter(counter);

	}
	//for (int b = 0; b < 64; b++) // upload all values from bloodArray to server, except X
	int b = 0;*/
	while (((ArrayEventPtr)event)->data.u8[counter] != ETX )
	{

		if (((ArrayEventPtr)event)->data.u8[counter] == 'S') // Status
		{
			counter = incrementCounter(counter);
			bloodStatus.u = ((ArrayEventPtr)event)->data.u8[counter];
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodStatus, SENSOR_PRIM_DTYPE_UINT);
			counter = incrementCounter(counter);
		}

		else if (((ArrayEventPtr)event)->data.u8[counter] == 'C') // Cycle time in minutes
		{
			counter = incrementCounter(counter);
			bloodCycle.u = ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);                  // if we run out of data, it will never continue and switch states
			bloodCycle.u = (10*bloodCycle.u)+ ((ArrayEventPtr)event)->data.u8[counter]; // makes it so if we get 1 and 3, it will show 13 
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodCycle, SENSOR_PRIM_DTYPE_UINT);
			counter = incrementCounter(counter);

		}
		else if (((ArrayEventPtr)event)->data.u8[counter] == 'M') // Message
		{
			counter = incrementCounter(counter);
			bloodMessage.u = ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodMessage.u = (10*bloodMessage.u) + ((ArrayEventPtr)event)->data.u8[counter];
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodCycle, SENSOR_PRIM_DTYPE_UINT);
			counter = incrementCounter(counter);
		}
		else if (((ArrayEventPtr)event)->data.u8[counter] == 'P') // blood pressure data
		{
			counter = incrementCounter(counter);
			bloodPressureSystole.u = ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodPressureSystole.u = (10*bloodPressureSystole.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodPressureSystole.u = (10*bloodPressureSystole.u)+ ((ArrayEventPtr)event)->data.u8[counter]; //send a 3 digit number
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodPressureSystole, SENSOR_PRIM_DTYPE_UINT);

			counter = incrementCounter(counter);
			bloodPressureDiastole.u = ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodPressureDiastole.u = (10*bloodPressureDiastole.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodPressureDiastole.u = (10*bloodPressureDiastole.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodPressureDiastole, SENSOR_PRIM_DTYPE_UINT);

			counter = incrementCounter(counter);
			bloodPressureMean.u = ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodPressureMean.u = (10*bloodPressureMean.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodPressureMean.u = (10*bloodPressureMean.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodPressureMean, SENSOR_PRIM_DTYPE_UINT);
			counter = incrementCounter(counter);

		}
		else if (((ArrayEventPtr)event)->data.u8[counter] == 'R') // heart rate pressure
		{
			counter = incrementCounter(counter);
			bloodHeartRate.u = ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodHeartRate.u = (10*bloodHeartRate.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodHeartRate.u = (10*bloodHeartRate.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodHeartRate, SENSOR_PRIM_DTYPE_UINT);
			counter = incrementCounter(counter);

		}
		else if (((ArrayEventPtr)event)->data.u8[counter] == 'T') // how long in seconds will the next measure occur
		{
			counter = incrementCounter(counter);
			bloodNextEvent.u = ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodNextEvent.u = (10*bloodNextEvent.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodNextEvent.u = (10*bloodNextEvent.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			counter = incrementCounter(counter);
			bloodNextEvent.u = (10*bloodNextEvent.u)+ ((ArrayEventPtr)event)->data.u8[counter];
			me->super.ackCount = McpSendSensorDataToAnalytics (&(me->super.ao_super), bloodNextEvent, SENSOR_PRIM_DTYPE_UINT);
			counter = incrementCounter(counter);
		}
		else // junk data or data we are not interested in
		{
			incrementCounter(counter); 
		}

	}
	return incrementCounter(counter);
}




int incrementCounter(int counter)   // checks if counter is out of bounds, is so change states to get more data, if not, increment counter for next data slot in array
{                                   // would be nice if C has operator overloading like C++  
	if (counter < 64)
	{
		counter++;   
	}
	else // change states, we need more data:: Issue, may cause lost of blood pressure data
	{
		UNEXPECTED_SIGNAL("test_MAIN_SENSOR.c:handleSampleFSM", "SAMPLE_ACK",event->signal);  
		bspPrint("unexpected signal");
	}
	return counter;

}







/* [] END OF FILE */
