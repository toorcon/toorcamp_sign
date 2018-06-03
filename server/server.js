//
//  server.js
//

const SerialPort = require("serialport");
const WebSocket = require("ws");
const fs = require("fs");
const _ = require("lodash");

// Arduino Uno: 19200 baud works, 57600 definitely does not.
const WEBSERVER_PORT = 8080;
const BAUD_RATE = 9600;

// Try to auto-detect the device
let dirs = fs.readdirSync("/dev/");
let devices = [];

_.each(dirs, (d) => {
	if (d.match(/^cu\.usbmodem/)) {
		console.log("Auto-detect found device: '" + d + "'");
		devices.push(d);
	}
});

//const device = "/dev/cu.usbmodem4399551";
//const device = "/dev/cu.usbmodem4400031";

if (devices.length !== 1) {
	console.error("** No device found that matches: '/cu/usbmodem*'. Sorry.");
	return;
}
let device = '/dev/' + devices[0];

let port = null;

let socket = new WebSocket.Server({ port: WEBSERVER_PORT });

socket.on('open', function open(){
	console.log("WebSocket: open");
});

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

function noop() {}

function heartbeat() {
  this.isAlive = true;
}

socket.on('connection', function connection(ws) {
	console.log("WebSocket open!");
  ws.isAlive = true;
  ws.on('pong', heartbeat);
});

const interval = setInterval(function ping() {
  socket.clients.forEach(function each(ws) {
    if (ws.isAlive === false) return ws.terminate();

    ws.isAlive = false;
    ws.ping(noop);
  });
}, 1000);

//
//  SERIAL PORT  (to microcontroller)
//

port = new SerialPort(device, {
	autoOpen: false,
	baudRate: BAUD_RATE
});

port.on('error', function(err){
	console.log("port general error:", err);
});

port.on('data', function (data) {
  console.log('<<<<<', data.toString('ascii'));
});

port.open(function(err){
	if (err) {
		console.log("port open error:", err);
		return;
	}

	console.log("Serial port open!");
});
