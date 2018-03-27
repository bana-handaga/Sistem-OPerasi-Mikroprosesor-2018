/*
#ifdef configUSE_APPLICATION_TASK_TAG
  #undef configUSE_APPLICATION_TASK_TAG
#endif
#define configUSE_APPLICATION_TASK_TAG 1

#include "FreeRTOS_AVR.h"
#include "basic_io_avr.h"

#define traceTASK_SWITCHED_IN() vSetDigitalOutput( (int)pxCurrentTCB->pxTaskTag)
*/

#include <Arduino_FreeRTOS.h>
#include <semphr.h>  // add the FreeRTOS functions for Semaphores (or Flags).

int luminositySensorPin = A0;
int temperatureSensorPin = A1;
int opticalSwicthSensorPin = 2;

int luminosityTaskON = 12;
int temperatureTaskON = 11;
int serialInfoTaskON = 10;
int opticalSwicthTaskON = 9;
int serialMsgTaskON = 8;
int idleTaskON = 13;

unsigned int luminosity = 0;
unsigned int temperature = 0;
unsigned int Counter = 0;
int opticalSwitch;

int MYDELAY = 8;

SemaphoreHandle_t xSemaphoreLuminosity;
SemaphoreHandle_t xSemaphoreTemperature;
SemaphoreHandle_t xSemaphoreOS;
SemaphoreHandle_t xSemaphoreSerial;



//------------------
//Reading luminositysensor value
static void vLuminosityTask(void *pvParameters) {
  //vTaskSetApplicationTaskTag(NULL, (TaskHookFunction_t)1);
  //Create mutex
  while (1) {
    if (xSemaphoreTake(xSemaphoreLuminosity, ( TickType_t ) 5) == pdTRUE)
    {
      luminosity = analogRead(luminositySensorPin);
      xSemaphoreGive(xSemaphoreLuminosity);
      /*
      if (xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 2 ) == pdTRUE ) {
        Serial.println("TASK-A : ----------------------------");        
      }
      */         
    }
    vTaskDelay(MYDELAY); //MYDELAY x15ms
  }
}

//---------------
//Reading void vTemperatureTask(void*pvParameter)
static void vTemperatureTask(void *pvParameters) {
  // vTaskSetApplicationTaskTag(NULL, (TaskHookFunction_t) 2);
  while (1) {
    if (xSemaphoreTake(xSemaphoreTemperature, ( TickType_t ) 5) == pdTRUE )
    {
      temperature = analogRead(temperatureSensorPin);
      xSemaphoreGive(xSemaphoreTemperature);
      /*
      if (xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 2 ) == pdTRUE ) {
        Serial.println("TASK-B : ----------------------------");        
      } 
      */     
    }
    vTaskDelay(MYDELAY); //MYDELAYx15ms
  }
}
//---------------
//Reading optocaptor sensor value
static void vOpticalSwitchTask(void *pvParameters) {
  // vTaskSetApplicationTaskTag(NULL, (TaskHookFunction_t)3);
  unsigned int temp;
  while (1) {

    if (xSemaphoreTake(xSemaphoreOS, ( TickType_t ) 5) == pdTRUE )
    {
        opticalSwitch = digitalRead(opticalSwicthSensorPin);
        xSemaphoreGive(xSemaphoreOS);    
    }
    
    /*
    if (xSemaphoreTake( xSerialSemaphore, ( TickType_t ) 2 ) == pdTRUE ) {
       Serial.println("TASK-C : ----------------------------");
    } 
    */            
    vTaskDelay(MYDELAY);  //MYDELAYx15ms
  }
}
//---------------
//Send informations via Serial Interface
static void vSerialInfoTask(void *pvParameter) {
  //vTaskSetApplicationTaskTag(NULL,(TaskHookFunction_t)4);
  while (1) {
    if  ( 
          (xSemaphoreTake(xSemaphoreLuminosity, ( TickType_t ) 3) == pdTRUE ) &&
          (xSemaphoreTake(xSemaphoreTemperature,( TickType_t ) 3) == pdTRUE ) &&
          (xSemaphoreTake(xSemaphoreOS,( TickType_t ) 3) == pdTRUE ) &&
          (xSemaphoreTake(xSemaphoreSerial,( TickType_t ) 3) == pdTRUE )
    )
    {
      Serial.print("Luminosity:");
      Serial.println(luminosity);      
      Serial.print("Temperature:");
      Serial.println(temperature);
      Serial.print("Optical swicth: ");
      Serial.println(opticalSwitch);
      xSemaphoreGive(xSemaphoreTemperature);
      xSemaphoreGive(xSemaphoreOS);
      xSemaphoreGive(xSemaphoreLuminosity);
      xSemaphoreGive(xSemaphoreSerial);
    }
    vTaskDelay(120);  //62 x 15 ms waiting
  }
}
//-----------------
static void vSerialMsgCheck(void *pvParameters) {
  //vTaskSetApplicationTaskTag(NULL,(TaskHookFunction_t)5);
  int x;
  for (;;) {    
    if (xSemaphoreTake(xSemaphoreSerial,( TickType_t ) 3) == pdTRUE )
    { 
      if (Serial.available() > 0) {
        int receivedByte = Serial.read();
        if ((receivedByte == 's')||(receivedByte == 'S')) {
           for (x=0;x<10;x++) {   
              Serial.print("wait for ");
              Serial.print(x);
              Serial.println(" of 10");
              //vTaskDelay(200);  //Force waiting 3 second
           }
        }
      }
      xSemaphoreGive(xSemaphoreSerial);             
    }
    vTaskDelay(4);  //4x15ms
  }
}

void vIdleTask(void *pvParameters) {
  // vTaskSetApplicationTaskTag(NULL, (TaskHookFunction_t) 6);
  //int X=0;
  while (1) 
        {
          //Serial.println("IDLE");
          //Serial.println(++Counter);
          //vSetDigitalOutput(6);
        }
}
//------------
void setup() {
  
  pinMode(luminosityTaskON, OUTPUT);
  pinMode(temperatureTaskON, OUTPUT);
  pinMode(serialInfoTaskON, OUTPUT);
  pinMode(opticalSwicthTaskON, OUTPUT);
  pinMode(serialMsgTaskON, OUTPUT);
  pinMode(idleTaskON, OUTPUT);

  /*
   //Configurating HIGHSPEED
   bitClear(ADCSRA, ADPS0);
   bitClear(ADCSRA, ADPS1);
   bitSet(ADCSRA,ADPS2);
   analogReference(EXTERNAL);
   */


  Serial.begin(9600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB, on LEONARDO, MICRO, YUN, and other 32u4 based boards.
  }
/*
  // Semaphores are useful to stop a Task proceeding, where it should be paused to wait,
  // because it is sharing a resource, such as the Serial port.
  // Semaphores should only be used whilst the scheduler is running, but we can set it up here.
  if ( xSerialSemaphore == NULL )  // Check to confirm that the Serial Semaphore has not already been created.
  {
    xSerialSemaphore = xSemaphoreCreateMutex();  // Create a mutex semaphore we will use to manage the Serial Port
    if ( ( xSerialSemaphore ) != NULL )
      xSemaphoreGive( ( xSerialSemaphore ) );  // Make the Serial Port available for use, by "Giving" the Semaphore.
  }
*/
   // Create a mutex semaphore we will use to manage the Luminosity
  if ( xSemaphoreLuminosity == NULL ) 
  {
    xSemaphoreLuminosity = xSemaphoreCreateMutex();  
    if ( ( xSemaphoreLuminosity ) != NULL )
      xSemaphoreGive( ( xSemaphoreLuminosity ) );  
  }
   // Create a mutex semaphore we will use to manage the Temperature
  if ( xSemaphoreTemperature == NULL ) 
  {
    xSemaphoreTemperature = xSemaphoreCreateMutex();  
    if ( ( xSemaphoreTemperature ) != NULL )
      xSemaphoreGive( ( xSemaphoreTemperature ) );  
  }

  if ( xSemaphoreOS == NULL ) 
  {
    xSemaphoreOS = xSemaphoreCreateMutex();  
    if ( ( xSemaphoreOS ) != NULL )
      xSemaphoreGive( ( xSemaphoreOS ) );  
  }

  if ( xSemaphoreSerial == NULL ) 
  {
    xSemaphoreSerial = xSemaphoreCreateMutex();  
    if ( ( xSemaphoreSerial ) != NULL )
      xSemaphoreGive( ( xSemaphoreSerial ) );  
  }

  Serial.print("Create MUTEX\r\n");
  delay(1000);

  
  // if all semaphore was created successfull
  if ((xSemaphoreLuminosity != NULL) && (xSemaphoreTemperature != NULL) && (xSemaphoreOS != NULL) && (xSemaphoreSerial != NULL)) {
    Serial.print("Create TASK\r\n");
    delay(100);
    xTaskCreate(vLuminosityTask, "luminosityTask", configMINIMAL_STACK_SIZE + 12, NULL, 3, NULL);
    xTaskCreate(vTemperatureTask, "temperatureTask", configMINIMAL_STACK_SIZE + 12, NULL, 3, NULL);
    xTaskCreate(vOpticalSwitchTask, "opticalSwitchTask", configMINIMAL_STACK_SIZE + 12, NULL, 2, NULL);
    xTaskCreate(vSerialMsgCheck, "serialMsgCheckTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vSerialInfoTask, "serialTask", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
    xTaskCreate(vIdleTask, "idleTask", configMINIMAL_STACK_SIZE, NULL, 0, NULL);

    //Serial.print("Start SCHEDULER\r\n");
    //delay(100);
/*
    //start FreeRTOS
    vTaskStartScheduler();
*/
    //version 10.** > Scheduler will be executed automatically by 
    //system after setup finished.
  }

/*
  Serial.println("Die");
  while (1)
    ;
*/
}


void vSetDigitalOutput(int task) {
  digitalWrite(temperatureTaskON, LOW);
  digitalWrite(serialInfoTaskON, LOW);
  digitalWrite(opticalSwicthTaskON, LOW);
  digitalWrite(serialMsgTaskON, LOW);
  digitalWrite(idleTaskON, LOW);
  digitalWrite(luminosityTaskON, LOW);
  switch (task) {
  case 1:
    digitalWrite(luminosityTaskON, HIGH);
    break;
  case 2:
    digitalWrite(temperatureTaskON, HIGH);
    break;
  case 3:
    digitalWrite(serialInfoTaskON, HIGH);
    break;
  case 4:
    digitalWrite(opticalSwicthTaskON, HIGH);
    break;
  case 5:
    digitalWrite(serialMsgTaskON, HIGH);
    break;
  case 6:
    digitalWrite(idleTaskON, HIGH);
    break;
  default:
    break;
  }
}

//#define traceTASK_SWITCHED_IN() vSetDigitalOutput((int) pxCurrentTCB->pxTaskTag);
void loop() {
  //Not use
}

