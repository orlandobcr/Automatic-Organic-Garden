/*
 A solar powered organic garden using an Arduino controller for sensing the soil humudity, air temperature, air humidity, water ph, light intensity and rain.
 It uses an ethernet shield to conect to internet and store the collected data on the cloud (ThinkSpeak). 
 It analyzes the data to generate SMS and mail alerts about the state of the garden.
 It controls a water pump 
 
 */


#include <Adafruit_Sensor.h>
#include <DHT.h>
#include <EEPROM.h>
#include <SPI.h>
#include <Ethernet.h>


#define DHTTYPE DHT22
#define DHTPIN1 4  

#define RELAY_PIN 8
#define RESET_PIN 9

DHT dht1(DHTPIN1, DHTTYPE);  //air humidity and temeprature sensor definition

unsigned long seconds = 1000L;
unsigned long minutes = seconds * 60;
unsigned long hours = minutes * 60; 

int addr = 0;
byte emprom_riego_on, emprom_riego_off;

float temp1=0, hum_aire1=0;
int hum_tierra1=0, hum_tierra2=0, hum_tierra3=0, hum_tierra4=0;
float hum_avg=0;

int riego_on, riego_off;

byte mac[] = { 0xD4, 0x28, 0xB2, 0xFF, 0xA0, 0xA1 }; // Must be unique on local network
boolean lastConnected = false;

char thingSpeakAddress[] = "api.thingspeak.com";
String writeAPIKey = "3IFZZTIOUT0P61VN";

int failedCounter = 0;

EthernetClient client;

void setup()
{
  
  pinMode(RELAY_PIN, OUTPUT);  
  pinMode(RESET_PIN, OUTPUT);  
  digitalWrite(RELAY_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);


    Serial.begin(9600);   //for debuging
    dht1.begin();
    
  if(erprom_read()){
    Serial.print("Eprom var riego_on=");
    Serial.println(riego_on);
    Serial.print("Eprom var riego_off=");
    Serial.println(riego_off);
    }else{
      inicializar();
    }

    startEthernet();
}



void loop()
{
  
 temp1 = dht1.readTemperature();
 hum_aire1 = dht1.readHumidity();
  
 hum_tierra1 = analogRead(A0);
 hum_tierra2 = analogRead(A1);
 hum_tierra3 = analogRead(A2);
 hum_tierra4 = analogRead(A3);
 
 hum_tierra1 = map(hum_tierra1, 250, 1023, 100, 0);
 hum_tierra2 = map(hum_tierra2, 350, 1023, 100, 0);
 hum_tierra3 = map(hum_tierra3, 250, 1023, 100, 0);
 hum_tierra4 = map(hum_tierra4, 240, 1023, 100, 0);

 hum_avg=(hum_tierra1+hum_tierra2+hum_tierra3+hum_tierra4)/4;


  Serial.print("  T_a1: ");
  Serial.print(temp1,0);
  Serial.print("°C  H_a1: ");
  Serial.print(hum_aire1,0);
  
  Serial.print("%  H_t1: ");
  Serial.print(hum_tierra1);
  Serial.print("%  H_t2: ");
  Serial.print(hum_tierra2);
  Serial.print("%  H_t3: ");
  Serial.print(hum_tierra3);
  Serial.print("%  H_t4: ");
  Serial.print(hum_tierra4);
  Serial.print("%  AVG: ");
  Serial.print(hum_avg);
  Serial.println("%");

  char buffer[10];
  
  String tempC = dtostrf(temp1, 4, 1, buffer);
  String humC = dtostrf(hum_aire1, 4, 1, buffer);
  
  String hum_t1C = String(hum_tierra1);
  String hum_t2C = String(hum_tierra2);
  String hum_t3C = String(hum_tierra3);
  String hum_t4C = String(hum_tierra4);
  
 
  // Print Update Response to Serial Monitor
  if (client.available())
  {
    char c = client.read();
    Serial.print(c);
  }

  // Disconnect from ThingSpeak
  if (!client.connected() && lastConnected)
  {
    Serial.println("...disconnected");
    Serial.println();
    
    client.stop();
  }
  
   updateThingSpeak("field1="+tempC+"&field2="+humC+"&field3="+hum_t1C+"&field4="+hum_t2C+"&field5="+hum_t3C+"&field6="+hum_t4C);

   if(hum_avg > riego_on){
    digitalWrite(RELAY_PIN, LOW);
    Serial.println("RELAY ON ");
    }

   if(hum_avg < riego_off){
      digitalWrite(RELAY_PIN, HIGH);
      Serial.println("RELAY OFF");
    }
  
 
  
  
  // Check if Arduino Ethernet needs to be restarted
  if (failedCounter > 2 ) {
    reset_arduino();
    //startEthernet();
    }
  
lastConnected = client.connected();

delay(5*minutes);
}




void updateThingSpeak(String tsData)
{
  if (client.connect(thingSpeakAddress, 80))
  {         
    client.print("POST /update HTTP/1.1\n");
    client.print("Host: api.thingspeak.com\n");
    client.print("Connection: close\n");
    client.print("X-THINGSPEAKAPIKEY: "+writeAPIKey+"\n");
    client.print("Content-Type: application/x-www-form-urlencoded\n");
    client.print("Content-Length: ");
    client.print(tsData.length());
    client.print("\n\n");

    client.print(tsData);
    
        
    if (client.connected())
    {
      Serial.println("Connecting to ThingSpeak...");
      Serial.println();
      
      failedCounter = 0;
    }
    else
    {
      failedCounter++;
  
      Serial.println("Connection to ThingSpeak failed ("+String(failedCounter, DEC)+")");   
      Serial.println();
    }
    
  }
 
}



void startEthernet(){
  
  client.stop();

  Serial.println("Connecting Arduino to network...");
  Serial.println();  

  delay(1000);
  
  // Connect to network amd obtain an IP address using DHCP
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("DHCP Failed, reset Arduino to try again");
    Serial.println();
    reset_arduino();
  }
  else
  {
    Serial.println("Arduino connected to network using DHCP");
    Serial.println();
  }
  
  delay(1000);
}


void inicializar(){
  
  int RIEGO_ON_INIT_VALUE=30;
  int RIEGO_OFF_INIT_VALUE=92;

    EEPROM.write(0, RIEGO_ON_INIT_VALUE);
    Serial.print("Variable creada y guardada riego_on=");
    Serial.println(RIEGO_ON_INIT_VALUE);
    
    EEPROM.write(1, RIEGO_OFF_INIT_VALUE);
    Serial.print("Variable creada y guardada riego_off=");
    Serial.println(RIEGO_OFF_INIT_VALUE);
  
  }



void reset_arduino(){
 
  Serial.println("Rebooting...");
  delay(3000);
  digitalWrite(RESET_PIN, LOW);  

}



int erprom_read(){
  
    emprom_riego_on = EEPROM.read(0);
    emprom_riego_off = EEPROM.read(1);

    riego_on = emprom_riego_on;
    riego_off = emprom_riego_off;
    
    if(riego_on==0 || riego_off==0){
      Serial.println("No hay variables");
      return 0;
      }

return 1;
  }

  
