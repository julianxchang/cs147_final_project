#include <Arduino.h>
#include <HttpClient.h>
#include <WiFi.h>
#include <inttypes.h>
#include <stdio.h>
#include <Wire.h>
#include <I2S.h>
#include <ctime>
#include <algorithm>
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs.h"
#include "nvs_flash.h"
#define I2S_WS 22
#define I2S_SD 17
#define I2S_SCK 21
#define SENSOR_PIN 32
#define SAMPLES 256

const unsigned long wait_for_scan = 1000;
unsigned long next_scan_time = 0;
const unsigned long wait_for_move = 11000;
unsigned long next_move_time = 0;
const unsigned long delayed_scan_time = 5000;
unsigned long delayed_time = 0;

int threshold = -10000;
int counter = 0;
int last_counter = 0;
int val = 0;
int last_move = 0;
int move = 0;


// This example downloads the URL " http://arduino.cc/"
char ssid[50]; // your network SSID (name)
char pass[50]; // your network password (use for WPA, or use
// Number of milliseconds to wait without receiving any data before we give up
const int kNetworkTimeout = 30 * 1000;
// Number of milliseconds to wait if no data is available before trying again
const int kNetworkDelay = 1000;

void nvs_access() {
  // Initialize NVS
  esp_err_t err = nvs_flash_init();
  if (err == ESP_ERR_NVS_NO_FREE_PAGES ||
    err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
    // NVS partition was truncated and needs to be erased
    // Retry nvs_flash_init
    ESP_ERROR_CHECK(nvs_flash_erase());
    err = nvs_flash_init();
  }
  ESP_ERROR_CHECK(err);
  // Open
  Serial.printf("\n");
  Serial.printf("Opening Non-Volatile Storage (NVS) handle... ");
  nvs_handle_t my_handle;
  err = nvs_open("storage", NVS_READWRITE, & my_handle);
  if (err != ESP_OK) {
    Serial.printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
  } else {
    Serial.printf("Done\n");
    Serial.printf("Retrieving SSID/PASSWD\n");
    size_t ssid_len;
    size_t pass_len;
    err = nvs_get_str(my_handle, "ssid", ssid, & ssid_len);
    err |= nvs_get_str(my_handle, "pass", pass, & pass_len);
    switch (err) {
    case ESP_OK:
      Serial.printf("Done\n");
      //Serial.printf("SSID = %s\n", ssid);
      //Serial.printf("PASSWD = %s\n", pass);
      break;
    case ESP_ERR_NVS_NOT_FOUND:
      Serial.printf("The value is not initialized yet!\n");
      break;
    default:
      Serial.printf("Error (%s) reading!\n", esp_err_to_name(err));
    }
  }
  // Close
  nvs_close(my_handle);
}





void update_cloud(int sample, int val) {
  int err = 0;
  WiFiClient c;
  HttpClient http(c);
  
  // Get current time
  time_t now;
  time(&now);
  String timeStr = String(ctime(&now));
  timeStr = timeStr.substring(11, 19);
  timeStr.replace(" ", "%20");
  err = http.get("3.21.231.47", 5000, ("/?var=csv" + timeStr + String(sample) + String(val)).c_str(), NULL);
  if (err == 0) {
    Serial.println("\n\n");
    Serial.println("startedRequest ok");
    err = http.responseStatusCode();
    if (err >= 0) {
      Serial.print("Got status code: "); 
      Serial.println(err);
      // Usually you'd check that the response code is 200 or a
      // similar "success" code (200-299) before carrying on,
      // but we'll print out whatever response we get
      err = http.skipResponseHeaders();
      if (err >= 0) {
        int bodyLen = http.contentLength();
        Serial.print("Content length is: ");
        Serial.println(bodyLen);
        Serial.println();
        Serial.println("Body returned follows:");
        // Now we've got to the body, so we can print it out
        unsigned long timeoutStart = millis();
        char c;
        // Whilst we haven't timed out & haven't reached the end of the body
        while ((http.connected() || http.available()) &&
          ((millis() - timeoutStart) < kNetworkTimeout)) {
          if (http.available()) {
            c = http.read();
            // Print out this character
            Serial.print(c);
            bodyLen--;
            // We read something, reset the timeout counter
            timeoutStart = millis();
          } else {
            // We haven't got any data, so let's pause to allow some to
            // arrive
            delay(kNetworkDelay);
          }
        }
      } else {
        Serial.print("Failed to skip response headers: ");
        Serial.println(err);
      }
    } else {
      Serial.print("Getting response failed: ");
      Serial.println(err);
    }
  } else {
    Serial.print("Connect failed: ");
    Serial.println(err);
  }
  http.stop();
  // And just stop, now that we've tried a download
  // while (1)
  // ;
}



void setup() {
  // Open serial communications and wait for port to open:
  // A baud rate of 115200 is used instead of 9600 for a faster data rate
  // on non-native USB ports
  Serial.begin(115200);
  Wire.begin();
  nvs_access();
  // We start by connecting to a WiFi network
  delay(1000);
  Serial.println();
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println("MAC address: ");
  Serial.println(WiFi.macAddress());

  // Configure NTP
  configTime(-28800, 0, "pool.ntp.org", "time.nist.gov");  // Use multiple NTP servers
  
  Serial.println("Waiting for NTP time sync...");
  while (time(nullptr) < 1000000000) {  // Wait until time is synchronized
    delay(500);
    Serial.print(".");
  }
  Serial.println();
  
  // Now get and print the time
  time_t now;
  time(&now);
  String timeStr = String(ctime(&now));
  timeStr.trim();
  timeStr = timeStr.substring(11, 19);  // Remove trailing newline
  Serial.println("Current time: ");
  Serial.println(timeStr);

  delayed_time = millis() + delayed_scan_time;

  pinMode(SENSOR_PIN, INPUT);
  I2S.setAllPins(I2S_SCK, I2S_WS, I2S_SD, -1, -1);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }

  // start I2S at 16 kHz with 32-bits per sample
  if (!I2S.begin(I2S_PHILIPS_MODE, 16000, 32)) {
    Serial.println("Failed to initialize I2S!");
    while (1); // do nothing
  }
}

void loop() {
  int samples[SAMPLES];
  int sample = 0;
  for (int i=0; i<SAMPLES; i++) {
    while ((sample == 0) || (sample == -1) ) {
      sample = I2S.read();
    }
    sample >>= 14; 
    samples[i] = sample;
  }

  float meanval = 0;
  for (int i=0; i<SAMPLES; i++) {
    meanval += samples[i];
  }
  meanval /= SAMPLES;

  for (int i=0; i<SAMPLES; i++) {
    samples[i] -= meanval;
  }

  float maxsample, minsample;
  minsample = 100000;
  maxsample = -100000;
  for (int i=0; i<SAMPLES; i++) {
    minsample = std::min(minsample, static_cast<float>(samples[i]));
    maxsample = std::max(maxsample, static_cast<float>(samples[i]));
  }
  sample = maxsample - minsample;
  if (sample > threshold) {
    counter++;
  }

  // if (counter > last_counter && millis() > next_scan_time) {
  //   last_counter = counter;
  //   next_scan_time = millis() + wait_for_scan;
  //   Serial.println("Baby is crying");
  //   String message = "Baby is crying";
  // }

  val = digitalRead(SENSOR_PIN);
  if(val == HIGH) {
    move++;
  }

  // if(move > last_move && millis() > next_move_time) {
  //   last_move = move;
  //   next_move_time = millis() + wait_for_move;
  //   Serial.println("Baby is moving");
  //   String message = "Baby is moving";
  // }

  if (millis() > next_scan_time && millis() > delayed_time) {
    update_cloud(sample, val);
    Serial.println("Sample sent");
    next_scan_time = millis() + wait_for_scan;
  }
}