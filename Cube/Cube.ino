// Sample RFM69 sender/node sketch, with ACK and optional encryption, and Automatic Transmission Control
// Sends periodic messages of increasing length to gateway (id=1)
// It also looks for an onboard FLASH chip, if present
// **********************************************************************************
// Copyright Felix Rusu 2016, http://www.LowPowerLab.com/contact
// **********************************************************************************
// License
// **********************************************************************************
// This program is free software; you can redistribute it 
// and/or modify it under the terms of the GNU General    
// Public License as published by the Free Software       
// Foundation; either version 3 of the License, or        
// (at your option) any later version.                    
//                                                        
// This program is distributed in the hope that it will   
// be useful, but WITHOUT ANY WARRANTY; without even the  
// implied warranty of MERCHANTABILITY or FITNESS FOR A   
// PARTICULAR PURPOSE. See the GNU General Public        
// License for more details.                              
//                                                        
// Licence can be viewed at                               
// http://www.gnu.org/licenses/gpl-3.0.txt
//
// Please maintain this license information along with authorship
// and copyright notices in any redistribution of this code
// **********************************************************************************
#include "FastLED.h"       // FastLED library. Please use the latest development version.
#include <RFM69.h>         // get it here: https://www.github.com/lowpowerlab/rfm69
#include <SPIFlash.h>      // get it here: https://www.github.com/lowpowerlab/spiflash
#include <SPI.h>           // included with Arduino IDE install (www.arduino.cc)

// Cube-related stuff
#include "Palette.h"

//*********************************************************************************************
//************ IMPORTANT SETTINGS - YOU MUST CHANGE/CONFIGURE TO FIT YOUR HARDWARE ************
//*********************************************************************************************
#define NETWORKID     200  //the same on all nodes that talk to each other (range up to 255)
#define FREQUENCY     RF69_915MHZ //Match freq to the hardware version of radio on your Moteino
#define IS_RFM69HW_HCW  //uncomment only for RFM69HW/HCW! Leave out if you have RFM69W/CW!
//*********************************************************************************************
//************* PREVIOUSLY IMPORTANT - PROVIDED ELSEWHERE *************************************
//#define NODEID        2    //PROVIDED BY BUILD -- must be unique for each node on same network (range up to 254, 255 is used for broadcast)
//#define ENCRYPTKEY    "yougetmeYAAAY" //exactly the same 16 characters/bytes on all nodes! -- seems buggy

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

int TRANSMITPERIOD = 1000; //transmit a packet to gateway so often (in ms)
int16_t RSSITHRESHOLD = -40;
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)
RFM69 radio;

// FastLED/Neopixel constants
#define LED_DT 6            // Data pin to connect to the strip.
// #define LED_CK 11         // Clock pin for WS2801 or APA102.
#define COLOR_ORDER GRB     // It's GRB for WS2812 and BGR for APA102.
#define LED_TYPE WS2812     // Using APA102, WS2812, WS2801. Don't forget to change LEDS.addLeds.
#define NUM_LEDS 60         // Number of LED's.
struct CRGB leds[NUM_LEDS]; // Initialize our LED array.

// CUBE-SPECIFIC STUFF
const char PAYLOAD[] = "4xth]jWt"; // security through obscurity!
const uint8_t PAYLOAD_LEN = sizeof(PAYLOAD);
const uint8_t RSSI_WINDOW_LEN = 10;
typedef struct {
  int8_t nodeID;
  uint8_t ticks;

  // Track moving window signal strength
  int16_t rssis[RSSI_WINDOW_LEN];
  uint8_t rssiPtr;
} NodeRecord;
const uint8_t DEFAULT_TICKS = 20;
uint8_t NUM_NODES = 0;
NodeRecord XMIT[10]; // initialize to send to self at first; relies on promiscuousMode to pick up other nodes' broadcasts

CRGBPalette16 currentPalette;

// setup sets everything up! \o/
void setup() {
  Serial.begin(BAUD);
  delay(1000);          // Soft startup to ease the flow of electrons.

  FastLED.addLeds<NEOPIXEL, LED_DT>(leds, NUM_LEDS);
  FastLED.setBrightness(50);
  set_max_power_in_volts_and_milliamps(5, 100); // FastLED Power management set at 5V, 500mA.

  radio.initialize(FREQUENCY,NODEID,NETWORKID);
  #ifdef IS_RFM69HW_HCW
    radio.setHighPower(); //must include this only for RFM69HW/HCW!
  #endif
  radio.encrypt(null);
  radio.promiscuous(true);
  //radio.setFrequency(919000000); //set frequency to some custom frequency

  radio.setPowerLevel(0);
}

// handleDebug processes any serial input from within the loop() fn
void handleDebug() {
  if (Serial.available() <= 0) {
    return;
  }

  char input = Serial.read();
  if (input >= 48 && input <= 57) { //[0,9]
    TRANSMITPERIOD = 100 * (input-48);
    if (TRANSMITPERIOD == 0) TRANSMITPERIOD = 1000;
    Serial.print("\nChanging delay to ");
    Serial.print(TRANSMITPERIOD);
    Serial.println("ms\n");
  }
  if (input == 'j') {
    RSSITHRESHOLD--;
    Serial.print("\nRSSI threshold:");
    Serial.print(RSSITHRESHOLD);
    Serial.println(";\n");
  }
  if (input == 'k') {
    RSSITHRESHOLD++;
    Serial.print("\nRSSI threshold:");
    Serial.print(RSSITHRESHOLD);
    Serial.println(";\n");
  }
  if (input == 'r') { //d=dump register values
    radio.readAllRegs();
  }
  if (input == 'd') { //d=dump flash area
    Serial.println("Flash content:");
    uint16_t counter = 0;

    Serial.print("0-256: ");
    while(counter<=256){
      Serial.print(flash.readByte(counter++), HEX);
      Serial.print('.');
    }
    while(flash.busy());
    Serial.println();
  }
  if (input == 'e') {
    Serial.print("Erasing Flash chip ... ");
    flash.chipErase();
    while(flash.busy());
    Serial.println("DONE");
  }
  if (input == 'i') {
    Serial.print("Node ID: ");
    Serial.print(NODEID);
  }
  if (input == 't') {
    byte temperature =  radio.readTemperature(-1); // -1 = user cal factor, adjust for correct ambient
    byte fTemp = 1.8 * temperature + 32; // 9/5=1.8
    Serial.print( "Radio Temp is ");
    Serial.print(temperature);
    Serial.print("C, ");
    Serial.print(fTemp); //converting to F loses some resolution, obvious when C is on edge between 2 values (ie 26C=78F, 27C=80F)
    Serial.println('F');
  }
}

uint8_t compact(NodeRecord arr[], uint8_t numNodes) {
  uint8_t pruned = 0;
  for (uint8_t i = 0; i < numNodes; i++) {
    if (arr[i].ticks == 0) {
      pruned++;
      for (uint8_t j = i; j < NUM_NODES; j++) {
        arr[j-1] = arr[j];
      }
      arr[numNodes-1] = {-1,0};

      bool anyLeft = false;
      for (uint8_t j = 1; j < numNodes; j++) {
        if (arr[j].ticks > 0) {
          anyLeft = true;
          break;
        }
      }
      if (anyLeft) {
        i--; // we just compacted, check the same position
      }
    }
  }
  return pruned;
}

bool checkPayload(char radioPayload[]) {
  for (uint8_t i = 0; i < PAYLOAD_LEN; i++) {
    if (PAYLOAD[i] != radioPayload[i]) {
      return false;
    }
  }
  return true;
}

int16_t avgRssis(int16_t rssis[]) {
  long sum = 0;
  for (uint8_t i = 0; i < RSSI_WINDOW_LEN; i++) {
    sum += rssis[i];
  }
  return sum/RSSI_WINDOW_LEN;
}

long lastPeriod = 0;
void loop() {
  //process any serial input
  handleDebug();
  static uint8_t startIndex = 0;

  //check for any received packets
  if (radio.receiveDone()) {
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    Serial.print("   [RX_RSSI:");Serial.print(radio.RSSI, DEC);Serial.println("] ");
    if ((sizeof(PAYLOAD) == radio.DATALEN) &&
        checkPayload(radio.DATA)) {
      // Find XMIT record, if appropriate
      uint8_t idx = 0;
      for (;idx < NUM_NODES; idx++) {
        if (XMIT[idx].nodeID == radio.SENDERID) {
          break;
        }
      }
      // at this point idx will either be the correct index or equal to NUM_NODES
      if (idx == NUM_NODES) {
        XMIT[idx] = {
          radio.SENDERID,
          DEFAULT_TICKS,
          { radio.RSSI, -100, -100, -100, -100,
            -100, -100, -100, -100, -100 },
          1
        };
        NUM_NODES++;
      } else {
        XMIT[idx].rssis[XMIT[idx].rssiPtr] = radio.RSSI;
        XMIT[idx].rssiPtr++;
        XMIT[idx].rssiPtr %= RSSI_WINDOW_LEN;

        Serial.print("========== Ptr: ");
        Serial.print(XMIT[idx].rssiPtr);
        Serial.print(", Moving avg: ");
        Serial.println(avgRssis(XMIT[idx].rssis), DEC);
        if (avgRssis(XMIT[idx].rssis) > RSSITHRESHOLD) {
          // reset the number of ticks
          XMIT[idx].ticks = DEFAULT_TICKS;
        }
      }
    }
  }

  static unsigned long currPeriod;
  currPeriod = millis()/TRANSMITPERIOD;
  if (currPeriod != lastPeriod) {
    lastPeriod=currPeriod;

    if (NUM_NODES == 0) {
      Serial.println("No nodes to transmit to!");
    }
    radio.send(NODEID, PAYLOAD, PAYLOAD_LEN);

    // decay nodes and compact the list to remove nodes if we can!
    for (uint8_t i = 0; i < NUM_NODES; i++) {
      XMIT[i].ticks--;
    }
    NUM_NODES -= compact(XMIT, NUM_NODES);

    // Output diagnostics
    Serial.print("\n= XMIT NODES: ");
    for (uint8_t i = 0; i < NUM_NODES; i++) {
      Serial.print(XMIT[i].nodeID, DEC);
      Serial.print(":");
      Serial.print(XMIT[i].ticks, DEC);
      Serial.print(" ");
    }
    Serial.println(";\n");

    // So we know there are NUM_NODES other nodes and 1 of me, which gives us
    // (NUM_NODES+1) 4-item palettes to choose from to fill out a 16-idx palette
    static CHSV hsv;
    for (uint8_t i = 0; i < 16; i++) {
      if (i % (NUM_NODES + 1) == 0) {
        hsv = COLORS[NODEID];
      } else {
        hsv = COLORS[XMIT[i % NUM_NODES].nodeID];
      }
      hsv.sat = 155 + 100 * (float(i % 4) / 4);
      currentPalette[i] = hsv;
    }
  }

  EVERY_N_MILLISECONDS(10) {
    fill_palette(leds, // LEDs aka CRGB *
      NUM_LEDS,        // length of LEDs
      startIndex,      // startIndex - if constant, then motion will stay constant
      1, // higher = higher frequency
      currentPalette,
      255, // max brightness
      LINEARBLEND);
    // And now reflect the effects of whatever we received during the receive phase
    FastLED.show(); // Power managed display
    startIndex++;
  }
}
