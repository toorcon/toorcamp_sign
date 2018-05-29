//
//  server.js
//

const SerialPort = require("serialport");
const WebSocket = require("ws");

// Arduino Uno: 19200 works, 57600 definitely does not
const PORT = 8080;
const BAUD_RATE = 19200;

var port = null;

const socket = new WebSocket.Server({ port: PORT });

socket.on('connection', function connection(ws) {
	ws.on('message', function incoming(message) {
		console.log('received: %s', message);
	});

	//ws.send('something');
});

port = new SerialPort("/dev/cu.usbmodem1421", {
	autoOpen: false,
	baudRate: BAUD_RATE
});

port.on('error', function(err){
	console.log("port error:", err);
});

port.open(function(err){
	if (err) {
		console.log("port error:", err);
		return;
	}

	console.log("port open!");

	function loopOff() {
		setTimeout(loopOn, 1000);
		port.write("0 is zero\n");
	}

	function loopOn() {
		setTimeout(loopOff, 1000);
		port.write("1 is a one! yay\n");
	}

	loopOff();

});
