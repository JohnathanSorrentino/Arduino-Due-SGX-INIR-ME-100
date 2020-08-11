//////////////////////////////////////////////////////////
//
///
//      Methane Command Code

/// Libraries
#include <string.h>

/// Definitions
#define _3V3          3.3
#define ADC_MAX       4095
#define R1            4700
#define R2            470
#define VIN_NUM_AVG   100

void readComputer(char* ); // reads from Serial until a '/r/n' is reached. Result is placed in char*
void readMethane(char * ); // reads from Serial until a '/r' is reached. Result is placed in char*

char com_serial_input[1000] = "";   // String to hold incoming data from the computer
char ch4_data[1000] = "";           // String to hold incoming data from the methane sensor
boolean ch4_data_received = false;  // Flag that identifies when data has been received by INIR-ME sensor
boolean ch4_automated = false;      // Flag that indicates am acknowledgement has been received from INIR-ME

boolean first_time = true;
int run_index = 0;

int indexME = 0;
int ppmME = 0;
float tempME = 0;
char faultsME[10] = "";
float voltage;                      //holds the voltage value of the ADC reading

unsigned long previous_time = 0;
unsigned long interval = 3000;


void serialEvent1() 
///
//  Function is automatically called when hardware serial port_1 receives a char
{
    char temp[10] = "";
    
    indexME++;
    Serial1.readBytesUntil('\r', ch4_data, 8);  //
    Serial1.readBytes(temp,2);                  //
    Serial.println(ch4_data);
    
    if (strcmp(ch4_data, "5b414b5d") == 0) {
      Serial.println("");
    }
    else if (strcmp(ch4_data, "5b4e415d") == 0) {
      Serial.println("");
    }
    else if (strcmp(ch4_data, "0000005d") == 0) {
      Serial.println(""); 
    }
    
/*
    else if (strcmp(ch4_data, "0000005b") == 0) {
      indexME = 0;
    }

    else if (indexME == 1){
      ppmME = strtoul(ch4_data, NULL, 16);
    }
    else if (indexME == 2){
      memcpy(faultsME, ch4_data,8);
      if (ch4_data[1] != 'a') {
        ppmME = -1;
      }
    }
    else if (indexME == 3){
      tempME = (float) strtoul(ch4_data, NULL, 16)/10 - 273.1;
    }
    else {
    }*/
}

void serialEvent()
///
// Function automatically called when hardware serial port_0 receives a char
{
  readComputer(com_serial_input);     // Read in data from the computer  
  Serial1.print(com_serial_input);  // Send data to INIR-ME Sensor
  Serial.print("Sent to INIR ME: ");
  Serial.println(com_serial_input);
}

void setup() {
  /// put your setup code here, to run once:
  Serial.begin(112500);   // communication with the computer
  Serial1.begin(9600, SERIAL_8N2);    // Serial communication to the INIR-ME Sensor

  analogReadResolution(12);                           //set analog resolution to 12 bits
  
  Serial.println("CH4 Command Code Version 3.3");
  Serial.println("tempME,ppmME,faultsME,VinADC,Vin");  
  
} 

void loop() {
  // put your main code here, to run repeatedly:
  char string[1000] = ""; // string array used to print output to Serial
  char temp[1000] = "";

  unsigned long current_time = millis();  

  if (current_time - previous_time > interval)
  {
    previous_time = current_time;
    
    if (first_time) {
      switch (run_index)
      {
        case 0:
          Serial1.print("[C]");
          Serial.println("Command [C] Sent");
          break;
        case 1:
          Serial1.print("[I]");
          Serial.println("Command [I] Sent");
          break;
        case 2:
          Serial1.print("[B]");
          Serial.println("Command [B] Sent");
          break;
        case 3:
          Serial1.print("[M]");
          Serial.println("Command [M] Sent");
          break;
        case 4:
          Serial1.print("[Q]");
          first_time = false;
          break;
        default: break;
      }
      run_index++;
    }
    else {
      if (ch4_data_received == true) {    // check string received flag 
      /// Print all recevied data to Serial

        int VinADC = 0;
        int i = 0;
        //Average Vin over 100 values
        while (i < VIN_NUM_AVG) {
          VinADC = VinADC + analogRead(A0);
          i++;
        }

        float Vin = VinADC / VIN_NUM_AVG * _3V3 / ADC_MAX * (R2 + R1) / R2;
      
        sprintf(string, "%f,%d,%s,%d,%f", tempME, ppmME, faultsME, VinADC, Vin);
        Serial.println(string);
        ch4_data_received = false;    // reset string received flag
        memset(com_serial_input, 0, 1000);
        //Serial1.print("[Q]");
      }
    }
  }
}

void readComputer(char* stringArray)
///
// Reads from Serial until a '/r/n' is reached. Result is place in char* string Array
{
  int i = 0;    // counter variable
  char rxByte;  // temporary byte to read data 
  while (Serial.available() > 0)
  {
    rxByte = Serial.read();
    if (rxByte != '\r') {
      // as long as byte is not '/r' record data to passed in array
      stringArray[i] = rxByte;
    }
    else if (Serial.available() > 0) { // check to make sure that the '/n' character is next
      Serial.read();                   // dispose of '/n' character so that it won't be read next cycle
    }
    i++;  // increment counter
  } 
}
