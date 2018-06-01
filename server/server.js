//
//  server.js
//

// FIXME: input from Arduino is:   <<<<< <Buffer 73 74 65 70 3a 20>

const SerialPort = require("serialport");
const WebSocket = require("ws");

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const PORT = 8080;
const BAUD_RATE = 9600;
const DEVICE = "/dev/cu.usbmodem1411";

var port = null;

const socket = new WebSocket.Server({ port: PORT });

socket.on('connection', function connection(ws) {
	ws.on('message', function incoming(message) {
		console.log(':::::', message);

		if (port) {
			port.write(message);
		} else {
			console.warn("PORT NOT OPEN, can't send");
		}

	});
});

port = new SerialPort(DEVICE, {
	autoOpen: false,
	baudRate: BAUD_RATE
});

port.on('error', function(err){
	console.log("port general error:", err);
});

port.on('data', function (data) {
  console.log('<<<<<', data);
});

port.open(function(err){
	if (err) {
		console.log("port open error:", err);
		return;
	}

	console.log("port open!");
});
