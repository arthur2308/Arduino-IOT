#include <Event.h>

/*************************************************** 
  This is an example for the Adafruit CC3000 Wifi Breakout & Shield

  Designed specifically to work with the Adafruit WiFi products:
  ----> https://www.adafruit.com/products/1469

  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried & Kevin Townsend for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
 
 /*
This example does a test of the TCP client capability:
  * Initialization
  * Optional: SSID scan
  * AP connection
  * DHCP printout
  * DNS lookup
  * Optional: Ping
  * Connect to website and print out webpage contents
  * Disconnect
SmartConfig is still beta and kind of works but is not fully vetted!
It might not work on all networks!
*/

#include <Adafruit_Sensor.h>
#include <Adafruit_CC3000.h>
#include <ccspi.h>
#include <SPI.h>
#include <string.h>
#include <Timer.h>
#include "utility/debug.h"

// These are the interrupt and control pins
#define ADAFRUIT_CC3000_IRQ   3  // MUST be an interrupt pin!
// These can be any two pins
#define ADAFRUIT_CC3000_VBAT  5
#define ADAFRUIT_CC3000_CS    10
// Use hardware SPI for the remaining pins
// On an UNO, SCK = 13, MISO = 12, and MOSI = 11
Adafruit_CC3000 cc3000 = Adafruit_CC3000(ADAFRUIT_CC3000_CS, ADAFRUIT_CC3000_IRQ, ADAFRUIT_CC3000_VBAT,
                                         SPI_CLOCK_DIVIDER); // you can change this clock speed

#define WLAN_SSID       "Nikita"           // cannot be longer than 32 characters!
#define WLAN_PASS       "12345123"
// Security can be WLAN_SEC_UNSEC, WLAN_SEC_WEP, WLAN_SEC_WPA or WLAN_SEC_WPA2
#define WLAN_SECURITY   WLAN_SEC_WPA2

#define IDLE_TIMEOUT_MS  3000      // Amount of time to wait (in milliseconds) with no data 
                                   // received before closing the connection.  If you know the server
                                   // you're accessing is quick to respond, you can reduce this value.

// What page to grab!
#define WEBSITE      "www.adafruit.com"
#define WEBPAGE      "/testwifi/index.html"
Adafruit_CC3000_Client cc3000Client;


#include <Adafruit_LSM303_U.h>
#include <Adafruit_Sensor.h>
/* Assign a unique ID to these sensors */
Adafruit_LSM303_Accel_Unified accel = Adafruit_LSM303_Accel_Unified(54321);
Adafruit_LSM303_Mag_Unified mag = Adafruit_LSM303_Mag_Unified(123);

float AccelMinX, AccelMaxX;
float AccelMinY, AccelMaxY;
float AccelMinZ, AccelMaxZ;
 
float MagMinX, MagMaxX;
float MagMinY, MagMaxY;
float MagMinZ, MagMaxZ;

long lastDisplayTime;

/**************************************************************************/
/*!
    @brief  Sets up the HW and the CC3000 module (called automatically
            on startup)
*/
/**************************************************************************/

// alocate Timer
Timer AccelTimer;
Timer CompassTimer;
Timer TempTimer;

// id for timers
int AccelTimerEvent;
int CompassTimerEvent;
int TempTimerEvent;

// allocate delay for sensors
int tempDelay = 2000;
int compassDelay = 2000;
int  accelerDelay = 2000;

// allocate enable/disable for sensors
bool enableTemp = false;
bool enableCompass = false;
bool enableAcceler = false;

int  AccelMaxXthresh = 10;
int  AccelMaxYthresh =  10;
int  AccelMaxZthresh = 10;
 
int  MagMaxXthresh = 10;
int  MagMaxYthresh = 10;
int  MagMaxZthresh  = 10;

// tempeture values
int tempThreshold;
int tempPin = 0;
int tempReading;
float voltage;
float temperatureC = 20;
float temperatureF;

// check if auto-alarm mode is enabled
bool accelAutoAlarm = false; 
bool compAutoAlarm = false; 
bool tempAutoAlarm = false;

//ardino
bool CheckPinMode = false;

bool displayConnectionDetails(void)
{
  uint32_t ipAddress, netmask, gateway, dhcpserv, dnsserv;
  
  if(!cc3000.getIPAddress(&ipAddress, &netmask, &gateway, &dhcpserv, &dnsserv))
  {
    Serial.println(F("Unable to retrieve the IP Address!\r\n"));
    return false;
  }
  else
  {
    Serial.print(F("\nIP Addr: ")); cc3000.printIPdotsRev(ipAddress);
    Serial.print(F("\nNetmask: ")); cc3000.printIPdotsRev(netmask);
    Serial.print(F("\nGateway: ")); cc3000.printIPdotsRev(gateway);
    Serial.print(F("\nDHCPsrv: ")); cc3000.printIPdotsRev(dhcpserv);
    Serial.print(F("\nDNSserv: ")); cc3000.printIPdotsRev(dnsserv);
    Serial.println();
    return true;
  }
}

void setup(void)
{
  Serial.begin(115200);
  Serial.println(F("Hello, CC3000! Client\n")); 


     if(!mag.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no compus detected ... Check your wiring!");
    while(1);
  }
      if(!accel.begin())
  {
    /* There was a problem detecting the LSM303 ... check your connections */
    Serial.println("Ooops, no accel detected ... Check your wiring!");
    while(1);
  }


  Serial.print("Free RAM: "); Serial.println(getFreeRam(), DEC);
  
  /* Initialise the module */
  Serial.println(F("\nInitializing..."));
  if (!cc3000.begin())
  {
    Serial.println(F("Couldn't begin()! Check your wiring?"));
    while(1);
  }
  
  // Optional SSID scan
  // listSSIDResults();
  
  Serial.print(F("\nAttempting to connect to ")); Serial.println(WLAN_SSID);
  if (!cc3000.connectToAP(WLAN_SSID, WLAN_PASS, WLAN_SECURITY)) {
    Serial.println(F("Failed!"));
    while(1);
  }
   
  Serial.println(F("Connected!"));
  
  /* Wait for DHCP to complete */
  Serial.println(F("Request DHCP"));
  while (!cc3000.checkDHCP())
  {
    delay(100); // ToDo: Insert a DHCP timeout!
  }  

  /* Display the IP address DNS, Gateway, etc. */  
  while (! displayConnectionDetails()) {
    delay(1000);
  }

   
  uint32_t ip; 
  uint16_t port = 23;
  
  uint32_t s1 = 172;
  uint32_t s2 = 20;
  uint32_t s3 = 10;
  uint32_t s4 = 5;


  ip = (s1 << 24) | (s2 << 16) | (s3 << 8) | (s4);
  cc3000Client = cc3000.connectTCP(ip, port);

  if (cc3000Client.connected()) {
    Serial.println("Succuess!");
  } else {
    Serial.println(F("Connection failed"));    
    return;
  }

   // create timers 
   AccelTimerEvent = AccelTimer.every(accelerDelay,myacceleration);
   CompassTimerEvent = CompassTimer.every(compassDelay,myMagnetic);
   TempTimerEvent =  TempTimer.every(tempDelay,mytemp);
}

void sendAck() {
  cc3000Client.fastrprint("$");
  cc3000Client.flush();
}

void setToDefault() {
 
  tempDelay = 2000;
  compassDelay = 2000;
  accelerDelay = 2000;
 
  enableTemp = false;
  enableCompass = false;
  enableAcceler = false;

  accelAutoAlarm = false; 
  compAutoAlarm = false; 
  tempAutoAlarm = false;

  CheckPinMode = false;

  AccelMaxXthresh = 10;
  AccelMaxYthresh = 10;
  AccelMaxZthresh = 10;
 
  MagMaxXthresh = 10;
  MagMaxYthresh =  10;
  MagMaxZthresh  = 10;

  tempThreshold = 30;

  // reset timers
  AccelTimer.configTime(accelerDelay,AccelTimerEvent);
  CompassTimer.configTime(compassDelay,CompassTimerEvent);
  TempTimer.configTime(tempDelay,TempTimerEvent);
}
int count = 0;
void loop(void)
{  
  
    //---------------------------
    // check for periodical report
    // if it is enabled
    if(enableAcceler) {
      AccelTimer.update();
    } 

    if (enableCompass)
    {
       CompassTimer.update();
    }

    if (enableTemp)
    {
       TempTimer.update();
    }
    //---------------------------
     handleAutoAlarm();

  // send to server
  if (Serial.available()) {
    
    String temp = Serial.readString();
    cc3000Client.println(temp);
    cc3000Client.flush();
  }
   String temp ="";
   // get from server
   unsigned long lastRead = millis();
   while (cc3000Client.connected() && (millis() - lastRead < IDLE_TIMEOUT_MS)) {
      while (cc3000Client.available()) {
        char modeType = cc3000Client.read();
        temp+=modeType;
        lastRead = millis();
        cc3000Client.flush();
        } 

       if(temp != "") { 

       if( temp == ("ACK\n")) {
        Serial.println("ACK");
        // do nothing
       }
       else if(temp == ("reset")){           // == "reset") {
         ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
          cc3000Client.fastrprint("Set default setting\n");
          cc3000Client.flush();
          setToDefault();
        } 
        else {
         ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
        cc3000Client.flush();
        Serial.println(temp);
        Serial.flush();
          getReportType(temp);

            if (CheckPinMode)
             {
                handlePinMode(temp); 
             }
        }
        //sendAck();  
        temp = "";
      }  
         
    }
   
}

void getReportType(String temp) {

    switch (temp.charAt(1))
    {
      case 'p': 
      {
        cc3000Client.fastrprint("Periodical Mode Selected\n");
        cc3000Client.flush();
        enableTimerDivices(temp);
        break;
      }

      case 'o':
      {
        cc3000Client.fastrprint("On demand Mode Selected\n");
        cc3000Client.flush();
        getSensor(temp);


         break;
      }

      case 'a':
      {
        cc3000Client.fastrprint("Auto-Alarm Mode Selected\n");
        cc3000Client.flush(); 
        enableAutoAlarm(temp);
         break;
      }

       case 'c':
      {
        cc3000Client.fastrprint("Config Mode Selected\n");
        cc3000Client.flush();
        configMode(temp);
        
        break;
      }

      case 'm':
      {
          CheckPinMode = true; 
          cc3000Client.fastrprint("Pin Mode Selected\n");
          cc3000Client.flush();
          break;
      }

      case 's':
      {
          CheckPinMode = true; 
          cc3000Client.fastrprint("Pin Status Selected\n");
          cc3000Client.flush();
          break;
      }
      
      default:
      {
            cc3000Client.fastrprint("Invalid input: Incorret mode\n");
            cc3000Client.flush();
            break;
      }
    }
    Serial.flush();
}


// function for ardino
void handlePinMode(String temp)
{

    ++count;
    cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
    cc3000Client.flush();
    if (temp.charAt(2) == 'o')
    {
         pinMode(getValue(temp, 2), OUTPUT);
         cc3000Client.fastrprint("PIN: "); cc3000Client.print(getValue(temp,2));
         cc3000Client.fastrprint(" SET TO OUTPUT!\n");
         
        int  p = analogRead(getValue(temp, 2));  
         cc3000Client.fastrprint("The output: "); cc3000Client.println(p);
         cc3000Client.flush();
         sendAck();
    }

    if (temp.charAt(2) == 'i')
    {
        pinMode(getValue(temp, 2), INPUT);
        cc3000Client.fastrprint("PIN: "); cc3000Client.print(getValue(temp,2));
        cc3000Client.fastrprint(" SET TO INPUT!\n ");
        cc3000Client.flush();
        sendAck();
    }

    if (temp.charAt(2) == 'h')
    {
        digitalWrite(getValue(temp, 2), HIGH);
        cc3000Client.fastrprint("PIN: "); cc3000Client.print(getValue(temp,2));
        cc3000Client.fastrprint(" SET TO HIGH!\n ");
        cc3000Client.flush();
        sendAck();
    }

    if (temp.charAt(2) == 'l')
    {
        digitalWrite(getValue(temp, 2), LOW);
        cc3000Client.fastrprint("PIN: "); cc3000Client.print(getValue(temp,2));
        cc3000Client.fastrprint(" SET TO LOW!\n ");
        cc3000Client.flush();
        sendAck();
    }
}


void getSensor(String temp){

   switch(temp.charAt(0)) {
       case 'a': {
          myacceleration();
          break;
       }
       case 'c': {
          myMagnetic();
          break;
       }
       case 't': {
          mytemp();
         break;
       }
      default: {
      cc3000Client.println("Invalid input: Incorret sensor");
      cc3000Client.flush();
       break;
     }
     
   }
}  

void enableAutoAlarm(String temp) {
  switch(temp.charAt(0)) {
       case 'a': {
         if(accelAutoAlarm)  {
          cc3000Client.fastrprint("Acceler auto-alarm is disabled!");
          cc3000Client.flush();
          accelAutoAlarm = false;
         } else {
          cc3000Client.fastrprint("Acceler auto-alarm is enabled!");
          cc3000Client.flush();
          accelAutoAlarm = true;
         }
         sendAck();
          break;
       }
       case 'c': {
          if(compAutoAlarm)  {
          cc3000Client.fastrprint("Compass auto-alarm is disabled!\n");
          cc3000Client.flush();
          enableCompass = false;
         } else {
          cc3000Client.fastrprint("Compass auto-alarm is enabled!\n");
          cc3000Client.flush();
          enableCompass = true;
         }
         sendAck();
           break;
       }
       case 't': { 
         if(tempAutoAlarm)  {
          cc3000Client.fastrprint("Temperature auto-alarm is disabled!\n");
          cc3000Client.flush();
          tempAutoAlarm = false;
         } else {
          cc3000Client.fastrprint("Temperature auto-alarm is enabled!\n");
          cc3000Client.flush();
          tempAutoAlarm = true;
         }
         sendAck();
         break;
       }
      default: {
      cc3000Client.fastrprint("Invalid input: Could not find appropirate divace to disable or enable auto-alarm\n");
      cc3000Client.flush();
       break;
     }
   }
}


// enable/disable timer for peridoical report
void enableTimerDivices(String temp) {
  switch(temp.charAt(0)) {
       case 'a': {
         if(enableAcceler)  {
          cc3000Client.fastrprint("Acceler periodical report is disabled!\n");
          cc3000Client.flush();
          enableAcceler = false;
         } else {
          cc3000Client.fastrprint("Acceler periodical report is enabled!\n");
          cc3000Client.flush();
          enableAcceler = true;
         }
         sendAck();
          break;
       }
       case 'c': {
          if(enableCompass)  {
          cc3000Client.fastrprint("Compass periodical report is disabled!\n");
          cc3000Client.flush();
          enableCompass = false;
         } else {
          cc3000Client.fastrprint("Compass periodical report is enabled!\n");
          cc3000Client.flush();
          enableCompass = true;
         }
         sendAck();
           break;
       }
       case 't': { 
          if(enableTemp)  {
          cc3000Client.fastrprint("Tempeture periodical report is disabled!\n");
          cc3000Client.flush();
          enableTemp = false;
         } else {
          cc3000Client.fastrprint("Tempeture periodical report is enabled!\n");
          cc3000Client.flush();
          enableTemp = true;
         }
         sendAck();
         break;
       }
      default: {
      cc3000Client.println("Invalid input: Could not find appropirate divace to disable");
      cc3000Client.flush();
       break;
     }
   }
}


void configMode(String temp) {

       switch(temp.charAt(2)) {
       case 't': {
        configThreshold(temp);
          break;
       }
       case 'd': {
        configDelay(temp);
          break;
       }
      default: {
      cc3000Client.println("Invalid input: Incorret Config Request!");
      cc3000Client.flush();
       break;
     }
   } 
}  


void configThreshold(String temp) {
      switch(temp.charAt(0)) {
       // config accel threshold
       case 'a': {
          chooseXYZ_Accel(temp);
          break;
       }
       // config 
       case 'c': {
        chooseXYZ_Compass(temp);
          break;
       }
       case 't': {
        
        choose_Temp(temp);
          break;
       }
      default: {
      cc3000Client.println("Invalid input: Incorret Config Request! for ThreshHold!");
      cc3000Client.flush();
       break;
      }
    }
}
void configDelay(String temp) {
      ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
     int temp1;
     temp1 = getValue(temp,2);
      if(temp1 > 999  &&  temp1 < 10001) {
           
      switch(temp.charAt(0)) {
       case 'a': {
            accelerDelay = temp1;
            AccelTimer.configTime(accelerDelay,AccelTimerEvent);
            cc3000Client.fastrprint("Set Delay for Accel : ");
            cc3000Client.println(accelerDelay);
            cc3000Client.flush();
            
         break;
       }

       case 'c': {
            compassDelay = temp1;
            CompassTimer.configTime(compassDelay,CompassTimerEvent);
            cc3000Client.fastrprint("Set Delay for Compass : ");
            cc3000Client.println(compassDelay);
            cc3000Client.flush();

        
          break;
       }
       case 't': {
            tempDelay = temp1;
            TempTimer.configTime(tempDelay,TempTimerEvent);
            cc3000Client.fastrprint("Set Delay for Tempeture : ");
            cc3000Client.println(tempDelay);
            cc3000Client.flush();
          break;
       }
      default: {
      cc3000Client.println("Invalid input: Incorret Config Request! for ThreshHold!");
      cc3000Client.flush();
       break;
      }
     }
   }
     else {
            cc3000Client.fastrprint("Invalid delay: the range has to be between 2000 and 10000!\n");
            cc3000Client.flush();
          } 

          sendAck();
}



void chooseXYZ_Accel(String temp) {
        ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
        switch(temp.charAt(3)) {
       // config accel threshold
       case 'x': {
          AccelMaxXthresh = getValue(temp,3);
          cc3000Client.fastrprint("Set Threshold for Accel X: ");
          cc3000Client.println(AccelMaxXthresh);
          cc3000Client.flush();
          break;
       }
       // config 
       case 'y': {
          AccelMaxYthresh = getValue(temp,3);
          cc3000Client.fastrprint("Set Threshold for Accel Y: ");
          cc3000Client.println(AccelMaxYthresh);
          cc3000Client.flush();
          
          break;
       }
       case 'z': {
          AccelMaxZthresh = getValue(temp,3);
          cc3000Client.fastrprint("Set Threshold for Accel Z: ");
          cc3000Client.println(AccelMaxZthresh);
          cc3000Client.flush();
          break;
       }
             default: {
      
      
      cc3000Client.println("Invalid input: Incorret Config Request for Accel ThreshHold!");
     //sendAck();
      cc3000Client.flush();
      
       break;
     }
   }
   sendAck();
}

void chooseXYZ_Compass(String temp) {
        ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
        switch(temp.charAt(3)) {
       // config accel threshold
       case 'x': {    
          MagMaxXthresh = getValue(temp,3);
          cc3000Client.fastrprint("Set Threshold for COMPASS X: ");
          cc3000Client.println(MagMaxXthresh);
          sendAck();
          cc3000Client.flush();
          break;
       }
       // config 
       case 'y': {
          MagMaxYthresh = getValue(temp,3);
          cc3000Client.fastrprint("Set Threshold for COMPASS Y: ");
          cc3000Client.println(MagMaxYthresh);
          sendAck();
          cc3000Client.flush();
          break;
       }
       case 'z': {
          MagMaxZthresh = getValue(temp,3);
          cc3000Client.fastrprint("Set Threshold for COMPASS Z: ");
          cc3000Client.println(MagMaxZthresh);
          sendAck();
          cc3000Client.flush();
          break;
       }
             default: {
      cc3000Client.println("Invalid input: Incorret Config Request for Accel ThreshHold!");
      //sendAck();
      cc3000Client.flush();
       break;
     }
   }
   sendAck();
}
void choose_Temp(String temp) {
    ++count;
    cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
    tempThreshold = getValue(temp, 2);
    cc3000Client.fastrprint("Set Threshold for TEMPERATURE: ");
    cc3000Client.println(tempThreshold);
    //sendAck();
    cc3000Client.flush();
    sendAck();
}

//Assumption: Start at the end of a string, move up
int getValue(String temp, int location) {
  int strSize = temp.length();
  int sum = 0;
  int n = 1;
  for (strSize; strSize > location; strSize--) {

    int a = temp.charAt(strSize);
    if (a < 58 && a > 47) {
       // convert from char to int 
       int ia = a - '0';
       sum += (ia * n);
       n *= 10;
    }
  }
  return sum;
}




/**************************************************************************/
/*!
    @brief  Begins an SSID scan and prints out all the visible networks
*/
/**************************************************************************/

void listSSIDResults(void)
{
  uint32_t index;
  uint8_t valid, rssi, sec;
  char ssidname[33]; 

  if (!cc3000.startSSIDscan(&index)) {
    Serial.println(F("SSID scan failed!"));
    return;
  }

  Serial.print(F("Networks found: ")); Serial.println(index);
  Serial.println(F("================================================"));

  while (index) {
    index--;

    valid = cc3000.getNextSSID(&rssi, &sec, ssidname);
    
    Serial.print(F("SSID Name    : ")); Serial.print(ssidname);
    Serial.println();
    Serial.print(F("RSSI         : "));
    Serial.println(rssi);
    Serial.print(F("Security Mode: "));
    Serial.println(sec);
    Serial.println();
  }
  Serial.println(F("================================================"));

  cc3000.stopSSIDscan();
}

/**************************************************************************/
/*!
    @brief  Tries to read the IP address and other connection details
*/
/**************************************************************************/


void handleAutoAlarm()
{
  sensors_event_t magEvent ,accelEvent ;   
  mag.getEvent(&magEvent);
  accel.getEvent(&accelEvent);

  if (accelAutoAlarm)
  {
    if (AccelMaxXthresh < accelEvent.acceleration.x || AccelMaxYthresh < accelEvent.acceleration.y || AccelMaxZthresh < accelEvent.acceleration.z)
    {
          ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
        cc3000Client.fastrprint("ALARM: Accelerator reached the threshold!\n");
        cc3000Client.flush();
        myacceleration();
        
    }
  }

  if (compAutoAlarm)
  {
    if (MagMaxXthresh < magEvent.magnetic.x || MagMaxYthresh < magEvent.magnetic.y || MagMaxZthresh < magEvent.magnetic.z)
    {
        ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
        cc3000Client.fastrprint("ALARM: Compass reached the threshold!\n");
        cc3000Client.flush();
        myMagnetic();
    }
  }

  if (tempAutoAlarm)
  {
    getTemp();
    if (temperatureC > tempThreshold)
    {   ++count;
        cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
        cc3000Client.fastrprint("ALARM: Temperature reached the threshold!\n");
        cc3000Client.flush();
        mytemp();
        
    }
  }
}

void myacceleration() {
  /* Get a new sensor event */ 

  sensors_event_t accelEvent; 
  accel.getEvent(&accelEvent);

  
  if (accelEvent.acceleration.x < AccelMinX) AccelMinX = accelEvent.acceleration.x;
  if (accelEvent.acceleration.x > AccelMaxX) AccelMaxX = accelEvent.acceleration.x;
  
  if (accelEvent.acceleration.y < AccelMinY) AccelMinY = accelEvent.acceleration.y;
  if (accelEvent.acceleration.y > AccelMaxY) AccelMaxY = accelEvent.acceleration.y;
 
  if (accelEvent.acceleration.z < AccelMinZ) AccelMinZ = accelEvent.acceleration.z;
  if (accelEvent.acceleration.z > AccelMaxZ) AccelMaxZ = accelEvent.acceleration.z;
    
    ++count;
    cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
     cc3000Client.fastrprint("ACCELEROMETER: \n");
    cc3000Client.fastrprint("Accel Minimums: "); cc3000Client.print(AccelMinX);  cc3000Client.fastrprint("  "); cc3000Client.print(AccelMinY);  cc3000Client.fastrprint("  ");  cc3000Client.print(AccelMinZ);  cc3000Client.println();
    //cc3000Client.flush();
    cc3000Client.fastrprint("Accel Maximums: ");  cc3000Client.print(AccelMaxX); cc3000Client.fastrprint("  "); cc3000Client.print(AccelMaxY);  cc3000Client.fastrprint("  ");  cc3000Client.print(AccelMaxZ);  cc3000Client.println();
    sendAck();
    cc3000Client.flush();

}


void myMagnetic(){
  sensors_event_t magEvent;   
  mag.getEvent(&magEvent);

  if (magEvent.magnetic.x < MagMinX) MagMinX = magEvent.magnetic.x;
  if (magEvent.magnetic.x > MagMaxX) MagMaxX = magEvent.magnetic.x;
  
  if (magEvent.magnetic.y < MagMinY) MagMinY = magEvent.magnetic.y;
  if (magEvent.magnetic.y > MagMaxY) MagMaxY = magEvent.magnetic.y;
 
  if (magEvent.magnetic.z < MagMinZ) MagMinZ = magEvent.magnetic.z;
  if (magEvent.magnetic.z > MagMaxZ) MagMaxZ = magEvent.magnetic.z;
   ++count;
   cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
   cc3000Client.fastrprint("COMPASS: \n");
   cc3000Client.fastrprint("Mag Minimums: ");  cc3000Client.print(MagMinX);  cc3000Client.fastrprint("  "); cc3000Client.print(MagMinY); cc3000Client.fastrprint("  ");  cc3000Client.print(MagMinZ);  cc3000Client.println();
   //cc3000Client.flush();
   cc3000Client.fastrprint("Mag Maximums: ");   cc3000Client.print(MagMaxX);  cc3000Client.fastrprint("  ");cc3000Client.print(MagMaxY);  cc3000Client.fastrprint("  ");  cc3000Client.print(MagMaxZ);  cc3000Client.println();
   sendAck();
   cc3000Client.flush();
}


void mytemp()
{
   digitalWrite(tempPin, HIGH);
  // TEMP SECTION
  //getting the voltage reading from the temperature sensor
  tempReading = analogRead(tempPin);  
   
  // converting that reading to voltage, for 3.3v arduino use 3.3
  voltage = tempReading * 5.0;
  voltage /= 1024.0; 

  ++count;
  cc3000Client.fastrprint("Message #"); cc3000Client.println(count);
  cc3000Client.fastrprint("Tempeture: \n");
  // print out the voltage
  cc3000Client.print(voltage); cc3000Client.fastrprint("volts \n");
   
  // now print out the temperature
  temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                          //to degrees ((voltage - 500mV) times 100)
  cc3000Client.print(temperatureC); cc3000Client.fastrprint(" degrees C \n");
 
   
  // now convert to Fahrenheit
  temperatureF = (temperatureC * 9.0 / 5.0) + 32.0;
  cc3000Client.print(temperatureF); cc3000Client.fastrprint(" degrees F \n");
  sendAck();
  cc3000Client.flush();
}


void getTemp() {
   digitalWrite(tempPin, HIGH);
  // TEMP SECTION
  //getting the voltage reading from the temperature sensor
  tempReading = analogRead(tempPin);  
   
  // converting that reading to voltage, for 3.3v arduino use 3.3
  voltage = tempReading * 5.0;
  voltage /= 1024.0; 
   
  // now print out the temperature
  temperatureC = (voltage - 0.5) * 100 ;  //converting from 10 mv per degree wit 500 mV offset
                          //to degrees ((voltage - 500mV) times 100)
}


