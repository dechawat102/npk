#include <ModbusMaster.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

//สร้างตัวแปร Modbus 
ModbusMaster node;
// กำหนดขา
//หมายเหตุ esp32 สามารถทำ hardware serial ได้ทุก pin แต่จะต้องใช้แรงดัน 3.3v 
//ดังนั้นหากใช้ตัวแปลงที่พูดคุยกันในแรงดัน 5v จะต้องเป็น Serial ที่ถูกกำหนดมาซึ่งจะเป็นขา 16 17
#define RX2 17
#define TX2 16
#define MAX485_DE 5
#define MAX485_RE_NEG 4



const char* ssid = "iPhone";
const char* password = "11111111";
const char* mqtt_server = "broker.netpie.io";
const char* client_id = "014c3929-02f6-43b8-b9b7-f48db4e1cc37";
const char* token     = "D2nJSCTmMjLnARaFkiBXRBVdTZujceKs";
const char* secret    = "77QYQvdVrGucWcxCL84eUryizoCVPZzp";


WiFiClient espClient;
PubSubClient client(espClient);
int ledState = LOW; 
unsigned long previousMillis = 0;
uint8_t result;

uint16_t soil_moisture;
int16_t soil_temperature;
uint16_t soil_ec;
uint16_t soil_ph;
uint16_t soil_n;
uint16_t soil_p;
uint16_t soil_k;


//ตัวอย่างโค๊ดนี้ไม่ได้ใช้ขา DE+RE ดังนั้นจึงใช้ LED เพื่อบอกสถานะเท่านั้น
//หากใช้ขา DE+RE จะต้องมากำหนด Pin input output เพื่อเขียน
void preTransmission() {
  digitalWrite(MAX485_RE_NEG, 1);
  digitalWrite(MAX485_DE, 1);
}

void postTransmission() {
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
}

void setup_wifi() {

  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  for (int i = 0; i < length; i++) {
    Serial.print((char)payload[i]);
  }
  Serial.println();
}


void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    String clientId = "TEST-NPK-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(client_id,token,secret)) {
      Serial.println("connected");
      client.subscribe("testnpk/test");
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}
void setup()
{
  pinMode(MAX485_RE_NEG, OUTPUT);
  pinMode(MAX485_DE, OUTPUT);
  digitalWrite(MAX485_RE_NEG, 0);
  digitalWrite(MAX485_DE, 0);
  Serial.begin(115200);
 //กำหนด Serial2 ให้ Data 8bit None 
  Serial2.begin(9600,SERIAL_8N1, RX2, TX2);
//กำหนดตัวแปรพอร์ตการเชื่อมต่อและหมายเลข slave id = 1
  node.begin(1, Serial2);
//กำหนดการ callbacks to toggle DE + RE on MAX485
  node.preTransmission(preTransmission);
  node.postTransmission(postTransmission);
  Serial.println("Setup OK!");
  Serial.println("----------------------------");
  Serial.println();
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}


void loop()
{
  unsigned long currentMillis = millis();
  const long interval = 3000;
  if (!client.connected()) {
    reconnect();
  }
  client.loop();
  if (currentMillis - previousMillis >= interval) 
  {
    previousMillis = currentMillis;

    if (ledState == LOW) 
    {
      ledState = HIGH;
    } else {
      ledState = LOW;
    }
    
    result = node.readHoldingRegisters(0x0000, 7);
    
    if (result == node.ku8MBSuccess) {
      soil_moisture     = (uint16_t)node.getResponseBuffer(0)/10;
      soil_temperature  = (int16_t)node.getResponseBuffer(1)/10;
      soil_ec           = (uint16_t)node.getResponseBuffer(2);
      soil_ph           = (uint16_t)node.getResponseBuffer(3)/10;
      soil_n            = (uint16_t)node.getResponseBuffer(4)/10;
      soil_p            = (uint16_t)node.getResponseBuffer(5)/10;
      soil_k            = (uint16_t)node.getResponseBuffer(6)/10;
      // soil_Salinity     = (uint16_t)node.getResponseBuffer(7);
      // soil_TDS          = (uint16_t)node.getResponseBuffer(8);1
      /*1
      Serial.print("Soil Moisture = ");
      Serial.print(soil_moisture);
      Serial.print(" : Soil Temperature = ");
      Serial.print(soil_temperature);
      Serial.print(" : Soil EC = ");
      Serial.print(soil_ec);
      Serial.print(" : Soil PH = ");
      Serial.print(soil_ph);
      Serial.print(" : Soil N = ");
      Serial.print(soil_n);
      Serial.print(" : Soil P = ");
      Serial.print(soil_p);
      Serial.print(" : Soil K = ");
      Serial.print(soil_k);
      Serial.print(" : Soil Salinity = ");
      Serial.print(soil_Salinity);
      Serial.print(" : Soil TDS = ");
      Serial.println(soil_TDS);
      Serial.println();
      */
    //ต่อข้อความเป็น json
    String json = "{\"data\": {\"N\":" + String(soil_n) +", \"P\":" + String(soil_p) +", \"K\":" + String(soil_k) + "}}";
                  // ", \"Tem\":" + String(soil_temperature) +
                  // ", \"EC\":" + String(soil_ec) +
                  // ", \"PH\":" + String(soil_ph) +
                  // ", \"N\":" + String(soil_n) +
                  // ", \"P\":" + String(soil_p) +
                  // ", \"K\":" + String(soil_k) +
                  // ", \"Sali\":" + String(soil_Salinity) +
                  // ", \"TDS\":" + String(soil_TDS) + "}";
                  // "{ "data" : "{\"H\":" + String(soil_moisture) +"}" }";
    String msgOut1 = String(soil_n);
    String msgOut2 = String(soil_p);
    String msgOut3 = String(soil_k);


    // แสดงผลที่แปลงเป็น json
    Serial.println(json);
    //แปลงเป็น char แล้วส่ง mqtt
    int jsonSize = json.length() + 1; 
    char jsonArray[jsonSize];
    json.toCharArray(jsonArray, jsonSize);   
    client.publish("@msg/D1/N", msgOut1.c_str());
    client.publish("@msg/D1/P", msgOut2.c_str());
    client.publish("@msg/D1/K", msgOut3.c_str());

    // client.publish("@shadow/data/update", jsonArray);
    } else {
      // update of status failed
      Serial.println("readHoldingRegisters(0x3000, 1) failed!");
    }



  }

}