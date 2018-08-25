// CUBES
#define NODEID        2    //PROVIDED BY BUILD -- must be uniquefor each node on same network (range up to 254, 255 is used for broadcast)

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

#ifdef __AVR_ATmega1284P__
  #define LED           15 // Moteino MEGAs have LEDs on D15
  #define FLASH_SS      23 // and FLASH SS on D23
#else
  #define LED           9 // Moteinos have LEDs on D9
  #define FLASH_SS      8 // and FLASH SS on D8
#endif

int TRANSMITPERIOD = 500; //transmit a packet to gateway so often (in ms)
int16_t RSSITHRESHOLD = -50;
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

// NodeRecord tracks a Node that we've received a message from.
typedef struct {
  // unique identifier for the Node
  int8_t nodeID;
  // ticks are the lifeblood of "include me in your color scheme!"
  // once a NodeRecord runs out of ticks (which are decremented each
  // TRANSMITPERIOD), it'll stop contributing to the color scheme.
  uint8_t ticks;

  // rssis + rssiPtr allow us to track a moving average of the signal strengths
  // received by this Node from the NodeRecord.nodeID.
  int16_t rssis[RSSI_WINDOW_LEN];
  uint8_t rssiPtr;
} NodeRecord;
const uint8_t DEFAULT_TICKS = 10;
uint8_t NUM_NODES = 0;
// we rely on promiscuousMode to pick up other nodes' broadcasts, but we keep
// track of which nodes we've seen (in order to maybe impact our colors)
NodeRecord XMIT[10];

// currentPalette stores the **current state of the LEDs**
CRGBPalette16 currentPalette(COLORS[NODEID]);
// targetPalette represents the palette we're blending towards. At steady
// state, currentPalette and targetPalette are identical
CRGBPalette16 targetPalette;

void setup() {
  Serial.begin(57600);
  while (!Serial) {
    ; // wait for serial port to connect. Needed for native USB port only
  }
  delay(1000);          // Soft startup to ease the flow of electrons.

  FastLED.addLeds<NEOPIXEL, LED_DT>(leds, NUM_LEDS);
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

// setPaletteColor helps us make sure we have a bit of texture for the many
// solid colors we fetched
//
// It also offers a guardrail against IndexOutOfRange errors since we call this
// in weird spots in a loop
void setPaletteColor(uint8_t ledIdx, uint8_t nodeIdx) {
  if (ledIdx >= 16) {
    return;
  }
  CHSV hsv = COLORS[nodeIdx];
  hsv.sat = 155 + 100 * (float(ledIdx % 4) / 4);
  targetPalette[ledIdx] = hsv;
}

// updateCurrentPalette takes a look at the NodeRecords we know about, and for
// the ones with .ticks > 0, updates our palette to include colors from that
// Node.
void updateCurrentPalette() {
  // Output diagnostics
  Serial.print("\n= XMIT NODES: ");
  for (uint8_t i = 0; i < NUM_NODES; i++) {
    Serial.print(XMIT[i].nodeID, DEC);
    Serial.print(":");
    Serial.print(XMIT[i].ticks, DEC);
    Serial.print(" ");
  }
  Serial.println(";\n");

  uint8_t numOtherColors = 0;
  for (uint8_t i = 0; i < NUM_NODES; i++) {
    if (XMIT[i].ticks > 0) {
      numOtherColors++;
    }
  }

  // Update target palette based on the Nodes (this may be a duplicate, who
  // knows)
  uint8_t i = 0;
  uint8_t nodeIdx = 0;
  if (numOtherColors == 0) {
    targetPalette = CRGBPalette16(COLORS[NODEID]);
  } else {
    while (i < 16) {
      // just start off with the NODEID color and essentially kick off another
      // iteration of the loop
      if (nodeIdx % numOtherColors == 0) {
        setPaletteColor(i, NODEID);
        setPaletteColor(i + 1, NODEID);
        i += 2;
      }

      // find the next XMIT node with a legit number of ticks
      while (XMIT[nodeIdx % NUM_NODES].ticks <= 0) {
        nodeIdx++;
      }

      setPaletteColor(i, XMIT[nodeIdx % NUM_NODES].nodeID);
      setPaletteColor(i + 1, XMIT[nodeIdx % NUM_NODES].nodeID);
      i += 2;
      nodeIdx++;
    }
  }

  // Crossfade current palette slowly toward the target palette
  //
  // Each time that nblendPaletteTowardPalette is called, small changes
  // are made to currentPalette to bring it closer to matching targetPalette.
  // You can control how many changes are made in each call:
  //   - meaningful values are 1-48.  1=veeeeeeeery slow, 48=quickest
  for (uint8_t i = 0; i < 10; i++) {
    nblendPaletteTowardPalette(currentPalette, targetPalette, 48);
  }
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
          0,
          { radio.RSSI, -100, -100, -100, -100,
            -100, -100, -100, -100, -100 },
          1
        };
        NUM_NODES++;
      } else {
        XMIT[idx].rssis[XMIT[idx].rssiPtr] = radio.RSSI;
        XMIT[idx].rssiPtr++;
        XMIT[idx].rssiPtr %= RSSI_WINDOW_LEN;

        if (avgRssis(XMIT[idx].rssis) > RSSITHRESHOLD) {
          Serial.print("Node over threshold! ");
          Serial.println(XMIT[idx].nodeID, DEC);
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

    // decay nodes and renew ones we've been seeing strongly
    for (uint8_t i = 0; i < NUM_NODES; i++) {
      if (XMIT[i].ticks > 0) {
        XMIT[i].ticks--;
      }
    }

    updateCurrentPalette();
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
