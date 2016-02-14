#include <RS485_non_blocking.h>
#include "ClickButton.h"
#include <EEPROM.h>

// EPROM memory location where each device holds it's own ID
#define DEVICE_ID 0

#define DST_BROADCAST 255

#define TURN_ALL_LIGHT_ON  1
#define TURN_ALL_LIGHT_OFF 2
#define TURN_ON_A1_L1      3
#define TURN_OFF_A1_L1     4
#define REED_DEBOUNCE        100
#define REED_AT_CHECKDELAY   1000

// ---------- Arduino/Button/Relay mapping ---------------------
// A1, A2 => Arduinos
// BTN_P1, BTN_P2, BTN_P3, BTN_P4 => Push-buttons
// RLY_40A => Relay 40A, stand-alone
// RLY_8M1, RLY_8M2 => first and second port of the 8-channel Relay module
// RLY_4M1, RLY_4M2 => first and second port of the 4-channel Relay module
//
// A1: first Mega 2560
const int A1_BTN_P3_Pin = 40; 
const int A1_BTN_P4_Pin = 44;
const int A1_RLY_40A_Pin = 24; int A1_RLY_40A_State = 0;
const int A1_RLY_8M1_Pin = 28; int A1_RLY_8M1_State = 0;
const int A1_RLY_4M1_Pin = 32; int A1_RLY_4M1_State= 0;

//
// A2: second Mega 2560
const int A2_BTN_P1_Pin = 42; 
const int A2_BTN_P2_Pin = 46;
const int A2_RLY_8M2_Pin = 10; int A2_RLY_8M2_State = 0;
const int A2_RLY_4M2_Pin = 8; int A2_RLY_4M2_State = 0;
const int REED1_AT_Pin = 50;  // REED1 sensor - AntiTamper - always CLOSE
const int REED1_SW_Pin = 52;  // REED1 sensor - Switch  - OPEN/CLOSE


// ---------- End of Arduino/Button/Relay mapping --------------

// Buttons object definitions
ClickButton BTN_P1(A2_BTN_P1_Pin, LOW, CLICKBTN_PULLUP);
ClickButton BTN_P2(A2_BTN_P2_Pin, LOW, CLICKBTN_PULLUP);
ClickButton BTN_P3(A1_BTN_P3_Pin, LOW, CLICKBTN_PULLUP);
ClickButton BTN_P4(A1_BTN_P4_Pin, LOW, CLICKBTN_PULLUP);
// ...with also some "temp" variables
int BTN_P1_clicks = 0;
int BTN_P2_clicks = 0;
int BTN_P3_clicks = 0;
int BTN_P4_clicks = 0;

int thisdev = -1; // to hold the ID of running device
int count = 1;

//timestamps, needed for various "delay" check
unsigned long ts;
unsigned long last_reed_sw1_read; // REED read delay
unsigned long last_reed_sw1_debounce; // REED read debounce
int reed_sw1_state;
int reed_sw1_previous_state;

unsigned long reed_at1_lastcheck;
int reed_at1_state;

// ----------------------------------------------------------------------
// following "fWrite", "fAvalable" and "fRead" functions are called
// by RS485 module, as callback routines for related operations
size_t fWrite (const byte what)
  {
  Serial2.write (what);  

  // following "flush" is _REALLY_ important!
  // You run the risk to loose data on TX without it, IF YOU USE HARDWARE SERIAL!
  // See:
  // - http://www.gammon.com.au/forum/?id=11428 => "Flushing the output" 
  // - https://www.baldengineer.com/when-do-you-use-the-arduinos-to-use-serial-flush.html
  // - https://www.arduino.cc/en/Serial/Flush
  Serial2.flush();
  }
  
int fAvailable ()
  {
  return Serial2.available ();  
  }

int fRead ()
  {
  return Serial2.read ();  
  }
// --------- End of callback definition -----------------------------------

// in all our arduino, PIN 3 is used as ENABLE_PIN
// Please note that due to RS485 half-duplex design, ENABLE_PIN
// is needed, regardless of Hardware/Software serial involvement,
// to switch between RECEIVE_MODE (ENABLE_PIN = LOW) and
// SENDING_MODE (ENABLE_PIN = HIGH)
const byte ENABLE_PIN = 3;

// PIN13 is used as a visual-feedback for:
// - MASTER: when sending "messages" over the RS485 bus
// - SLAVES: when a "messages" has been received from the RS485 bus
const byte LED_PIN = 25;

// Let's instantiate the object (rs485bus) to deal with the RS485 bus
// Note: we're using 3 char-long "messages", so we declare such a size
//       in addition to callbacks
RS485 myRS485_bus (fRead, fAvailable, fWrite, 3);

// This is the structure of the "message" we're sending over the bus. 
// It's MANDATORY to have a FIXED lenght! In our case, 3 bytes!
typedef struct {
    byte src_device;
    byte dst_device;
    byte command;
}  message_t;

message_t rcvd_message;
message_t snd_message;

// this prototype definition is required in order to avoid complains
// by the Arduino compiler (...unable to properly pre-process the code, otherwise)
void send_message (message_t *msg);

void send_message (message_t *msg) {
    // Let's debug, in the console, that a "message" is ready to be sent over the RS485 bus
    Serial.print(millis());Serial.print(" - Sending message: ["); Serial.print(count++); Serial.println("]");
    Serial.print("src_device=["); Serial.print(msg->src_device); Serial.println("]");
    Serial.print("dst_device=["); Serial.print(msg->dst_device); Serial.println("]");
    Serial.print("   command=["); Serial.print(msg->command); Serial.println("]");
    Serial.println("--------------------------------------------------");
    
    // send the message over the RS485 bus
    digitalWrite (LED_PIN,HIGH);      // let's turn on our visual-feedback
    digitalWrite (ENABLE_PIN, HIGH);  // let's tell our RS485 brick to switch to SEND mode
    myRS485_bus.sendMsg ((byte *) msg, sizeof(*msg));  // send the message
    digitalWrite (ENABLE_PIN, LOW);  // disable sending
    digitalWrite (LED_PIN, LOW);  // turn-off visual-feedback  
}

void setup()
{
  // this is my main "output/debug console"
  Serial.begin (9600);
  
  // Let's check, in the EEPROM, which is my ID
  // BTW:
  //  - ID: 1   => First MEGA 2560
  //  - ID: 2   => Second MEGA 2560
  thisdev = EEPROM.read(DEVICE_ID);

  // some "debug" message...
  Serial.print("Starting on [");
  Serial.print(thisdev);
  Serial.println("]");

  // this is my RS485 bus
  Serial2.begin (300);
  myRS485_bus.begin();  // this is required by the RS485 library
  
  Serial.print("Setting up RS485 PINs...");
  pinMode (ENABLE_PIN, OUTPUT);  // driver output enable
  pinMode (LED_PIN, OUTPUT);  // built-in LED

  switch (thisdev) {
    case 1:
      // Let's setup relay PINs as OUTPUT
      Serial.println("Initializing A1 OUTPUT PINs...");
      pinMode(A1_RLY_40A_Pin, OUTPUT);  
      pinMode(A1_RLY_8M1_Pin, OUTPUT);
      pinMode(A1_RLY_4M1_Pin, OUTPUT);
      //
      Serial.println("Initializing A1 BUTTONs...");
      // Setup button timers (all in milliseconds / ms)
      // <button>.debounceTime   => Debounce timer in ms
      // <button>.multiclickTime => Time limit for multi clicks
      // <button>.longClickTime  => time until "held-down clicks" register
      BTN_P3.debounceTime   = 20;
      BTN_P3.multiclickTime = 350;
      BTN_P3.longClickTime  = 1200;
      //
      BTN_P4.debounceTime   = 20;
      BTN_P4.multiclickTime = 350;
      BTN_P4.longClickTime  = 1200;
      break;
    case 2:
      // Let's setup relay PINs as OUTPUT
      Serial.println("Initializing A2 OUTPUT PINs...");
      pinMode(A2_RLY_8M2_Pin, OUTPUT);
      pinMode(A2_RLY_4M2_Pin, OUTPUT);

      // Let's setup relay PINs as OUTPUT
      Serial.println("Initializing A2 REED SWITCH PINs...");
      pinMode(REED1_SW_Pin, INPUT_PULLUP);
      pinMode(REED1_AT_Pin, INPUT_PULLUP);

      //
      Serial.println("Initializing A2 BUTTONs...");
      // Setup button timers (all in milliseconds / ms)
      // <button>.debounceTime   => Debounce timer in ms
      // <button>.multiclickTime => Time limit for multi clicks
      // <button>.longClickTime  => time until "held-down clicks" register
      BTN_P1.debounceTime   = 20;
      BTN_P1.multiclickTime = 350;
      BTN_P1.longClickTime  = 1200;
      //
      BTN_P2.debounceTime   = 20;
      BTN_P2.multiclickTime = 350;
      BTN_P2.longClickTime  = 1200;
      break;
    default: 
      // if nothing else matches, do the default
      // default is optional
      Serial.print("Hei! You should _NEVER_ get here! [");
      Serial.print(thisdev);
      Serial.println("]");
      break;
  }

  ts=millis();
}


void loop()
{
//  if (millis()-ts >=10000) {
//    Serial.print(millis());Serial.print("[KL] - Keepalive: [");Serial.print(myRS485_bus.getLength ()); Serial.println("]");
//    ts=millis();
//
//    Serial.println("[KL] RS485 bus status:");
//    if (myRS485_bus.available()) {
//      Serial.println("[KL]    - Bus AVAILABLE");
//    } else {
//      Serial.println("[KL]    - Bus NOT AVAILABLE");
//    }
//    if (myRS485_bus.isPacketStarted()) {
//      Serial.println("[KL]    - something ARRIVED!");
//    } else {
//      Serial.println("[KL]    - still nothing came in!");
//    }
//    Serial.print("[KL]    - length...: [");Serial.print(myRS485_bus.getLength());Serial.println("]");
//    Serial.print("[KL]    - errors...: [");Serial.print(myRS485_bus.getErrorCount());Serial.println("]");
//    Serial.print("[KL]    - starttime: [");Serial.print(myRS485_bus.getPacketStartTime());Serial.println("]");
//  }
  // let's check if a "message" have been received
  // BTW: "update" will return TRUE when the "message" is ready 
  if (myRS485_bus.update ()) {
      //
      // let's turn-on visual feedback
      digitalWrite (LED_PIN,HIGH);
      //
      // let's destroy previous "message", by zero-ing all its bytes
      memset (&rcvd_message, 0, sizeof rcvd_message);
      //
      // let's check the size of our message
      int len = myRS485_bus.getLength ();
      //
      // debug some info on console
      Serial.print("Received ["); Serial.print(len); Serial.println("] bytes. Let's see...");
      //
      // let's ensure to not overflow our "message" structure
      if (len > sizeof rcvd_message) len = sizeof rcvd_message;
      //
      // ok. So let's get our RS485 data and copy them on our "message" data structure
      memcpy (&rcvd_message, myRS485_bus.getData (), len);
      //
      // let's check if message has been sent to me, or not (or is a broadcast)
      if ((rcvd_message.dst_device == thisdev) || (rcvd_message.dst_device == DST_BROADCAST)) {
        // let's debug some info over the console
        Serial.print("  src_dev=["); Serial.print(rcvd_message.src_device); Serial.println("]");
        if (rcvd_message.dst_device == thisdev) {
          Serial.print("  dst_dev=["); Serial.print(rcvd_message.dst_device); Serial.println("]");
        } else {
          Serial.println("  dst_dev= ***BROADCAST***");            
        }
        Serial.print("  command=["); Serial.print(rcvd_message.command); Serial.println("]");
        Serial.println("--------------------------------------------------");

        
        // let's process the command
        switch (thisdev) {
          case 1:
              if (rcvd_message.command == TURN_ALL_LIGHT_ON) {
                 Serial.println("REMOTE TURN_ALL_LIGHT_ON [L1+L2 on]");
                 A1_RLY_8M1_State = HIGH;
                 A1_RLY_4M1_State = HIGH;
              } else if (rcvd_message.command == TURN_ALL_LIGHT_OFF) {
                 Serial.println("REMOTE TURN_ALL_LIGHT_OFF [L1+L2 off]");
                 A1_RLY_8M1_State = LOW;
                 A1_RLY_4M1_State = LOW;
              } else if (rcvd_message.command == TURN_ON_A1_L1) {
                 A1_RLY_8M1_State = HIGH;
              } else if (rcvd_message.command == TURN_OFF_A1_L1) {
                 A1_RLY_8M1_State = LOW;
              }
              break;
          case 2:
            if (rcvd_message.command == TURN_ALL_LIGHT_ON) {
               Serial.println("REMOTE TURN_ALL_LIGHT_ON [L3+L4 on]");
               A2_RLY_8M2_State = HIGH;
               A2_RLY_4M2_State = HIGH;
            } else if (rcvd_message.command == TURN_ALL_LIGHT_OFF) {
               Serial.println("REMOTE TURN_ALL_LIGHT_OFF [L3+L4 off]");
               A2_RLY_8M2_State = LOW;
               A2_RLY_4M2_State = LOW;
            }
            break;
        }
      } else {
        Serial.print("  NOT MINE => dst_dev=["); Serial.print(rcvd_message.dst_device); Serial.println("]");
      }
      // let's turn-off visual-feedback
      digitalWrite (LED_PIN, LOW);  
  }
  
  switch (thisdev) {
    case 1:
      // Update button state
      BTN_P3.Update();
      BTN_P4.Update();
      //
      // Save clicks count, as click codes are reset at next Update()
      if (BTN_P3.clicks != 0) {
        BTN_P3_clicks = BTN_P3.clicks;
        Serial.print("BTN_P3 has been CLICKSed...(");Serial.print(BTN_P3_clicks);Serial.println(")");
      }
      if (BTN_P4.clicks != 0) {
        BTN_P4_clicks = BTN_P4.clicks;
        Serial.print("BTN_P4 has been CLICKSed...(");Serial.print(BTN_P4_clicks);Serial.println(")");
      }
      //
      // Simply toggle relay on single clicks
      // (Cant use LEDfunction like the others here,
      //  as it would toggle on and off all the time)
      if(BTN_P3.clicks == 1) {
        Serial.println("BTN_P3 has been single-pressed [L1 switch]");
        A1_RLY_8M1_State = !A1_RLY_8M1_State;
      }
      if(BTN_P4.clicks == 1) {
        Serial.println("BTN_P4 has been single-pressed [L2 switch]");
        A1_RLY_4M1_State = !A1_RLY_4M1_State;
      }
      // Long press on P3 will turn all lights on
      if(BTN_P3.clicks <= -1) {
        Serial.println("BTN_P3 has been long-pressed [L1+L2 on]");
        A1_RLY_8M1_State = HIGH;
        A1_RLY_4M1_State = HIGH;

        // Let me prepare the "message" to send over the RS485 bus...
        //
        // let's destroy previous "snd_message", by zero-ing all its bytes
        memset (&snd_message, 0, sizeof snd_message);
        snd_message.src_device = thisdev;        // it's "my" message
        snd_message.dst_device = 255;            // broadcast
        snd_message.command = TURN_ALL_LIGHT_ON; // it carries some command code
        // OK, let's send...
        send_message(&snd_message);
      }
      // Long press on P4 will turn all lights off
      if(BTN_P4.clicks <= -1) {
        Serial.println("BTN_P4 has been long-pressed [L1+L2 off]");
        A1_RLY_8M1_State = LOW;
        A1_RLY_4M1_State = LOW;

        // Let me prepare the "message" to send over the RS485 bus...
        //
        // let's destroy previous "snd_message", by zero-ing all its bytes
        memset (&snd_message, 0, sizeof snd_message);
        snd_message.src_device = thisdev;        // it's "my" message
        snd_message.dst_device = 255;            // broadcast
        snd_message.command = TURN_ALL_LIGHT_OFF; // it carries some command code
        // OK, let's send...
        send_message(&snd_message);      
      }
      if(BTN_P3.clicks == 2) {
        Serial.println("BTN_P3 has been double-pressed [40A switch]");
        A1_RLY_40A_State = !A1_RLY_40A_State;
      }
      // update the Relay status
      digitalWrite(A1_RLY_40A_Pin, A1_RLY_40A_State);
      digitalWrite(A1_RLY_8M1_Pin, A1_RLY_8M1_State);
      digitalWrite(A1_RLY_4M1_Pin, A1_RLY_4M1_State);
      break;
    case 2:
      // Update button state
      BTN_P1.Update();
      BTN_P2.Update();
      //
      // Save clicks count, as click codes are reset at next Update()
      if (BTN_P1.clicks != 0) {
        BTN_P1_clicks = BTN_P1.clicks;
        Serial.print("BTN_P1 has been CLICKSed...(");Serial.print(BTN_P1_clicks);Serial.println(")");
      }
      if (BTN_P2.clicks != 0) {
        BTN_P2_clicks = BTN_P2.clicks;
        Serial.print("BTN_P2 has been CLICKSed...(");Serial.print(BTN_P2_clicks);Serial.println(")");
      }
      
      // Simply toggle relay on single clicks
      // (Cant use LEDfunction like the others here,
      //  as it would toggle on and off all the time)
      if(BTN_P1.clicks == 1) {
        Serial.println("BTN_P1 has been single-pressed [L3 switch]");
        A2_RLY_8M2_State = !A2_RLY_8M2_State;
      }
      if(BTN_P2.clicks == 1) {
        Serial.println("BTN_P2 has been single-pressed [L4 switch]");
        A2_RLY_4M2_State = !A2_RLY_4M2_State;
      }
      // Long press on P1 will turn all lights on
      if(BTN_P1.clicks <= -1) {
        Serial.println("BTN_P1 has been long-pressed [L3+L4 on]");
        A2_RLY_8M2_State = HIGH;
        A2_RLY_4M2_State = HIGH;

        // Let me prepare the "message" to send over the RS485 bus...
        //
        // let's destroy previous "snd_message", by zero-ing all its bytes
        memset (&snd_message, 0, sizeof snd_message);
        snd_message.src_device = thisdev;        // it's "my" message
        snd_message.dst_device = 1;            
        snd_message.command = TURN_ON_A1_L1;   // it carries some command code
        // OK, let's send...
        send_message(&snd_message);
      }
      // Long press on P4 will turn all lights off
      if(BTN_P2.clicks <= -1) {
        Serial.println("BTN_P2 has been long-pressed [L3+L4 off]");
        A2_RLY_8M2_State = LOW;
        A2_RLY_4M2_State = LOW;
        // Let me prepare the "message" to send over the RS485 bus...
        //
        // let's destroy previous "snd_message", by zero-ing all its bytes
        memset (&snd_message, 0, sizeof snd_message);
        snd_message.src_device = thisdev;        // it's "my" message
        snd_message.dst_device = 1;             
        snd_message.command = TURN_OFF_A1_L1; // it carries some command code
        // OK, let's send...
        send_message(&snd_message);      
      }
      digitalWrite(A2_RLY_8M2_Pin, A2_RLY_8M2_State);
      digitalWrite(A2_RLY_4M2_Pin, A2_RLY_4M2_State);

      // Let's check REED Switches state
      reed_sw1_state = ! digitalRead(REED1_SW_Pin);
      // did the state changed?
      if (reed_sw1_state != reed_sw1_previous_state) {
        // a change occurred. Are we in a "boucing" window?
        if (millis() - last_reed_sw1_debounce > REED_DEBOUNCE) {
          // no. So it's a real change
          // Let's handle the event...
          Serial.print(millis());
            Serial.print("[REED] - SW1 state CHANGED from [");
            Serial.print(reed_sw1_previous_state);
            Serial.print("] to ["); 
            Serial.print(reed_sw1_state);
            Serial.println("]");
            reed_sw1_previous_state = reed_sw1_state;
            last_reed_sw1_debounce = millis();
        } else {
          // yes. Let's ignore it
        }   
      }

      // Let's check REED Switches anti-tamper state
      if (millis()-reed_at1_lastcheck > REED_AT_CHECKDELAY) {
         reed_at1_state = ! digitalRead(REED1_AT_Pin);
         if (reed_at1_state == HIGH ) {
            Serial.print(millis());
            Serial.print("[REED] - AT1 state OK [");
            Serial.print(reed_at1_state);
            Serial.println("]");
         } else {
            Serial.print(millis());
            Serial.print("[REED] - AT1 state ALARM - OnGoing TAMPER!!! [");
            Serial.print(reed_at1_state);
            Serial.println("]");          
         }
         reed_at1_lastcheck = millis();
      }
      break;
      
    default:
      break;
  }
}

