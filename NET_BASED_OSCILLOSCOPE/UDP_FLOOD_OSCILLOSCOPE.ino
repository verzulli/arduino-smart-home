#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>    


#define ANALOG_A_IN 2
#define ANALOG_B_IN 1

byte mac[] = {
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED
};

// IP and PORT of LOCAL Arduino
IPAddress local_ip(10, 20, 20, 4);
unsigned int local_port = 8888;

// IP and PORT of REMOTE host
IPAddress remote_ip(10, 20, 20, 1);
unsigned int remote_port = 8888;

unsigned int counter = 0;

// buffers for receiving and sending data
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  //buffer to hold incoming packet,
char ReplyBuffer[] = "acknowledged";       // a string to send back

// An EthernetUDP instance to let us send and receive packets over UDP
EthernetUDP Udp;

void setup() {
  Serial.begin(115200);
  Ethernet.begin(mac,local_ip);

  Udp.begin(local_port);
}

void loop() {

    int val_a = analogRead(ANALOG_A_IN); 
    int val_b = analogRead(ANALOG_B_IN); 
    Udp.beginPacket(remote_ip, remote_port);
    Udp.write(val_a >> 2);
    Udp.write(val_b >> 2);
    Udp.endPacket();

//    if (counter = 1000) {
//      Serial.print("[");
//      Serial.print(counter);
//      Serial.println("]");
//      counter=0;
//    } else {
//      counter++;
//    }
}
