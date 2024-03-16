void MQTT_init() {
	HTTP.on("/mqtt", handle_Set_MQTT); // Установка настроек MQTT Сервера /mqtt?mqtt_host=192.168.1.200&mqtt_port=1883&mqtt_user=&mqtt_password=
	
	MQTT_connect();
}

void MQTT_loop() {
	if (WiFi.status() != WL_CONNECTED) return;
	
	if (!clientForMQTT.connected()) {
		if (currentMillis < mqtt_reconnect_previous_millis) mqtt_reconnect_previous_millis = 0;
		if(currentMillis - mqtt_reconnect_previous_millis >= mqtt_reconnect_interval) {
			mqtt_reconnect_previous_millis = currentMillis;
			if (MQTT_reconnect()) {
				//mqtt_reconnect_previous_millis = 0;
			}
		}
	} else {
		clientForMQTT.loop();
	}
	
}

boolean MQTT_reconnect() {
	error_log("MQTT reconnect");
	MQTT_connect();
	return clientForMQTT.connected();
}

void Mqtt_HA_Discovery() {
  
  error_log("MQTT auto discovery setting for counter topics");
  
  clientForMQTT.setBufferSize (512); //Делаем размер буфера загрузки больше
  
  JsonDocument cold_config, hot_config;
  JsonObject device;
  String payload;
  
  String cold_discovery_topic = _mqtt_topic + ESP_Name + "/cold/config";
  String hot_discovery_topic = _mqtt_topic + ESP_Name + "/hot/config";
  
  cold_config["unit_of_meas"] = "м3";
  cold_config["stat_cla"] =     "measurement";
  cold_config["name"] =         ESP_Name + " cold water";
  cold_config["stat_t"] =       _mqtt_topic + ESP_Name + "/cold/state";
  cold_config["uniq_id"] =      ESP_Name + ".cold";
   //"value_template": "{{ value | round (3) }}" 
  device = cold_config["dev"].to<JsonObject>();
  device["name"] =              ESP_Name;
  device["ids"] =               ha_ids;
  device["sw_version"] =        _revision;
  device["mdl"] =               "d1_mini";
  device["manufacturer"] =      "nmare";
  
  hot_config["unit_of_meas"] = "м3";
  hot_config["stat_cla"] =     "measurement";
  hot_config["name"] =         ESP_Name + " hot water";
  hot_config["stat_t"] =       _mqtt_topic + ESP_Name + "/hot/state";
  hot_config["uniq_id"] =      ESP_Name + ".hot";
  //"value_template": "{{ value | round (3) }}"
  device = hot_config["dev"].to<JsonObject>();
  device["name"] =              ESP_Name;
  device["ids"] =               ha_ids;
  device["sw_version"] =        _revision;
  device["mdl"] =               "d1_mini";
  device["manufacturer"] =      "nmare";  

  serializeJson(cold_config, payload);
  MQTT_publish(cold_discovery_topic, payload);  
  serializeJson(hot_config, payload);
  MQTT_publish(hot_discovery_topic, payload);
//  error_log(payload);  
  clientForMQTT.setBufferSize (256);  //Возвращаем размер буфера загрузки по умолчанию

}

void MQTT_connect() {
	error_log("MQTT connect");
	if (!_mqtt_host || _mqtt_port < 1) return;
	
	if (WiFi.status() != WL_CONNECTED) return;
	
	error_log("MQTT set Server IP and PORT");
	clientForMQTT.setServer(_mqtt_host.c_str(), _mqtt_port);
	clientForMQTT.setCallback(callbackForMQTT);
	
	error_log("MQTT connecting...");
	
	if (!clientForMQTT.connected()) {
		wifiClientForMQTT.stop();
		if (_mqtt_user) {
			if (!clientForMQTT.connect(ESP_Name.c_str(), _mqtt_user.c_str(), _mqtt_password.c_str())) {
				error_log("MQTT can't connect");
				return;
			}
		} else {
			if (!clientForMQTT.connect(ESP_Name.c_str())) {
				error_log("MQTT can't connect");
				return;
			}
		}
	}
	
	error_log("MQTT connected");
  
	// Подписываемся на топики
	String topic = _mqtt_topic + ESP_Name + "/#";
	error_log("MQTT subscribe to topic "+topic);
	clientForMQTT.subscribe(topic.c_str());
  clientForMQTT.subscribe(_ha_status.c_str());
}

void MQTT_publish(String topic, String message) {
	if (WiFi.status() != WL_CONNECTED) return;
	error_log("MQTT publish " + topic + " = " + message);
	if (!clientForMQTT.connected()) {
		MQTT_reconnect();
	}
	clientForMQTT.publish(topic.c_str(), message.c_str(), true);
}

//  Функция вызывается, когда есть изменения по топику на который мы подписаны!
void callbackForMQTT(char* topic, byte* payload, unsigned int length) {
	error_log("MQTT callbackForMQTT");
	
	String topic_String = (String) topic;
	String payload_String = (String)((char *)payload);

	payload_String = payload_String.substring(0,length);
	
	error_log("incoming: "+topic_String+" - "+payload_String);
	boolean saveConfigNeed = false;
	String t = _mqtt_topic + ESP_Name + "/";
	
	topic_String.remove(0, t.length());
 
  // Устанавливаем параметры сенсоров
  if(String(topic) == _ha_status) 
  {
        if(payload_String == "online")
           Mqtt_HA_Discovery();
  }

	if (topic_String.equals("cold/state")) {
//    Serial.println(String((unsigned long)(payload_String.toFloat() * _Div1)));
		if (ColdWaterCount != (unsigned long)(payload_String.toFloat() * _Div1) ) saveConfigNeed = true;
		ColdWaterCount = (unsigned long)(payload_String.toFloat() * _Div1);
	} else if (topic_String.equals("hot/state")) {
		if (HotWaterCount != (unsigned long)(payload_String.toFloat() * _Div1) ) saveConfigNeed = true;
		HotWaterCount = (unsigned long)(payload_String.toFloat() * _Div1);
	} else if (topic_String.equals("alert/state")) {
		if (Alert != payload_String.toInt()) saveConfigNeed = true;
		Alert = payload_String.toInt();
	}

	if (saveConfigNeed) saveConfig();
}

void handle_Set_MQTT() {
	if(!HTTP.authenticate(_http_user.c_str(), _http_password.c_str())) return HTTP.requestAuthentication();
	_mqtt_host = HTTP.arg("mqtt_host");
	_mqtt_port = HTTP.arg("mqtt_port").toInt();
	_mqtt_user = HTTP.arg("mqtt_user");
	_mqtt_password = HTTP.arg("mqtt_password");
	saveConfig();                         // Функция сохранения данных во Flash
	HTTP.send(200, "text/plain", "OK");   // отправляем ответ о выполнении
	
	MQTT_connect();
}
