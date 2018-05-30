//
//  server.js
//

const SerialPort = require("serialport");
const WebSocket = require("ws");

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const PORT = 8080;
const BAUD_RATE = 19200;

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

port = new SerialPort("/dev/cu.usbmodem1421", {
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
