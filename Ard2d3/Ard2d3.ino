/*
 Name:		Ard2d3.ino
 Created:	11.06.2017 9:36:08
 Author:	volv
*/

#include "SmartDelay.h"
#include <Wire.h>
#include <OneWire.h>
#include <Ultrasonic.h> 
Ultrasonic ultrasonic(12, 11); // Trig - 12, Echo - 11


OneWire  ds(10);  // on pin 10 (a 4.7K resistor is necessary)

// распиновка контактов
#define TSensorPin 3 // пин для датчика касания 
#define LightPin 4 // пин для лампочки
int swithLampPin = 2; // пин для включения прожектора
bool PompIsOn = false; // переменая "горит ли лампочка и вода"
bool LampIsOn = false; // переменая "включен ли свет?"

SmartDelay LampDelay(2000000UL); // Пауза для кнопки лампы
SmartDelay PompDelay(3000000UL); // Пауза для помпы и лампочки

void setup(void) {
	Serial.begin(9600);
	pinMode(TSensorPin, INPUT);
	pinMode(swithLampPin, OUTPUT);
	digitalWrite(swithLampPin, HIGH);
	pinMode(LightPin, OUTPUT);
	digitalWrite(LightPin, HIGH);
	pinMode(5, OUTPUT);
	pinMode(6, OUTPUT);
	pinMode(7, OUTPUT);
	pinMode(A5, INPUT);
}

void loop(void) {
	int inputCod;           // код входного напряжения 
	float inputVoltage; // входное напряжение в В
	byte i;
	byte present = 0;
	byte type_s;
	byte data[12];
	byte addr[8];
	float celsius, fahrenheit;

	// проверка нажатия сенсора

	int ctsValue = digitalRead(TSensorPin);
	// Serial.print(" TSensorPin = ");
	// Serial.println(ctsValue);
	if (ctsValue == HIGH && LampDelay.Now()) {


		if (!LampIsOn) {
			digitalWrite(swithLampPin, HIGH);
			
		}
		else
		{
			digitalWrite(swithLampPin, LOW);
		}
		LampIsOn = !LampIsOn;
		
	}
	if (!ds.search(addr)) {
		ds.reset_search();
		delay(600);
		return;
	}

	// the first ROM byte indicates which chip
	switch (addr[0]) {
	case 0x10:

		type_s = 1;
		break;
	case 0x28:

		type_s = 0;
		break;
	case 0x22:

		type_s = 0;
		break;
	default:
		delay(1);

	}

	ds.reset();
	ds.select(addr);
	ds.write(0x44, 1);        // start conversion, with parasite power on at the end



							  // maybe 750ms is enough, maybe not
							  // we might do a ds.depower() here, but the reset will take care of it.

	present = ds.reset();
	ds.select(addr);
	ds.write(0xBE);         // Read Scratchpad


	for (i = 0; i < 9; i++) {           // we need 9 bytes
		data[i] = ds.read();
	}

	//           КАКИЕ-ТО ХИТРЫЕ ВЫЧИСЛЕНИЯ
	//-----------------------------------------------------------
	// Convert the data to actual temperature
	// because the result is a 16 bit signed integer, it should
	// be stored to an "int16_t" type, which is always 16 bits
	// even when compiled on a 32 bit processor.
	int16_t raw = (data[1] << 8) | data[0];
	if (type_s) {
		raw = raw << 3; // 9 bit resolution default
		if (data[7] == 0x10) {
			// "count remain" gives full 12 bit resolution
			raw = (raw & 0xFFF0) + 12 - data[6];
		}
	}
	else {
		byte cfg = (data[4] & 0x60);
		// at lower res, the low bits are undefined, so let's zero them
		if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
		else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
		else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
											  //// default is 12 bit resolution, 750 ms conversion time
											  //---------------------------------------------------------
	}
	celsius = (float)raw / 16.0;
	Serial.print("  Temperature = ");
	Serial.println(celsius);
	if (celsius<20.00 && celsius != 0) {
		digitalWrite(5, LOW); // Включаем реле(подогрев) если температура(вода) < 20 градусов
	}
	else if (celsius>23.00) {
		digitalWrite(5, HIGH);// Выключаем реле(подогрев) если температура(вода) > 23 градусов
	}
	inputCod = analogRead(A5); // чтение напряжения на входе A5 
	inputVoltage = ((float)inputCod * 5. / 1024.);
	Serial.println(inputVoltage);
	if (inputVoltage == 0.00) {
		digitalWrite(6, LOW); // Включаем реле (насос) если тока (воды) нет
	}
	else if (inputVoltage != 0.00 && inputVoltage > 0.00) {
		digitalWrite(6, HIGH); // Выключаем реле (насос) если ток (вода) есть
	}
	float dist_cm = ultrasonic.distanceRead(CM); // дистанция в см
	Serial.println(dist_cm);// выводим дистанцию в порт

	if (dist_cm < 40) {
		PompIsOn = true;
		PompDelay.Wait();
		digitalWrite(LightPin, LOW);
	}
	if (PompIsOn && PompDelay.Now()) {
		PompIsOn = false;
		digitalWrite(LightPin, HIGH);
	}



	/*
	if (dist_cm < 35) {
		digitalWrite(7, LOW); // Включаем реле (насос) если датчик показывает < 30см
		delay(3000);

	}
	if (dist_cm > 35) {
		digitalWrite(7, HIGH); // Выключаем реле (насос) если датчик показывает >  30см
	}
	*/
}