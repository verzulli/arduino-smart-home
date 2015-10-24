#include <RS485_non_blocking.h>
#include <EEPROM.h>

// EPROM memory location where each device holds it's own ID
#define DEVICE_ID 0

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

// PIN8 is used as a visual-feedback for:
// - MASTER: when sending "messages" over the RS485 bus
// - SLAVES: when a "messages" has been received from the RS485 bus
const byte LED_PIN = 8;

// Let's instantiate the object (myChannel) to deal with the RS485 bus
// Note: we're using 22 char-long "messages", so we declare such a size
//       in addition to callbacks
RS485 myChannel (fRead, fAvailable, fWrite, 22);

// Let's define an 8-elements text-array so to send also some "text" over the bus
char* text[]={"Alfa", "Beta", "Gamma", "Delta", "Charly","Defcon 4", "tarantana", "blindosbarra"};
int text_size = 8;

// This is the structure of the "message" we're sending over the bus. 
// It's MANDATORY it has a FIXED lenght! In our case, 22 bytes!
struct {
    byte device;
    byte command;
    char text[20];
}  message;

// some other variables
unsigned int thisdev = 0;   // to hold our EEPROM retrieved ID
unsigned int count = 1;
unsigned long ts = millis();
unsigned long delta = 0;


void setup()
{
  // this is my main "console"
  Serial.begin (9600);

  // Let's check, in the EEPROM, which is my ID
  // BTW:
  //  - ID: 1   => MASTER
  //  - ID: 2   => SLAVE
  //  - ID: 100 => SLAVE
  thisdev = EEPROM.read(DEVICE_ID);

  // some "debug" message...
  Serial.print("Starting on [");
  Serial.print(thisdev);
  Serial.println("]");

  // this is my RS485 bus
  Serial2.begin (38400);
  myChannel.begin();  // this is required by the RS485 library

  Serial.print("Setting up PINs...");
  pinMode (ENABLE_PIN, OUTPUT);  // driver output enable
  pinMode (LED_PIN, OUTPUT);  // built-in LED

  Serial.println("Done!");

  // as I'm using "random" function, let's "seed" it...
  randomSeed(millis());
}  // end of setup
  
void loop()
{

  if (thisdev == 1) {
    // I'm the master

    // I'm sending a message every 2-4 seconds...
    if (millis()-ts >= 2000+delta) {

      delta = 10*random(200);
      
      // Let me prepare the "message" to send over the RS485 bus...
      message.device = thisdev;       // it's "my" message
      message.command = random(255);  // it carries some command code
       
      unsigned int idx = random(text_size-1);  // and a (random)...
      strlcpy(message.text, text[idx], sizeof(message.text)) ; // ...text

      // Let's debug, in the console, that a "message" is ready to be sent over the RS485 bus
      Serial.print(millis());Serial.print(" - Sending message: ["); Serial.print(count++); Serial.println("]");
      Serial.print("   device.=["); Serial.print(message.device); Serial.println("]");
      Serial.print("   command=["); Serial.print(message.command); Serial.println("]");
      Serial.print("   text...=["); Serial.print(message.text); Serial.println("]");
      Serial.println("--------------------------------------------------");

      // send the message over the RS485 bus
      digitalWrite (LED_PIN,HIGH);      // let's turn on our visual-feedback
      digitalWrite (ENABLE_PIN, HIGH);  // let's tell our RS485 brick to switch to SEND mode
      myChannel.sendMsg ((byte *) &message, sizeof message);  // send the message
      digitalWrite (ENABLE_PIN, LOW);  // disable sending
      digitalWrite (LED_PIN, LOW);  // turn-off visual-feedback

      // let's update "ts" so to properly wait for next trasmission
      ts=millis();
    } else {
      // ...it's too early for another message. Let me just
      // loop, so to spend some time...
    }
  }

  if ((thisdev == 2) || (thisdev == 100)) {
    // I'm a slave

    // let's check if a "message" have been received
    // BTW: "update" will return TRUE when the "message" is ready 
    if (myChannel.update ()) {

        // let's turn-on visual feedback
        digitalWrite (LED_PIN,HIGH);

        // let's destroy previous "message", by zero-ing all its bytes
        memset (&message, 0, sizeof message);

        // let's check the size of our message
        int len = myChannel.getLength ();

        // debug some info on console
        Serial.print("Received ["); Serial.print(len); Serial.println("] bytes. Let's see...");

        // let's ensure to not overflow our "message" structure
        if (len > sizeof message)
          len = sizeof message;

        // ok. So let's get our RS485 data and copy them on our "message" data structure
        memcpy (&message, myChannel.getData (), len);

        // let's debug some info over the console
        Serial.print("  device.=["); Serial.print(message.device); Serial.println("]");
        Serial.print("  command=["); Serial.print(message.command); Serial.println("]");
        Serial.print("  text...=["); Serial.print(message.text); Serial.println("]");
        Serial.println("--------------------------------------------------");

        // let's turn-off visual-feedback
        digitalWrite (LED_PIN, LOW);  
    }
  }
}  // end of loop
