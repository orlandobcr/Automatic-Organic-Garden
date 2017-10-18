/*
 A solar powered organic garden using an Arduino controller for sensing the soil humudity, air temperature, air humidity, water ph, light intensity and rain.
 The arduino uses an ethernet shield to create a bi-directional comunication using the MQTT protocol with a rapsberry pi (broker). The Raspbery-pi analizes and stores the data in a cloud-service (ThingSpeak).
 The Rasperry-pi also controls two electro-valvules for the irrigation process.
 */

#include <SPI.h>
#include <Ethernet.h>
#include <PubSubClient.h>
#include <DHT.h>

#define DHTTYPE DHT22
#define DHTPIN1 4  

#define PUMP1_PIN 7
#define PUMP2_PIN 8
#define RESET_PIN 9

DHT dht1(DHTPIN1, DHTTYPE);  //air humidity and temeprature sensor definition DHT22

unsigned long seconds = 1000L;
unsigned long minutes = seconds * 60;
unsigned long hours = minutes * 60; 

float temp1=0, hum_aire1=0;
int hum_tierra1=0, hum_tierra2=0, hum_tierra3=0, hum_tierra4=0;
float hum_avg=0;

int pump1_status=0, pump2_status=0;

int riego_on=30, riego_off=70;

byte mac[] = { 0xD4, 0x28, 0xB2, 0xFF, 0xA0, 0xA1 }; // Must be unique on local network
boolean lastConnected = false;

IPAddress mqtt_server(192, 168, 1, 108);


//SENSORES
const char* temp1_a_topic = "temp1_a";
const char* hum1_a_topic = "hum1_a";
const char* hum1_t_topic = "hum1_t";
const char* hum2_t_topic = "hum2_t";
const char* hum3_t_topic = "hum3_t";
const char* hum4_t_topic = "hum4_t";
const char* hum_t_avg_topic = "hum_t_avg";

//CONTROLES
const char* pump1_topic = "pump1";
const char* pump2_topic = "pump2";
const char* reboot_topic = "reboot";


//ESTADO_ACTUADORES
const char* pump1_status_topic = "pump1_status";
const char* pump2_status_topic = "pump2_status";


int failedCounter = 0;

EthernetClient ethClient;
PubSubClient client(ethClient);


void setup()
{
  
  pinMode(PUMP1_PIN, OUTPUT);  
  pinMode(PUMP2_PIN, OUTPUT);  
  pinMode(RESET_PIN, OUTPUT);
    
  digitalWrite(PUMP1_PIN, HIGH);
  digitalWrite(PUMP2_PIN, HIGH);
  digitalWrite(RESET_PIN, HIGH);

  Serial.begin(9600);   //for debuging
  
  dht1.begin();

  startEthernet();
  
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
  
  
}

void loop(){

  
  read_sensors();
 // print_sensor_data();

  
   if (!client.connected()) {
    Serial.print("Connecting ...\n");
     if (client.connect("Arduino_MQTT")) {
        Serial.println("connected");
        client.subscribe(pump1_topic);
        client.subscribe(pump2_topic);
        client.subscribe(reboot_topic);
      }
  }
  else {

  char buffer[10]; 
     
  dtostrf(temp1, 4, 1, buffer);
  client.publish(temp1_a_topic, buffer);
  
  dtostrf(hum_aire1, 4, 1, buffer);
  client.publish(hum1_a_topic, buffer);


   dtostrf(hum_tierra1, 4, 1, buffer);
  client.publish(hum1_t_topic, buffer);

   dtostrf(hum_tierra2, 4, 1, buffer);
  client.publish(hum2_t_topic, buffer);

   dtostrf(hum_tierra3, 4, 1, buffer);
  client.publish(hum3_t_topic, buffer);

   dtostrf(hum_tierra4, 4, 1, buffer);
  client.publish(hum4_t_topic, buffer);

    dtostrf(hum_avg, 4, 1, buffer);
  client.publish(hum_t_avg_topic, buffer);

   dtostrf(pump1_status, 4, 1, buffer);
  client.publish(pump1_status_topic, buffer);
  
   dtostrf(pump2_status, 4, 1, buffer);
  client.publish(pump2_status_topic, buffer);
    

delay(2000);
  }

  client.loop();
  
  
}



void callback(char* topic, byte* payload, unsigned int length) {

  
  
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  int i=0;
  for (i=0;i<length;i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();

  payload[length] = '\0';
  String data;
  data = String((char*)payload);
  

   if (strcmp(topic,"pump1")==0) {
    if(data=="1"){
      Serial.println("Bomba1 ON");
      digitalWrite(PUMP1_PIN, LOW);
      }else{
      Serial.println("Bomba1 OFF");
      digitalWrite(PUMP1_PIN, HIGH);
      }
  }

  
  if (strcmp(topic,"pump2")==0) {
    if(data=="1"){
      Serial.println("Bomba2 ON");
      digitalWrite(PUMP2_PIN, LOW);
      }else{
      Serial.println("Bomba2 OFF");
      digitalWrite(PUMP2_PIN, HIGH);
      }
  }
  
  if (strcmp(topic,"reboot")==0) {
    if(data=="1"){
      reboot_arduino();
      }else{
      Serial.println("No valid command");
      }
  }
  
}

void read_sensors(){
  
    temp1 = dht1.readTemperature();
    hum_aire1 = dht1.readHumidity();
  
    hum_tierra1 = analogRead(A0);
    hum_tierra2 = analogRead(A1);
    hum_tierra3 = analogRead(A2);
    hum_tierra4 = analogRead(A4);
 
    hum_tierra1 = map(hum_tierra1, 250, 1023, 100, 0);
    hum_tierra2 = map(hum_tierra2, 350, 1023, 100, 0);
    hum_tierra3 = map(hum_tierra3, 250, 1023, 100, 0);
    hum_tierra4 = map(hum_tierra4, 240, 1023, 100, 0);

    hum_avg=(hum_tierra1+hum_tierra2+hum_tierra3+hum_tierra4)/4;

    pump1_status = digitalRead(PUMP1_PIN);
    pump2_status = digitalRead(PUMP2_PIN);
    
  }



void print_sensor_data(){
  
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
     
  }


void startEthernet(){
  
//  client.stop();

  Serial.println("Connecting Arduino to network...");
  
  delay(1000);
  
  // Connect to network amd obtain an IP address using DHCP
  if (Ethernet.begin(mac) == 0)
  {
    Serial.println("DHCP Failed, reset Arduino to try again");
    reboot_arduino();
    failedCounter++;
  }
  else
  {
    Serial.println("Arduino connected to network using DHCP");
  }
  
  delay(1000);
}


void check_com_failure(){
  
   if (failedCounter != 0 ) {
    reboot_arduino();
    } 
  
  }


void reboot_arduino(){
 
  Serial.println("Rebooting...");
  delay(2000);
  digitalWrite(RESET_PIN, LOW);  

}






  

