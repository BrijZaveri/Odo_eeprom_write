TaskHandle_t Task1;
TaskHandle_t Task2;
#include <esp_task_wdt.h>
#include <CAN.h>
#include <EEPROM.h>
#include "esp_system.h"

const int EEPROM_ADDRESS_O0 = 0;
const int EEPROM_ADDRESS_O1 = 1;
const int EEPROM_ADDRESS_O2 = 2;

unsigned long lastEepromStoreTime = 0;

const int ODOMETER_ARRAY_SIZE = 10;
int odometer[ODOMETER_ARRAY_SIZE];
int odometerIndex = 0;
int currentOdometerValue = 0;

// Define CAN message IDs
const int ODOMETER_MSG_ID = 522207235;  // ID for odometer message
const int ODOMETER_TX_ID = 0x1F200340;  // ID for transmitting odometer value

//////////////////////////////////////////__ODO__////////////////////////
uint32_t odo1;                 //to store combined hex values into DEC values which is coming from cluster
uint32_t odo;                 //to remove right most digit from the values stored in odo1.
byte o0 = 0, o1 = 0, o2 = 0;
byte q[3]; // array to store 3 bytes
byte Stored;  
byte Counter = 0;

#define EEPROM_SIZE 3
#include <CAN.h>
#include <EEPROM.h>
long int startTime ;
long int currentTime ;
long int elapsedTime ;

void setup() {
  Serial.begin(9600);
  EEPROM.begin(EEPROM_SIZE);
  //  EEPROMReadSetup();
  if (!CAN.begin(250E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  xTaskCreatePinnedToCore(
    Task1code,   
    "Task1",     
    10000,       
    NULL,        
    1,           
    &Task1,      
    0);          
  delay(500);

  xTaskCreatePinnedToCore(
    Task2code,   
    "Task2",     
    10000,       
    NULL,      
    1,           
    &Task2,      
    1);          
  delay(500);
}

void Task1code( void * pvParameters )
{
  for (;;) {
    // Reset thewatchdog timer
    esp_task_wdt_reset();
    eeprom();                                
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
}

void Task2code( void * pvParameters ) {
  for (;;)
  {
    CANReceiver();                     //to receive odometer values from cluster
    Set();                            //to write eeprom values
  }
}

void CANReceiver() {
  int packetSize = CAN.parsePacket();

  if (packetSize) {
    if (CAN.packetExtended()) {
      if (CAN.packetId() == 522207235) {  
        byte Counter = 0;

        while (CAN.available()) {
          byte b = CAN.read();
          Counter++;
          if (Counter == 1) {
            o0 = b;                                   //o0 is Odometer high byte
//            Serial.print("O0 = ");
//            Serial.println(o0,HEX);
          }
          else if (Counter == 2) {
            o1 = b;                                   //o1 is odometer middle byte
//            Serial.print("o1 = ");
//            Serial.println(o1,HEX);
          }
          else if (Counter == 3) {
            o2 = b;                                   //o2 is odometer low byte
//            Serial.print("o2 = ");
//            Serial.println(o2,HEX);

            odo1 = (o2 & 0xFF) | ((o1 & 0xFF) << 8) | ((o0 & 0xFF) << 16);         
            odo = ((odo1 - odo1 % 10) / 10);                                       
            Serial.print("ODO = ");
            Serial.println(odo);
               // Check if the odometer value has reached 100000
//            if (odo >= 100000) {
//              odo = 0;  // Reset odometer to zero
//              // Perform any additional actions here if needed
//            }
            break; 
          }
        }
      }
    }
  }
}

void Set()
{
//   static unsigned long prevTime = 0;
//  static unsigned long prevOdo = 0;
//  static unsigned long prevTime = 0;   
  int storedOdometerValue = EEPROM.read(EEPROM_ADDRESS_O2)| (EEPROM.read(EEPROM_ADDRESS_O1) << 8) | (EEPROM.read(EEPROM_ADDRESS_O0) << 16);
//  Serial.print("storedOdometerValue = ");
//  Serial.println(storedOdometerValue);
  if (odo != storedOdometerValue)
  {
    q[0] = (odo >> 16) & 0xFF;
    q[1] = (odo >> 8) & 0xFF;
    q[2] = odo & 0xFF; 

    // print the values in hexadecimal format
//    Serial.print("Storing values: ");
//    Serial.print(q[0], HEX);
//    Serial.print(", ");
//    Serial.print(q[1], HEX);
//    Serial.print(", ");
//    Serial.println(q[2], HEX);

    EEPROM.write(EEPROM_ADDRESS_O0, q[0]);
    EEPROM.write(EEPROM_ADDRESS_O1, q[1]);
    EEPROM.write(EEPROM_ADDRESS_O2, q[2]);
    EEPROM.commit(); // Save the changes to EEPROM

    Serial.println("Values stored in EEPROM................");
//    prevTime = millis();
  }
}



void eeprom() {                                               //write directly to CAN
  q[0] = EEPROM.read(EEPROM_ADDRESS_O0);
  q[1] = EEPROM.read(EEPROM_ADDRESS_O1);
  q[2] = EEPROM.read(EEPROM_ADDRESS_O2);
  Serial.print("EEPROM Values: ");
  Serial.print(q[0],HEX);
  Serial.print(" ");
  Serial.print(q[1],HEX);
  Serial.print(" ");
  Serial.println(q[2],HEX);
  
  if (odo == 0 && q[2] > 0) {
//    if (odo > 99999) {
//      esp_restart();
//      {
        CAN.beginExtendedPacket(ODOMETER_TX_ID, 8);
        CAN.write(q[0]);   
        CAN.write(q[1]);    
        CAN.write(q[2]);  
        CAN.write(0x00);                         
        CAN.write(0x00);
        CAN.write(0x00);
        CAN.write(0x00);
        CAN.write(0x00);
        CAN.endPacket();
        Serial.println("DONE...................");
//      }
    }
  }
//}
void loop()
{
}
