/**************************************************************************
    Souliss - Simple.... (to be written)
***************************************************************************/
#include <EEPROM.h>

// EPROM memory location where each device stores it's own ID
#define DEVICE_ID 0

// -------- Let's start configuring the framework... ----------
//
// as I'm using a MEGA2560_rev3, let's start including the (tiny, indeed) "default"
// arduino-family stuff
#include "bconf/StandardArduino.h"
//
// as I'm using an RS485 bus, let's include the usart/RS-485 stuff as well
// (BTW: I'm using this brick: https://arduino-info.wikispaces.com/SoftwareSerialRS485Example
//       over Serial2 (phisical)
#include "conf/usart.h"
// ...and let me configure how I'm going to phisically interface the RS-485 bus
#define USARTDRIVER_INSKETCH
#define USART_TXENABLE          1          // I need the TX-Enable "stuff"
#define USART_TXENPIN           3          // this is my TX-Enable PIN
#define USARTDRIVER             Serial2   // I'm using Serial2 on my Mega2560
#define USART_LOG               Serial.print  // for logging, I'll.... "Serial.print"
//
// Let's override the baud-rate to use over the RS-485 bus
#define USARTBAUDRATE_INSKETCH
#define USART_BAUD9k6                          1
#define USART_BAUD19k2                         0
#define USART_BAUD57k6                         0
#define USART_BAUD115k2                        0
#define USART_BAUD256k                         0
// ^^^^^
// Let's override the debugging code...
#define USART_DEBUG_INSKETCH
#define USART_DEBUG                            0
#define SOULISS_DEBUG_INSKETCH
#define SOULISS_DEBUG                          0
#define VNET_DEBUG_INSKETCH
#define VNET_DEBUG                             0
// ^^^^^
// ---------- done, with the MEGA2560 stuff. Let's proceed with other Souliss stuffs...

// I'm going to define a dedicate "topic" to handle LOG messages allover
// the souliss network
// https://github.com/souliss/souliss/wiki/Peer2Peer
#define LOG_EVENT 0xF001, 0x02

#include "conf/ethW5100.h"                  // Ethernet through Wiznet W5100
#include "conf/Gateway.h"                   // The main node is the Gateway

// Include framework code and libraries
#include <SPI.h>

/*** All configuration includes should be above this line ***/ 
#include "Souliss.h"

// Define the network configuration according to your router settings
uint8_t gw_ip_address[4]  = {10, 20, 20, 99};
uint8_t gw_subnet_mask[4] = {255, 255, 255, 0};
uint8_t gw_ip_gateway[4]  = {10, 20, 20, 100};

int thisdev = -1;       // default unexisting device_id, to be retrieved and assigned during setup()

#define Gateway_address 77
#define Peer_address    78
#define myvNet_address  gw_ip_address[3]       // The last byte of the IP address (77) is also the vNet address

#define Gateway_RS485_address   0xCE01
#define Peer2_address           0xCE02
#define myvNet_subnet           0xFF00
#define myvNet_supern           Gateway_RS485_address

// Those following are the memory slot for the logic that handle the lights
#define LIGHT3          0
#define LIGHT4          1
#define LIGHT1          2
#define LIGHT2          3


// Define human-readable costants for this application
// ----------- vvvvvvvvvvvvvvv -----------------------
// PushButtons and Relay on the GATEWAY
#define PUSHBUTTON_P1_INPUT_PIN  41   // on DEVICE_ID 100 => GATEWAY
#define PUSHBUTTON_P2_INPUT_PIN  45   // on DEVICE_ID 100 => GATEWA
#define OUTPUT_RELAY1_PIN      25   // on GW
#define OUTPUT_RELAY2_PIN      29   // on GW
//
// PushButtons and Relay on PEER2
#define PUSHBUTTON_P3_INPUT_PIN  48   // on DEVICE_ID 2 => PEER2
#define PUSHBUTTON_P4_INPUT_PIN  44   // on DEVICE_ID 2 => PEER2
#define OUTPUT_RELAY3_PIN      33   // on PEER2
#define OUTPUT_RELAY4_PIN      37   // on PEER2
// -------------^^^^^^^^^^^^^^^^^---------------------


// I need this to troubleshoot memory issues...
int freeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void setup()
{   

    // this is my main "output/debug console"
    Serial.begin (115200);

    // Let's check, in the EEPROM, which is my ID
    // BTW:
    //  - ID: 1   => MASTER
    //  - ID: 2   => SLAVE
    //  - ID: 100 => GATEWAY
    thisdev = EEPROM.read(DEVICE_ID);

    // some "debug" message...
    Serial.print("Starting SOULISS on device with ID [");
    Serial.print(thisdev);
    Serial.println("] ");

    Initialize();

    switch (thisdev) {
      case 100:
        // ***********************************************************
        // Device with DEVICE_ID=100 is the GATEWAY so let's handle it
        // --------------------vvvvvvvvvv-----------------------------
        // Set network parameters
        Serial.println("Initializing networking...");
        Souliss_SetIPAddress(gw_ip_address, gw_subnet_mask, gw_ip_gateway);
        SetAsGateway(myvNet_address);     // Set this node as gateway for SoulissApp  

        // Define the address for the RS485 interface
        Souliss_SetAddress(Gateway_RS485_address, myvNet_subnet, 0x0000);

        // Let's declare our "peers"
        SetAsPeerNode(Peer2_address, 1);
                
        // Define inputs, outputs pins
        Serial.println("Initializing pinouts...");
        pinMode(PUSHBUTTON_P1_INPUT_PIN, INPUT_PULLUP);
        pinMode(PUSHBUTTON_P2_INPUT_PIN, INPUT_PULLUP);
        pinMode(OUTPUT_RELAY1_PIN, OUTPUT);
        pinMode(OUTPUT_RELAY2_PIN, OUTPUT);
        
        // Set the typical logic to use
        // List of "typicals" is here: https://github.com/souliss/souliss/wiki/Typicals
        Serial.println("Initializing TYPICALs on dev 100...");
        Set_T11(LIGHT3);
        Set_T11(LIGHT4);

        // --------------------^^^^^^^^^^^^-----------------------------

        break;
      case 1:
        // ***********************************************************
        // Device with DEVICE_ID=1 is the PEER 2 so let's handle it
        // --------------------vvvvvvvvvv-----------------------------
        // Set network parameters
        Serial.println("Initializing SOULISS networking...");

        // Define the address for the RS485 interface
        Souliss_SetAddress(Peer2_address, myvNet_subnet, Gateway_RS485_address);
        
        // Define inputs, outputs pins
        Serial.println("Initializing pinouts...");
        pinMode(PUSHBUTTON_P3_INPUT_PIN, INPUT_PULLUP);
        pinMode(PUSHBUTTON_P4_INPUT_PIN, INPUT_PULLUP);
        pinMode(OUTPUT_RELAY3_PIN, OUTPUT);
        pinMode(OUTPUT_RELAY4_PIN, OUTPUT);

        // Set the typical logic to use
        // List of "typicals" is here: https://github.com/souliss/souliss/wiki/Typicals
        Serial.println("Initializing (local) TYPICALs...");
        Set_T11(LIGHT1);
        Set_T11(LIGHT2);
        
        break;
      default: 
        // if nothing else matches, do the default
        // default is optional
        Serial.print("Hei! You should _NEVER_ get here, as you have an UNKNOWN device_id! [");
        Serial.print(thisdev);
        Serial.println("]");

        break;  
    }
    
    Serial.println("All done! Let's enter the main loop! Good luck!");
}

void loop()
{ 
    EXECUTEFAST() {                     
        UPDATEFAST();   

        // Read every 50ms the input state and send it to the other board   
        FAST_50ms() {

            switch (thisdev) {
              case 100:      
                  // Here we start to play with the GATEWAY
                  
                  // Use PUSHBUTTON_P1_INPUT_PIN as ON/OFF command (don't forget to handle pullup/pulldown issues)
                  // It get the digital input state and store it in the LIGHT* logic
                  DigIn(PUSHBUTTON_P1_INPUT_PIN, Souliss_T1n_ToggleCmd, LIGHT3);
                  DigIn(PUSHBUTTON_P2_INPUT_PIN, Souliss_T1n_ToggleCmd, LIGHT4);
      
                  // Use in inputs stored in the RELAY1 to control the OUTPUT_PIN/RELAY,
                  Logic_T11(LIGHT3);
                  Logic_T11(LIGHT4);
                  
                  DigOut(OUTPUT_RELAY1_PIN, Souliss_T1n_Coil, LIGHT3);
                  DigOut(OUTPUT_RELAY2_PIN, Souliss_T1n_Coil, LIGHT4);
                  break;

              case 1:      
                  // Here we start to play with PEER2
                  
                  // Use PUSHBUTTON_P1_INPUT_PIN as ON/OFF command (don't forget to handle pullup/pulldown issues)
                  // It get the digital input state and store it in the LIGHT* logic
                  DigIn(PUSHBUTTON_P3_INPUT_PIN, Souliss_T1n_ToggleCmd, LIGHT1);
                  DigIn(PUSHBUTTON_P4_INPUT_PIN, Souliss_T1n_ToggleCmd, LIGHT2);
      
                  // Use in inputs stored in the RELAY1 to control the OUTPUT_PIN/RELAY,
                  Logic_T11(LIGHT1);
                  Logic_T11(LIGHT2);
                  
                  DigOut(OUTPUT_RELAY3_PIN, Souliss_T1n_Coil, LIGHT1);
                  DigOut(OUTPUT_RELAY4_PIN, Souliss_T1n_Coil, LIGHT2);
                  break;
            }           
        }
        // Run communication as Gateway or Peer
        if (IsRuntimeGateway())
          FAST_GatewayComms();
        else
          FAST_PeerComms();
    }

    EXECUTESLOW() { 
        UPDATESLOW();

        SLOW_10s() {                
            Serial.print("This is an heartbeat from [");
            Serial.print(thisdev);
            Serial.print("|RamFree: ");
            Serial.print(freeRam());
            Serial.println("]");
        }
        // If running as Peer
        if (!IsRuntimeGateway())
          SLOW_PeerJoin();        
    }
} 
