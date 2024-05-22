#include <ESP8266WiFi.h> 
#include <ESP8266WebServer.h>
#include <LittleFS.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <ESP8266HTTPUpdateServer.h>

#define RED_LED_PIN 15
#define BLUE_LED_PIN 13
#define INNER_LED_PIN 2

#define COLD_WATER_PIN 14
#define HOT_WATER_PIN 4
#define ALARM_WATER_PIN 12

// Объект для обнавления с web страницы 
ESP8266HTTPUpdateServer httpUpdater;

// Для файловой системы
File fsUploadFile;

String _revision = "1.7"; // Версия кода
char ha_ids[32]; 
byte mac[6]; 

// Определяем переменные wifi
String _ssid     = "APName"; // Для хранения SSID
String _password = "pass"; // Для хранения пароля сети
String _ssidAP = "WaterCounter";   // SSID AP точки доступа
String _passwordAP = ""; // пароль точки доступа
IPAddress apIP(192, 168, 0, 1);

String _http_user = "admin";
String _http_password = "0000";

String _mqtt_host     = "host";
int _mqtt_port     = 1883;
String _mqtt_user     = "user";
String _mqtt_password     = "pass";
String _mqtt_topic     = "homeassistant/sensor/";
String _ha_status = "homeassistant/status";


String ESP_Name = "iCounter";
int timezone = 3;               // часовой пояс GTM
int SaveCount = 0;

String jsonConfig = "{}";
int port = 80;

// Web интерфейс для устройства
ESP8266WebServer HTTP(port);
  
unsigned long ColdWaterCount = 0;
unsigned long HotWaterCount = 0;
time_t local_cold_update_time;
time_t local_hot_update_time;
time_t mqtt_cold_update_time;
time_t mqtt_hot_update_time;
int Alert = 0;
int ColdWaterState = 0;
int HotWaterState = 0;
int AlertState = 0;

// Делители значений счетчика
int _Div1 = 1000;
int _Div2 = 1000;

int HighMillis=0;
int Rollover=0;

unsigned long currentMillis;
int save_time_interval = 60*60*1000;
unsigned long save_previous_millis = 0;
int wifi_mode_time = 2000;
int wifi_mode = 0;
unsigned long wifi_mode_previous_millis = 0;
int wifi_mode_reconnect_millis = 10*60*1000;
unsigned long wifi_mode_reconnect_previous_millis = 0;
int inner_led_state = LOW;
unsigned long mqtt_reconnect_previous_millis = 0;
int mqtt_reconnect_interval = 5000;
unsigned long water_send_previous_millis = 0;
int water_send_interval = 10000;
unsigned long water_previous_millis = 0;
int water_interval = 500;
unsigned long alert_led_previous_millis = 0;
int alert_led_interval = 100;
int alert_led_state = LOW;

boolean water_changes_for_send = true;

WiFiClient wifiClientForMQTT;
PubSubClient clientForMQTT(wifiClientForMQTT);

String Log = "";

void error_log(String str) {
	error_log(str, true);
}

void error_log(String str, boolean ln) {
	Serial.print(str);
	
	if (ln) Serial.println("");
	
	Log = "[" + (String)GetTime() + "] " + str + "<br>" + Log;
	
	if (Log.length() >= 4096) {
		Log = Log.substring(0, 4096);
	}
}

void array_to_string(byte array[], unsigned int len, char buffer[])
{
    for (unsigned int i = 0; i < len; i++)
    {
        byte nib1 = (array[i] >> 4) & 0x0F;
        byte nib2 = (array[i] >> 0) & 0x0F;
        buffer[i*2+0] = nib1  < 0xA ? '0' + nib1  : 'a' + nib1  - 0xA;
        buffer[i*2+1] = nib2  < 0xA ? '0' + nib2  : 'a' + nib2  - 0xA;
    }
    buffer[len*2] = '\0';
}

void setup() {

	Serial.begin(115200);
	error_log("");
  
  //Берем МАС адрес и используем как ids для HA 
  WiFi.macAddress(mac);
  array_to_string(mac, 6, ha_ids);
  //Serial.println(ha_ids);
  
	//Запускаем файловую систему
	error_log("Start File System");
	FS_init();
	
	error_log("Load Config File");
	loadConfig();
	
	error_log("Start WiFi");
	//Запускаем WIFI
	WIFI_init();
	
	error_log("Start Time module");
	// Получаем время из сети
	Time_init();

	error_log("Start MQTT module");
	MQTT_init();
	
	//Настраиваем и запускаем HTTP интерфейс
	error_log("Start WebServer");
	HTTP_init();
		
	error_log("Start Water module");
	Water_init();
}

void loop() {
	HTTP_loop();
  //Serial.printf("heap size: %u\n", ESP.getFreeHeap());
	
	delay(1);
	Time_loop();
	
	WIFI_loop();

	FileConfig_loop();

	MQTT_loop();

	Water_loop();
	
}
