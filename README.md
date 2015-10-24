# arduino-smart-home
Software and details about my (on-going) Arduino-powered smart-home

---

This is the collection of infos, refences and software that I'm actively searching/studying/developing to power-up and serve the "smart" home I'm currently building (as of 2015!).

More concretly, I'm going to install 4 arduinos within 4 electric-boxes around my home and use them to power lights (via relay modules), driven by common push-buttons.

Arduinos will be interconnected via an RS485 bus.

As of today (25/10/2015), I've assembled a prototype with:

   - three [Arduino MEGA 2560](https://www.arduino.cc/en/Main/ArduinoBoardMega2560Rev03 )
   - one 8-channel SSR relay module
   - one 4-channel SSR relay module
   - one 40A SSR relay
   - three RS485 "bricks" used to interconnect the three Arduinos to the RS485 bus, in a multidrop configuration
   - two push-buttons, one socket and a bunch of cabling

I'm currently powering all of them with:

  - a common 2A, 2 ports, USB power adapter (commonly used to power tablets);
  - an 4 ports USB2 HUB

A [fritzing](http://fritzing.org) schema is included here, altough it's not complete. A PNG export is included as well.

Hopefully I'll be able to improve this project furing following days/weeks. Stay tuned!

