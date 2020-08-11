//////////////////////////////////////////////////////////
//
//
//      Methane Sensor Code
//      
//      Code is written for Arduino Due to connect with Amphenol SGX Sensortech INIR-ME100%


/// Libraries
#include <string.h>


/// Definitions
#define _3V3          3.3
#define ADC_MAX       4095
#define R1            4700
#define R2            470
#define VIN_NUM_AVG   100


/// Function Declarations
void readComputer(char* );  // reads from Serial until a '/r/n' is reached. Result is placed in char*


/// Global Variable Initializations
char com_serial_input[1000] = "";   // String to hold incoming data from the computer
char ch4_data[1000] = "";           // String to hold incoming data from the methane sensor
boolean ch4_data_received = false;  // Flag that identifies when data has been received by INIR-ME sensor
boolean ch4_automated = false;      // Flag that indicates am acknowledgement has been received from INIR-ME

boolean first_time = true;    // Flag that tells when the Loop() functions runs the first time
                              // Creates a secondary "Loop()" function within Loop() that runs at the beginning of the program
int run_index = 0;            // Tells us where we are in the mini-loop that run during the first Loop()

int indexME = 0;          // Keep track of what line we are processing in INIR-ME data chain
int ppmME = 0;            // Contains the Methane ppm
float tempME = 0;         // Contains the Methane sensor internal temperature
char faultsME[10] = "";   // Stores the error message sent from INIR-ME
float voltage;            // Holds the voltage value of the power supply 

unsigned long previous_time = 0;  // Stores the last instance that the time was checked
unsigned long interval = 3000;    // The Arduino will only check for a message every three seconds


void serialEvent1() 
///
//  ISR Function is automatically called when hardware serial port_1 receives a char
//  Hardware serial port_1 is connected to the INIR-ME sensor
{
    char temp[10] = "";   // Temporary buffer used to store unneeded data
    
    indexME++;
    Serial1.readBytesUntil('\r', ch4_data, 8);  // Capture the first 8 bytes of data which are useful
    Serial1.readBytes(temp,2);                  // Discard \r\n which are appended to data
    
    if (first_time)Serial.println(ch4_data);    // First data ever received from INIR is in a different format

    /// IF sequence to determine course of action based on INIR-ME message
    // INIR Acknowledge message, used for debugging only
    if (strcmp(ch4_data, "5b414b5d") == 0) {
      //Serial.println("INIR-ME: [AK]");        
    }
    // INIR Not Acknowledge message
    else if (strcmp(ch4_data, "5b4e415d") == 0) {
      Serial.println("INIR-ME: [NA]");          
    }
    // Indicates the beginning of an INIR Message
    else if (strcmp(ch4_data, "0000005b") == 0) {
      indexME = 0;                              
    }
    // Indicates the end of an INIR Message
    else if (strcmp(ch4_data, "0000005d") == 0) {
      ch4_data_received = true; // sets the flag used to say that a completed string has been received  
    }
    // First piece of info in INIR data chain, CH4 ppm
    else if (indexME == 1){
      ppmME = strtoul(ch4_data, NULL, 16);
    }
    // Second piece of info in INIR data chain, error code
    else if (indexME == 2){
      memcpy(faultsME, ch4_data,8);
      if (ch4_data[1] != 'a') {
        ppmME = -1; // ppm data is not valid
      }
    }
    // Third piece of information in INIR data chain, internal temp
    else if (indexME == 3){
      tempME = (float) strtoul(ch4_data, NULL, 16)/10 - 273.1;
    }
    // default action is to do nothing
    else {
    }
}

void serialEvent()
///
// Function automatically called when hardware serial port_0 receives a char
// This code is for debugging purposes only, and is not supposed to run during proper operation
{
  // readComputer(com_serial_input);     // Read in data from the computer  
  // Serial1.print(com_serial_input);    // Send data to INIR-ME Sensor
  // Serial.println("Send data to INIR ME");
}

void setup() {
  /// put your setup code here, to run once:
  Serial.begin(112500);               // Communication with the computer
  Serial1.begin(9600, SERIAL_8N2);    // Communication with the INIR-ME Sensor

  analogReadResolution(12);           // Set analog resolution to 12 bits
  
  Serial.println("CH4 Recording Code Version 1.2");
  Serial.println("tempME,ppmME,faultsME,VinADC,Vin");  // Shows the order of data printed to copmuter
  
} 

void loop() {
  /// put your main code here, to run repeatedly:
  
  char string[1000] = "";   // string array used to print output to Serial
  char temp[1000] = "";     // string array used for temporary manipulations

  unsigned long current_time = millis();  // Used to keep track of time

  // Data collection is only checked when the interval (set at 3000 ms) has passed
  // Each loop run only collects or sends one piece of data
  if (current_time - previous_time > interval)
  {
    previous_time = current_time;

    /// On bootup, the Arduino must setup the INIR-ME
    if (first_time) {
      switch (run_index)
      {
        case 0:
        // Enter Command Mode
          Serial1.print("[C]");
          Serial.println("Command [C] Sent");
          break;
        case 1:
        // Collect System information
          Serial1.print("[I]");
          Serial.println("Command [I] Sent");
          break;
        case 2:
        // Put INIR-ME into On-Demand Mode
          Serial1.print("[H]");
          Serial.println("Command [H] Sent");
          break;
        case 3:
        // Turn on Humidity Compensation Algorithm
          Serial1.print("[L]");
          Serial.println("Command [L] Sent");
          break;
        case 4:
        // Collect Data, begins data collection
          Serial1.print("[Q]");
          first_time = false;
          break;
        default: break;
      }
      run_index++;
    }
    else {
      // Check Data received flag. True indicates a data package is ready
      if (ch4_data_received == true) {    

        /// Collect power supply data to ensure steady power supply
        int VinADC = 0;
        int i = 0;
        
        while (i < VIN_NUM_AVG) {             //Average Vin over 100 values (see Definitions section)
          VinADC = VinADC + analogRead(A0);
          i++;
        }
        float Vin = VinADC / VIN_NUM_AVG * _3V3 / ADC_MAX * (R2 + R1) / R2;

        /// Send data to computer and reset variables
        sprintf(string, "%f,%d,%s,%d,%f", tempME, ppmME, faultsME, VinADC, Vin);
        Serial.println(string);
        ch4_data_received = false;          
        memset(com_serial_input, 0, 1000);

        /// Collect another point of data
        Serial1.print("[Q]");
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
    else if (Serial.available() > 0) { // check to make sure that the '/n' character is still in the buffer
      Serial.read();                   // dispose of '/n' character so that it won't be read next cycle
    }
    i++;  
  } 
}
