/*
 
Door Bell
Handles both ringing and sensing up to 4 bells
Posts to open.sen.se
 
 created 3/2/2014
 by Keith Baker
 
 */

#include <SPI.h>
#include <Ethernet.h>
#include <avr/wdt.h>

// Enter a MAC address for your controller below.
// Newer Ethernet shields have a MAC address printed on a sticker on the shield
byte mac[] = {  
  0x00, 0xAA, 0xBB, 0xCC, 0xDE, 0x04 };

// Initialize the Ethernet client library
EthernetClient client;

//Button Pins
int buttons[] = {7, 3, 8, 10};

//State of the buttons
int button_status[] = {0, 0, 0, 0};

//Relay Pins
int relays[] = {9, 2, 5, 6};

//Feeds
long feed_ids[] = {39398, 39399, 39400, 39401};
//long feed_ids[] = {1, 2, 3, 4};

//Post 1 / 5 minutes to sen.se no matter what
int heartbeat = 3000;
int lastbeat = 3000; //trigger push on startup

//Button timeout (1/10s of a second)
int button_timeout = 5;

//Sen.se API name
char server[] = "api.sen.se";
char sense_key[] = "";

void setup() {
  //Important to let Ethernet Controller Initialize
  delay(2000);
  
  //Setup pin modes
  for (int i = 0; i < 4; i++){
    pinMode(relays[i], OUTPUT);
    pinMode(buttons[i], INPUT);
  } 
  
  //General protection  
  wdt_enable(WDTO_8S);  
  // start the Ethernet connection:
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    // no point in carrying on, so do nothing forevermore, will trip watchdog
    for(;;)
      ;
  }
}

void update(long feed_id, int value){
  if (client.connect(server, 80)) {
    String data = "{\"feed_id\":";
    data += feed_id;
    data += ", \"value\":"; 
    data += value;
    data += '}';
    client.print("POST /events/ HTTP/1.1\n");
    client.print("Host: ");
    client.print(server);
    client.print("\n");
    client.print("Content-Type: application/json\n");
    client.print("sense_key: ");
    client.print(sense_key);
    client.print("\n");
    client.print("Connection: close\n");
    client.print("User-Agent: ArdunioBell\n");
    client.print("Content-Length: ");
    client.print(data.length());
    client.print("\n\n");
    client.print(data);
    while (client.connected()) {
        client.read();
    }
    client.stop();
  } 
}

void loop() {
  wdt_reset();
  lastbeat += 1;
  for (int i = 0; i < 4; i++){
    int current = digitalRead(buttons[i]);
    if (lastbeat > heartbeat){
      update(feed_ids[i], !current);
    }
    if ( current == 1 ){
      if (button_status[i] != 0){
        digitalWrite(relays[i], 0);
        button_status[i] = 0;
        update(feed_ids[i],0);
      }
    }
    if ( current == 0 ){
      button_status[i] += 1;
      if (button_status[i] == 1){
        digitalWrite(relays[i], 1);
        update(feed_ids[i],1);
      }
      else if (button_status[i] > button_timeout) {
        digitalWrite(relays[i], 0);
        //keep this low so we don't overflow
        button_status[i] = button_timeout;
      }
    }
  }
  if (lastbeat > heartbeat){
    lastbeat = 0; 
  }
  delay(100);
}


