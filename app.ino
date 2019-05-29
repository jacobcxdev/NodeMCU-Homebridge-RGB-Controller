//
//  app.ino
//  NodeMCU RGB Controller for Homebridge
//
//  Created by Jacob Clayden on 04/04/2019.
//  Copyright © 2019 @JacobCXDev. All rights reserved.
//


/* Instructions:

1)  Install homebridge and homebridge-better-http-rgb via npm
2)  Create a config.json in ~/.homebridge/ and paste the following contents in it, modifying as necessary:

{
	"bridge": {
		"name": "Homebridge",
		"username": "CC:22:3D:E3:CE:30",
		"port": 51826,
		"pin": "031-45-154"
	},

	"description": "Jacob's Homebridge",

	"accessories": [{
		"accessory": "HTTP-RGB",
		"name": "Light Strip",
		"service": "Light",

		"switch": {
			"status": "http://192.168.1.10:80/status",
			"powerOn": "http://192.168.1.10:80/on",
			"powerOff": "http://192.168.1.10:80/off"
		},

		"color": {
			"status": "http://192.168.1.10:80/set",
			"url": "http://192.168.1.10:80/set/%s"
		},

		"brightness": {
			"status": "http://192.168.1.10:80/brightness",
			"url": "http://192.168.1.10:80/brightness/%s"
		}
	}],

   "platforms": []

}

3)  Copy the config.json to /var/homebridge/ and start Homebridge.
4)  Modify the WiFi network SSID, password, IP, gateway, subnet mask, and hostname below as necessary (read below - starred).
5)  Adjust offColour, hexColour and transitionMultiplier below to your preference (I would advice leaving them alone until you have everything already working) (read below - starred).
6)  Upload to your NodeMCU, and if you've followed your guide correctly, this should all work beautifully!


NB: To fix a brightness compatibility bug with homebridge-better-http-rgb, follow these instructions:

1)  Stop Homebridge.
2)  Navigate to /usr/local/lib/node_modules/homebridge-better-http-rgb/index.js.
3)  Search for where it says "_setRGB: function(callback)".
4)  Read the next line where it says "var rgb = this._hsvToRgb(this.cache.hue, this.cache.saturation, this.cache.brightness);".
5)  Change "this.cache.brightness" to "100".
6)  Save and close the file.
7)  Restart Homebridge and everything should be working normally.


I followed the guide over at esp8266.com, replacing their code and config.json with mine.
NB: You must register an account with esp8266.com before you can view the circuit diagrams etc.

*/

#include <ESP8266WiFi.h>
#include <cstdlib>

#define redPin 13                                                                                           // Pin D7.
#define greenPin 12                                                                                         // Pin D6.
#define bluePin 14                                                                                          // Pin D5.

WiFiServer server(80);                                                                                      // Starts a WiFi server on port 80.

const char *ssid = "YOUR_ROUTER_SSID";                                                                      // * Input your WiFi network SSID.
const char *password = "YOUR_ROUTER_PASSWORD";                                                              // * Input your WiFi network password.
IPAddress ip(192, 168, 1, 10);                                                                              // * Chose a fixed IP for your NodeMCU (ensure it is the the same as the one in the Homebridge config.json).
IPAddress gateway(192, 168, 1, 1);                                                                          // * Enter your Router's gateway. This should be printed on the back of the router.
IPAddress subnet(255, 255, 255, 0);                                                                         // * Enter the subnet mask for your network.

const String offColour = "000000";                                                                          // * Input the hex colour your light should be when in the "off" state.
String hexColour = "FFFFFF";                                                                                // * Input the hex colour your light should be when you turn it on initially.
String readString;                                                                                          // readString is used to store requests sent to the NodeMCU. This will contain the URL for the request.

float transitionMultiplier = 0.0005;                                                                        // * transitionMultiplier is used to determine the magnitude of the transition delay.
int precisionMultiplier = 10;                                                                               // precisionMultiplier is used to multiply the rgb123 values to avoid stuttering with < 10% brightness.
int brightness = 100;                                                                                       // brightness stores the brightness percentage.
int r1, g1, b1, r2, g2, b2, r3, g3, b3 = 0;                                                                 // rgb1 stores the rgb values for the beginning of a transition, rgb3 stores the rgb values for the end of a transition, and rgb2 stores the values of the currently displayed rgb values / transitionMultiplier. Imagine it like a slider, with rgb1 at the beginning and rgb3 at the end. rgb2 is the track which is incremented by a specific amount (calulated by rgb3 - rgb1) each loop to gradually transition to the final colour of the transition, held by the rgb3 values. rgb1 and rgb3 can be dynamically adjusted so that attempted concurrent transitions will not interfere. For example, a transition from #000000 to #FFFFFF would look like this: rgb1 = 0, rgb3 = 255, rgb2 += 255 until rgb2 * 0.0005 (the transitionMultiplier) == rgb3, at which point the transition is complete.
int state = 1;                                                                                              // state stores the state (on = 1, off = 0) of the light.


void WiFiStart() {                                                                                          // WiFiStart() is a function which sets up your connection to the WiFi network.
  WiFi.begin(ssid, password);                                                                               // Starts the WiFi connection.
  WiFi.config(ip, gateway, subnet);                                                                         // Sets the local IP, router gateway and the subnet mask.
  while (WiFi.status() != WL_CONNECTED) delay(100);                                                         // While the WiFi network isn't connected, this line loops a delay of 100 ms.
  server.begin();
  
  // Uncomment the code below to print the localIP and macAddress to the serial monitor.
  /*
  Serial.print("\n\n");
  Serial.print("MAC address : ");
  Serial.println(WiFi.macAddress());
  Serial.print("Local IP : ");
  Serial.println(WiFi.localIP());
  Serial.print("\n");
  */
}

void hexToRGB(String hex, int &r, int &g, int &b) {                                                         // hexToRGB() converts a hex string into an rgb value. It takes three int references as parameters, which it then sets to the calculated rgb values.
  long number = (long)strtol(&hex[0], NULL, 16);
  r = number >> 16;
  g = number >> 8 & 0xFF;
  b = number & 0xFF;
}

void setColour(String hex) {                                                                                // setColour() performs various operations to transition the colour of the light to the colour defined by the hex string it takes as a parameter.
  hexToRGB(hex, r3, g3, b3);                                                                                // This line sets rgb3 to the values for the colour of the endpoint of the transtion. If we're sticking with the slider analogy, this sets the end of the slider.
  r1 = r2 * transitionMultiplier;                                                                           // This line sets r1 to the r value which is being displayed at the beginning of the transition. In the slider analogy it sets the beginning of the slider.
  g1 = g2 * transitionMultiplier;                                                                           // This line does the same as above but with the g1 value.
  b1 = b2 * transitionMultiplier;                                                                           // This line does the same as above but with the b1 value.
  r3 *= precisionMultiplier;                                                                                // This multiplies the endpoint r value, r3, by the precisionMultiplier to increase the precision of the calculations.
  g3 *= precisionMultiplier;                                                                                // This line does the same as above but with the g3 value.
  b3 *= precisionMultiplier;                                                                                // This line does the same as above but with the b3 value.
  r3 *= (float)brightness / 100;                                                                            // This multiplies the r3 value by the current brightness level, dividing by 100 to obtain a decimal in the range 0.01 <= x = 1.
  g3 *= (float)brightness / 100;                                                                            // This line does the same as above but with the g3 value.
  b3 *= (float)brightness / 100;                                                                            // This line does the same as above but with the b3 value.
  setRGB();                                                                                                 // This line calls setRGB() to increment the rgb2 values by the difference between rgb3 and rgb1 (rgb3 - rgb1).
  writeRGB();                                                                                               // This line writes the rgb2 values to the pins, causing the light to change colour.
}

void setRGB() {                                                                                             // setRGB() increments the rgb2 values by the difference between rgb3 and rgb1 (rgb3 - rgb1). If you remember the slider analogy, this is what causes the track to move.
  if (r2 * transitionMultiplier != r3) r2 += r3 - r1;                                                       // If r2 multiplied by the transitionMultiplier is not equal to r3, in other words, if the transition is not complete, it increments r2 by the calculated amount. Bear in mind that this amount can be negative, for example, if the light is transitioning from #FFFFFF to #000000.
  if (g2 * transitionMultiplier != g3) g2 += g3 - g1;                                                       // This line does the same as above but with the g values.
  if (b2 * transitionMultiplier != b3) b2 += b3 - b1;                                                       // This line does the same as above but with the b values.
}

void writeRGB() {                                                                                           // writeRGB() writes the rgb2 values to the pins using analogWrite(). This is what causes the light to actually physically change colour.
  analogWrite(redPin, map(r2 * transitionMultiplier, 0, 255 * precisionMultiplier, 0, 1023));               // This line maps the colour to be displayed (r2 * transitionMultiplier) from the range 0 <= x <= 255 to 0 <= x <= 1023, which is the 10-bit range of the pin.
  analogWrite(greenPin, map(g2 * transitionMultiplier, 0, 255 * precisionMultiplier, 0, 1023));             // This line does the same as above but with the g2 value.
  analogWrite(bluePin, map(b2 * transitionMultiplier, 0, 255 * precisionMultiplier, 0, 1023));              // This line does the same as above but with the b2 value.
}


void setup() {                                                                                              // setup() is called when the NodeMCU is initially powered on. This is where one would usually perform operations which are required for the program to function as expected.
  Serial.begin(115200);                                                                                     // Serial.begin(115200) sets the baud rate for data transmittion via the serial pins to 115200.
  WiFi.hostname("Light-Strip");                                                                             // * Input what you would like the hostname of the NodeMCU to be, as seen by the router.
  WiFi.mode(WIFI_STA);                                                                                      // Sets the WiFi mode to station (rather than Access Point).
  WiFiStart();                                                                                              // Calls WiFiStart() to setup and start the WiFi network connection and server.
  setColour(hexColour);                                                                                     // Calls setColour() to set the colour to the initial colour, defined by the hexString.
}

void loop() {                                                                                               // loop() is called continuously while the NodeMCU is powered on and operational. This is the function where one would place any operations which would be ran continuously, for example, checking for network requests from other devices.
  if (state == 1) setColour(hexColour);                                                                     // If the light is on (state = 1), set the colour to the hexColour. Note that this is called continuously, so when you want to start a transition to a new colour you simply update the hexColour string.
  else setColour(offColour);                                                                                // If the light is off (state = 0), set the colour to the offColour.

  WiFiClient client = server.available();                                                                   // server.available() gets a client that is connected to the server which is being hosted by the NodeMCU and has data available for reading. This client is then saved to the variable client.
  if (!client) return;                                                                                      // If there is no client available (i.e. no network requests have been made), return.
  while (client.connected() && !client.available()) delay(1);                                               // While the client is connected but there are no bytes of data available for reading, delay for 1 ms.
  if (client) while (client.connected()) if (client.available()) {                                          // If the client still exists, while the client is connceted, if the client has bytes of data available for reading...

    char c = client.read();                                                                                 // Read and write the next byte of the data recieved from the client, assigning it the reference "c".
    if (readString.length() < 100) readString += c;                                                         // If the length of readString < 100 (i.e. there's no data in it), add c to it.

    if (c == '\n') {                                                                                        // If c == a newline character...
      client.println("HTTP/1.1 200 OK");                                                                    // Print the HTTP status code to the client.
      client.println("Content-Type: text/html");                                                            // Print the content-type of the requested webpage to the client.
      client.println();

      if (readString.indexOf("on") > 0) state = 1;                                                          // If readString contains the string "on", turn the light on (state = 1).
      if (readString.indexOf("off") > 0) state = 0;                                                         // If readString contains the string "off", turn the light off (state = 0).
      if (readString.indexOf("status") > 0) client.println(state);                                          // If readString contains the string "status", print the state of the light to the client (on/off = 1/0).
      if (readString.indexOf("set") > 0) {                                                                  // If readString contains the string "color" (ew the American spelling!)...
        if (readString.substring(9, 15).indexOf("HTT") == -1) hexColour = (readString.substring(9, 15));    // Create a substring of readString from the 9th to the 15th character. If it contains a hex colour (and so doesn't contain "HTT"), set the hexString to said colour.
        client.println(hexColour);                                                                          // Print hexColour to the client.
      }
      if (readString.indexOf("brightness") > 0) {                                                           // If readString contains the string "brightness"...
        if (readString.substring(16, 19).indexOf("HTT") == -1) {                                            // Create a substring of readString from the 16th to the 19th character. If it contains a brightness percentage (and so doesn't contain "HTT")...
          const String source = readString.substring(16, 19).c_str();                                       // Create a substring of readString from the 16th to the 19th character and assign it the reference "source".
          String target = "";                                                                               // Create an empty string called "target".
          for (char c : source) if ((c >= '0' && c <= '9')) target += c;                                    // For each character in source, if the character is a numeric character append it to target.
          brightness = std::atoi(target.c_str());                                                           // Set the brightness to the numeric value of the target as an integer.
        }
        client.println(brightness);                                                                         // Print brightness to the client.
      }

      delay(1);                                                                                             // Delays for 1 ms.
      while (client.read() >= 0);                                                                           // Read through any remaining bytes of data from the client.
      client.stop();                                                                                        // Stop the connection with the client.
      readString.remove(0);                                                                                 // Reset readString.
    }
  }
}
