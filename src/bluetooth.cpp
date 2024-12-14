// #include <Arduino.h>
// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEServer.h>
// #include <BLE2902.h>
// #include "Wire.h"
// #include <I2S.h>
// #include <inttypes.h>
// #include <stdio.h>
// #include "esp_system.h"
// #include "freertos/FreeRTOS.h"
// #include "freertos/task.h"
// #define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
// #define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
// #define I2S_WS 22
// #define I2S_SD 17
// #define I2S_SCK 21
// #define SENSOR_PIN 32

// const unsigned long wait_for_scan = 10000;
// unsigned long next_scan_time = 0;
// const unsigned long wait_for_move = 11000;
// unsigned long next_move_time = 0;

// int threshold = -10000;
// int counter = 0;
// int last_counter = 0;
// int val = 0;
// int last_move = 0;
// int move = 0;


// class MyCallbacks: public BLECharacteristicCallbacks {
//     void onWrite(BLECharacteristic *pCharacteristic) {
//       std::string value = pCharacteristic->getValue();
//       if (value.length() > 0) {
//           Serial.println("*********");
//           Serial.print("Timer set for: ");
//           for (int i = 0; i < value.length(); i++)
//             Serial.print(value[i]);
//           Serial.print(" hours.");
//       }
//     }
// };

// BLECharacteristic *pCharacteristic;

// void setup() {
//   // Open serial communications and wait for port to open:
//   // A baud rate of 115200 is used instead of 9600 for a faster data rate
//   // on non-native USB ports
//   Serial.begin(115200);
//   Wire.begin();
//   BLEDevice::init("MyESP32");
//   BLEServer *pServer = BLEDevice::createServer();
//   BLEService *pService = pServer->createService(SERVICE_UUID);
//   pCharacteristic = pService->createCharacteristic(
//     CHARACTERISTIC_UUID,
//     BLECharacteristic::PROPERTY_READ |
//     BLECharacteristic::PROPERTY_WRITE |
//     BLECharacteristic::PROPERTY_NOTIFY
//   );

//   pCharacteristic->setCallbacks(new MyCallbacks());
//   pCharacteristic->addDescriptor(new BLE2902());
//   pCharacteristic->setValue("Set a timer in seconds: ");
//   pService->start();
//   BLEAdvertising *pAdvertising = pServer->getAdvertising();
//   pAdvertising->start();

//   pinMode(SENSOR_PIN, INPUT);
//   I2S.setAllPins(I2S_SCK, I2S_WS, I2S_SD, -1, -1);
//   while (!Serial) {
//     ; // wait for serial port to connect. Needed for native USB port only
//   }

//   // start I2S at 16 kHz with 32-bits per sample
//   if (!I2S.begin(I2S_PHILIPS_MODE, 16000, 32)) {
//     Serial.println("Failed to initialize I2S!");
//     while (1); // do nothing
//   }
// }

// void loop() {
//   // read a sample
//   int sample = I2S.read();

//   if ((sample == 0) || (sample == -1) ) {
//     return;
//   }
//   // convert to 18 bit signed
//   sample >>= 14;

//   if (sample == 0) {
//     return;
//   }

//   if (sample > threshold) {
//     counter++;
//   }
//   if (counter > last_counter && millis() > next_scan_time) {
//     last_counter = counter;
//     next_scan_time = millis() + wait_for_scan;
//     Serial.println("Baby is crying");
//     String message = "Baby is crying";
//     pCharacteristic->setValue(message.c_str());
//     pCharacteristic->notify(true);
//   }

//   val = digitalRead(SENSOR_PIN);
//   if(val == HIGH) {
//     move++;
//   }

//   if(move > last_move && millis() > next_move_time) {
//     last_move = move;
//     next_move_time = millis() + wait_for_move;
//     Serial.println("Baby is moving");
//     String message = "Baby is moving";
//     pCharacteristic->setValue(message.c_str());
//     pCharacteristic->notify(true);
//   }
// }