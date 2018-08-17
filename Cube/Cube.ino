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
#include <RFM69_ATC.h>     // get it here: https://www.github.com/lowpowerlab/rfm69
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
//*********************************************************************************************
//Auto Transmission Control - dials down transmit power to save battery
//Usually you do not need to always transmit at max output power
//By reducing TX power even a little you save a significant amount of battery power
//This setting enables this gateway to work with remote nodes that have ATC enabled to
//dial their power down to only the required level (ATC_RSSI)
#define ENABLE_ATC    //comment out this line to disable AUTO TRANSMISSION CONTROL
#define ATC_RSSI      -80
//*********************************************************************************************

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

int TRANSMITPERIOD = 500; //transmit a packet to gateway so often (in ms)
SPIFlash flash(FLASH_SS, 0xEF30); //EF30 for 4mbit  Windbond chip (W25X40CL)

#ifdef ENABLE_ATC
  RFM69_ATC radio;
#else
  RFM69 radio;
#endif

// FastLED/Neopixel constants
#define LED_DT 6            // Data pin to connect to the strip.
// #define LED_CK 11         // Clock pin for WS2801 or APA102.
#define COLOR_ORDER GRB     // It's GRB for WS2812 and BGR for APA102.
#define LED_TYPE WS2812     // Using APA102, WS2812, WS2801. Don't forget to change LEDS.addLeds.
#define NUM_LEDS 20          // Number of LED's.
struct CRGB leds[NUM_LEDS]; // Initialize our LED array.

// CUBE-SPECIFIC STUFF
const char PAYLOAD[] = "."; // it really doesn't matter what the actual payload is, we're all just broadcasting into the ether
typedef struct {
  int8_t nodeID;
  uint8_t ticks;
} NodeRecord;
uint8_t DEFAULT_TICKS = 10;
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

  //Auto Transmission Control - dials down transmit power to save battery (-100 is the noise floor, -90 is still pretty good)
  //For indoor nodes that are pretty static and at pretty stable temperatures (like a MotionMote) -90dBm is quite safe
  //For more variable nodes that can expect to move or experience larger temp drifts a lower margin like -70 to -80 would probably be better
  //Always test your ATC mote in the edge cases in your own environment to ensure ATC will perform as you expect
  #ifdef ENABLE_ATC
    Serial.println("RFM69_ATC Enabled (Auto Transmission Control)\n");
    radio.enableAutoPower(ATC_RSSI);
  #endif

  char buff[50];
  sprintf(buff, "\nTransmitting at %d Mhz...", FREQUENCY==RF69_433MHZ ? 433 : FREQUENCY==RF69_868MHZ ? 868 : 915);
  Serial.println(buff);
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

long lastPeriod = 0;
bool success = false;
bool gotData = false;
void loop() {
  //process any serial input
  handleDebug();
  static uint8_t startIndex = 0;

  static unsigned long currPeriod;
  currPeriod = millis()/TRANSMITPERIOD;
  if (currPeriod != lastPeriod) {
    lastPeriod=currPeriod;

    radio.sendWithRetry(NODEID, PAYLOAD, 1);
    gotData = false;
  }

  //check for any received packets
  if (radio.receiveDone()) {
    gotData = true;
    Serial.print('[');Serial.print(radio.SENDERID, DEC);Serial.print("] ");
    for (byte i = 0; i < radio.DATALEN; i++)
      Serial.print((char)radio.DATA[i]);
    Serial.print("   [RX_RSSI:");Serial.print(radio.readRSSI());Serial.println("]");

    // Find XMIT record, if appropriate
    uint8_t idx = 0;
    for (;idx < NUM_NODES; idx++) {
      if (XMIT[idx].nodeID == radio.SENDERID) {
        break;
      }
    }
    // at this point idx will either be the correct index or equal to NUM_NODES
    if (idx == NUM_NODES) {
      XMIT[idx] = { radio.SENDERID, DEFAULT_TICKS };
      NUM_NODES++;
    } else {
      // reset the number of ticks
      XMIT[idx].ticks = DEFAULT_TICKS;
    }

    // Output diagnostics
    Serial.print("XMIT NODES: ");
    for (uint8_t i = 0; i < NUM_NODES; i++) {
      Serial.print(XMIT[i].nodeID, DEC);
      Serial.print(":");
      Serial.print(XMIT[i].ticks, DEC);
      Serial.print(" ");
    }
    Serial.println(";");

    if (radio.ACKRequested()) {
      radio.sendACK();
      Serial.print(" - ACK sent to ");
      Serial.println(radio.SENDERID, DEC);
    }
    Serial.println();
  }

  for (uint8_t i = 0; i < 10; i++) {
    if (gotData) {
      leds[i] = CRGB::Green;
    } else {
      leds[i] = CRGB::Red;
    }
  }
  // And now reflect the effects of whatever we received during the receive phase
  FastLED.show(); // Power managed display
  FastLED.delay(1000/100); // update 100 times a second, apparently
  startIndex++;
}

void FillLEDsFromPaletteColors(uint8_t colorIndex) {
  for(uint8_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = ColorFromPalette(currentPalette, colorIndex, 255, LINEARBLEND);
    colorIndex += 1;
  }
}