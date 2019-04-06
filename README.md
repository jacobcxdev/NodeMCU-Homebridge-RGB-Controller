# NodeMCU Homebridge RGB Controller

## Instructions
1)  Install `homebridge` and `homebridge-better-http-rgb` via `npm`
2)  Create a `config.json` in `~/.homebridge/` and paste the following contents in it, modifying as necessary:
```json
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
```
3) Copy the `config.json` to `/var/homebridge/` and start Homebridge.
4) Modify the `WiFi network SSID`, `password`, `IP`, `gateway`, `subnet mask`, and `hostname` below as necessary (read in the comments - starred).
5) Adjust `offColour`, `hexColour` and `transitionMultiplier` below to your preference (I would advice leaving them alone until you have everything already working) (read in the comments - starred).
6) Upload to your NodeMCU, and if you've followed your guide correctly, this should all work beautifully!

### NB: To fix a brightness compatibility bug with `homebridge-better-http-rgb`, follow these instructions:
1)  Stop Homebridge.
2)  Navigate to `/usr/local/lib/node_modules/homebridge-better-http-rgb/index.js`.
3)  Search for where it says `_setRGB: function(callback)`.
4)  Read the next line where it says `var rgb = this._hsvToRgb(this.cache.hue, this.cache.saturation, this.cache.brightness);`.
5)  Change `this.cache.brightness` to `100`.
6)  Save and close the file.
7)  Restart Homebridge and everything should be working normally.

#### I followed the guide over at [esp8266.com](https://esp8266.com/viewtopic.php?f=11&t=12259), replacing their code and config.json with mine.
###### NB: You must register an account with esp8266.com before you can view the circuit diagrams etc.
