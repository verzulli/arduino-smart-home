/**************************************************************************
    Souliss - Simple LIGHT/LED
    
    Turn on/off on-board LED (on PIN 13) via a push-button connected to PIN 2
    or the Souliss APP.

    It can be easily adapted to control a real light, by using an external
    relay-module
 
    Run this code on mostly every Arduino (I've used a MEGA-2560_Rev3)
    with an Ethernet shield (I've used a W5100) 
    
***************************************************************************/

// Configure the basic of the Souliss framework
#include "bconf/StandardArduino.h"          // I'm using a "standard" Arduino
#include "conf/ethW5100.h"                  // I've a 5100 Ethernet shield

// this is required by the Ethernet library (ethW5100.h)
#include <SPI.h>

// I'm using only one Arduino. As such, it acts also as the Souliss GATEWAY
// So I need to include following header file
#include "conf/Gateway.h"                   

// I'm including the souliss framework. It should be the LAST thing to be
// included
#include "Souliss.h"


// Define the network configuration
// Here, my node is 10.20.20.99 with netmask 255.255.255.0. My default
// gateway is 10.20.20.100
uint8_t ip_address[4]  = {192, 168, 2, 199};
uint8_t subnet_mask[4] = {255, 255, 255, 0};
uint8_t ip_gateway[4]  = {192, 168, 2, 1};

// As for souliss addressing, we use the last octet              
// of the IP address (199) as the souliss vNet address
#define myvNet_address  ip_address[3]       
#define myvNet_subnet   0xFF00

#define RELAY_CONTROL          0            // This is the memory slot for the logic that handle the light


// My PushButton is connected to PIN 2
#define PUSHBUTTON_INPUT_PIN  2

// I'm currently turning ON/OFF the internal LED. It's connected on PIN 13
#define OUTPUT_RELAY_PIN      13


void setup()
{   
    // Let's initialize Souliss' stuff...
    Initialize();
    
    // Set network parameters
    Souliss_SetIPAddress(ip_address, subnet_mask, ip_gateway);

    // Set this node as gateway for SoulissApp  
    SetAsGateway(myvNet_address);        
            
    // Set the typical logic to use
    // List of "typicals" is here: https://github.com/souliss/souliss/wiki/Typicals
    Set_T11(RELAY_CONTROL);
    
    // Define inputs, outputs pins
    pinMode(PUSHBUTTON_INPUT_PIN, INPUT_PULLUP);      // Don't forget to properly handle PULLups
    pinMode(OUTPUT_RELAY_PIN, OUTPUT);                // Let's start with on-board LED
}

void loop()
{ 
    // Here we start to play
    EXECUTEFAST() {                     
        UPDATEFAST();   

        // Read every 50ms the input state and send it to the other board   
        FAST_50ms() {
            
            // Use PUSHBUTTON_INPUT_PIN as ON/OFF command (don't forget to handle pullup/pulldown issues)
            // It get the digital input state and store it in the RELAY_CONTROL logic
            DigIn(PUSHBUTTON_INPUT_PIN, Souliss_T1n_ToggleCmd, RELAY_CONTROL);     

            // Use in inputs stored in the RELAY_CONTROL to control the OUTPUT_PIN/RELAY,
            Logic_T11(RELAY_CONTROL);
            
            DigOut(OUTPUT_RELAY_PIN, Souliss_T1n_Coil, RELAY_CONTROL);

            // Automatically handle the communication
            ProcessCommunication();
        }           
    }
} 
