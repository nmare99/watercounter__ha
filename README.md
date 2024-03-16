# README #

Простая плата на базе микросхемы ESP8266 (Wemos D1 mini). Плата считает кол-во импульсов от счетчиков воды (горячей и холодной) и импульс от системы АкваСторож.
Отображение в web интерфейсе и отправка на сервер умного дома по протоколу MQTT

Установка файловой системы [https://github.com/esp8266/arduino-esp8266fs-plugin](https://github.com/earlephilhower/arduino-esp8266littlefs-plugin)


# Распиновка #
* Красная лампа - D8 - GPIO15
* Синяя лампа - D7 - GPIO13
* Встроенная лампа - D4 - GPIO2
* Холодная вода - D5 - GPIO14
* Горячая вода - D2 - GPIO4
* ALARM - D6 - GPIO12


## Ссылки ##
* [esptool-gui](https://github.com/Rodmg/esptool-gui)
* [esptool](https://github.com/espressif/esptool)


## Код для заливки прошивки ##
```
#!bash

./esptool.py --port /dev/cu.wchusbserial1420 write_flash -fm=dio -fs=4MB 0x00000 ~/Downloads/0x00000_ESP8266_201705062105.bin

```
