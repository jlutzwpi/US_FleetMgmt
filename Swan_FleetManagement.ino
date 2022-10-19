//
// Copyright 2019 Blues Inc.  All rights reserved.
// Use of this source code is governed by licenses granted by the
// copyright holder including that found in the LICENSE file.
//
// This example contains the complete source for the Sensor Tutorial at dev.blues.io
// https://dev.blues.io/build/tutorials/sensor-tutorial/notecarrier-af/esp32/arduino-wiring/
//
// This tutorial requires an external Adafruit BME680 Sensor.
//

// Include the Arduino library for the Notecard
#include <Notecard.h>

#include <Wire.h>
#include <Arduino.h>
#include "I2CDriver.h"

//Adafruit_BME680 bmeSensor;

//#define serialNotecard Serial1
#define serialDebug Serial

#define productUID "xxxx"
Notecard notecard;
I2CDriver i2c = I2CDriver();
//results struct from the useful sensor
inference_results_t results;

// One-time Arduino initialization
void setup()
{
#ifdef serialDebug
    delay(2500);
    serialDebug.begin(115200);
    notecard.setDebugOutputStream(serialDebug);
#endif

    // Initialize the physical I/O channel to the Notecard
#ifdef serialNotecard
    notecard.begin(serialNotecard, 9600);
#else
    Wire.begin();

    notecard.begin();
#endif
    //set cellular to periodic update every 2 minutes
    //can't have GPS and cell be continous at same time
    J *req = notecard.newRequest("hub.set");
    JAddStringToObject(req, "product", productUID);
    JAddStringToObject(req, "mode", "periodic");
    JAddNumberToObject(req, "outbound", 3);
    JAddNumberToObject(req, "inbound", 720);
    notecard.sendRequest(req);

    //set GPS to continuous
    req = notecard.newRequest("card.location.mode");
    JAddStringToObject(req, "mode", "periodic");
    JAddNumberToObject(req, "seconds", 15);
    notecard.sendRequest(req);
  
    req = notecard.newRequest("card.location.track");
    JAddBoolToObject(req, "sync", true);
    JAddBoolToObject(req, "heartbeat", true);
    //JAddNumberToObject(req, "hours", 1);
    notecard.sendRequest(req);

    i2c.begin();
    i2c.setMode(i2c.MODE_CONTINUOUS);
    i2c.setIdModelEnabled(true);
    i2c.setDebugMode(true);
    i2c.setPersistentIds(false);

    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    //get results from sensor
    results = i2c.read();
    if(results.num_faces == 0)
      digitalWrite(LED_BUILTIN, LOW);
    for (int i=0; i< results.num_faces; i++) {
      uint8_t confidence = results.boxes[i].confidence;
      if(confidence > 99) confidence = 99;
      //face ID'd
      if(confidence > 70)
      {
        digitalWrite(LED_BUILTIN, HIGH); 
        uint8_t id_confidence = results.boxes[i].id_confidence;
        int8_t id = results.boxes[i].id;
        if (id >= 0) {
          //send to notecard
          J *req = notecard.newRequest("note.add");
          if (req != NULL) {
              JAddStringToObject(req, "file", "sensors.qo");
              JAddBoolToObject(req, "sync", true);
      
              J *body = JCreateObject();
              if (body != NULL) {
                  JAddNumberToObject(body, "ID", id);
                  JAddNumberToObject(body, "Confidence", confidence);
                  JAddNumberToObject(body, "Num_Faces", results.num_faces);
                  JAddItemToObject(req, "body", body);
              }
      
              notecard.sendRequest(req);
              delay(20000);
          }
        }
      }
      else {
        digitalWrite(LED_BUILTIN, LOW); 
      }
    }
}
