const $ = require("jquery");
const _ = require("lodash");
const W3CWebSocket = require('websocket').w3cwebsocket;

/*  Parser test
x = (3 + 1) * 5;
y = x * x * T;
y += 2;
z = atan2(y, x);
w = sin(cos(z), tan(x * x));
q = w >= 12.34;
q10 = q + 10;
y = q10 ? GA : LA;
*/

const MAX_CODE_STEPS = 100;
const AUTO_PARSE_INTERVAL_MS = 500;
const PORT = 8080;
const RECONNECT_INTERVAL_MS = 2000;

// In order of precedence
const OPERATORS = "*,/,%,+,-,<,<=,>,>=,==,!=,?,:".split(",");

const OPERATOR_REF = "* / % + - < <= > >= == != ?:";

const ASSIGN_REF = '= += -= *= /= %=';

const FUNCS = "sin,cos,sin01,cos01,sinq,cosq,tan,pow,abs,atan2,floor,ceil,round,frac,sqrt,log,logBase,rand,randRange,noise1,noise2,noise3,noise1q,noise2q,noise3q,min,max,lerp,clamp,tri,peak,u2b,b2u,ternary,accum0,rgb,hsv".split(",").sort();

// All operations/functions must be sent as single-char. These are overrides:
const OPS = [
	{name: '<',         args: 2, code: '<'},
	{name: '>',         args: 2, code: '>'},
	{name: '<=',        args: 2, code: '{'},
	{name: '>=',        args: 2, code: '}'},
	{name: '==',        args: 2, code: '='},
	{name: '!=',        args: 2, code: '!'},
	{name: 'ternary',   args: 3, code: '?'},
	{name: 'sin',       args: 1, code: 'S'},
	{name: 'cos',       args: 1, code: 'C'},
	{name: 'sin01',     args: 1, code: 's'},
	{name: 'cos01',     args: 1, code: 'c'},
	{name: 'sinq',      args: 1, code: 'q'},
	{name: 'cosq',      args: 1, code: 'Q'},
	{name: 'tan',       args: 1, code: 'T'},
	{name: 'pow',       args: 2, code: 'P'},
	{name: 'abs',       args: 1, code: '|'},
	{name: 'atan2',     args: 2, code: 'A'},
	{name: 'floor',     args: 1, code: '_'},
	{name: 'ceil',      args: 1, code: '`'},
	{name: 'round',     args: 1, code: 'R'},
	{name: 'frac',      args: 1, code: '.'},
	{name: 'sqrt',      args: 1, code: 'r'},
	{name: 'log',       args: 1, code: 'L'},
	{name: 'logBase',   args: 2, code: 'B'},
	{name: 'rand',      args: 1, code: 'z'},
	{name: 'randRange', args: 2, code: 'Z'},
	{name: 'noise1',    args: 1, code: '1'},
	{name: 'noise2',    args: 2, code: '2'},
	{name: 'noise3',    args: 3, code: '3'},
	{name: 'noise1q',   args: 1, code: '4'},
	{name: 'noise2q',   args: 2, code: '5'},
	{name: 'noise3q',   args: 3, code: '6'},
	{name: 'min',       args: 2, code: 'm'},
	{name: 'max',       args: 2, code: 'M'},
	{name: 'lerp',      args: 3, code: 'l'},
	{name: 'clamp',     args: 3, code: 'x'},
	{name: 'tri',       args: 1, code: 't'},
	{name: 'peak',      args: 1, code: 'p'},
	{name: 'u2b',       args: 1, code: 'b'},	// uni to bi
	{name: 'b2u',       args: 1, code: 'u'},	// bi to uni
	{name: 'accum0',    args: 1, code: '0'},
	{name: 'rgb',       args: 3, code: '['},
	{name: 'hsv',       args: 3, code: ']'}
];

function opWithName(name) {
	return _.find(OPS, function(op){
		return op.name == name;
	});
}

// FIXME: check for duplicate chars

const SPECIAL_VARS = {
	"T": "time, in seconds",
	"S": "station id: T=0, O=1, O=2, R=3, ...",
	"I": "index of LED on strand",
	"C": "number of LEDs on strand",
	"P": "ratio of LED on strand (==I/C)",
	"X": "global X position",
	"Y": "global Y position",
	"A": "global angle (from center of sign)",
	"U": "ultrasonic sensor",
	"UA": "pow(U, 2)",
	"UB": "pow(U, 4)",
	"UC": "pow(U, 8)",
	//"LX": "X position within the letter",
	//"LA": "local angle (from center of letter)",
	//"IN": "true if inside letter (hole)",
	//"OT": "true if outside letter (outer edge)",
};

// Code will execute in order
// Value is object like:  (a,b,c are arguments)
//   {op: "+", a: "step_0", b: 123.4, c: 0}
var steps = [];

// Key: "myVar", value: "step_0"
var varNames = {};

var client = null;

function isSendEnabledChecked() {
	return $('#sendEnabled').get(0).checked;
}

function isClientOpen() {
	return client && (client.readyState === WebSocket.OPEN);
}

function isClientAvailable() {
	if (!isSendEnabledChecked()) return false;
	return isClientOpen();
}

function sendMessageToRing(msg, options) {
	var status = isClientOpen() ? "(open)" : "(closed)";
	if (!isSendEnabledChecked()) status = "(send_disabled)";

	// First byte: message lifespan
	if (options && options['raw']) {
		// "i!" station ID: send in the raw
		console.log(status, msg);

		if (isClientAvailable()) {
			client.send(msg);
		}

	} else {
		var lifespan = "2";
		var out = lifespan + msg + "\n";
		console.log(status, out);

		if (isClientAvailable()) {
			client.send(out);
		}
	}
}

function barf(reason, expr) {
	var html = '<p class="barf">Error: <b>';
	html += reason + '</b></p>';
	html += '<blockquote class="barf">' + expr + '</blockquote>';
	$('#status').html(html);

	console.warn(reason, expr);
}

// Returns index step
function addStep(op, a, b, c) {
	var idx = steps.length;

	steps.push({op: op, a: a || 0, b: b || 0, c: c || 0});

	if (steps.length > MAX_CODE_STEPS) {
		throw new Error("TOO MANY STEPS");
	}

	return "step_" + idx;
}

function findParenGroup(str, fromIdx) {
	console.log("findParenGroup:", '"'+str+'"', fromIdx);

	// You can match balanced parens with a regex, but it's a known hard problem.
	// Here's a simpler way: Count paren pairs.
	var depth = 0;

	for (var i = fromIdx; i < str.length; i++) {
		if (str[i] === '(') depth++;
		if (str[i] === ')') depth--;

		if (depth === 0) {
			return str.substring(fromIdx, i + 1);
		}
	}

	barf("Unmatched parens @ " + fromIdx, str);
}

// Returns array of arguments that were in the top level of a paren group.
//      "(1, sin(2, 3), hi)"   =>   ["1", "sin(2, 3)", "hi"]
function splitArgsInParens(str) {
	var args = [""];
	var depth = 0;

	for (var i = 0; i < str.length; i++) {
		if (str[i] === ')') depth--;

		// Add the characters
		if (((depth === 1) && (str[i] !== ',')) || (depth >= 2)) {
			args[args.length - 1] += str[i];
		}

		if (str[i] === '(') depth++;

		// Comma on level 1: Start building a new argument string
		if ((depth === 1) && (str[i] === ',')) args.push("");

		// End of paren group?
		if (depth === 0) break;
	}

	return args;
}

// Format is code. Some possibilities: "789.0"  "(789.0)"  "5+6"  "myVar/(otherVar-1)"
// Returns the index of the concluding step
function parseExpression(e)
{
	var tokens = [];

	function stripToken(token) {
		e = e.substr(token.length);
	}

	// Split e into tokens.
	while (e.length) {
		e = e.trim();

		console.log("  e:", e);

		// Number?
		var numMatch = e.match(/^[0-9\.]+/);
		if (numMatch) {
			tokens.push(parseFloat(numMatch[0]));
			stripToken(numMatch[0]);
			continue;
		}

		// Operator?
		const OPS_SORTED = OPERATORS.sort(function(a,b){return b.length - a.length;});
		var opFound = _.find(OPS_SORTED, function(op){
			return e.substr(0, op.length) === op;
		});
		if (opFound) {
			tokens.push(opFound);
			stripToken(opFound);
			continue;
		}

		// Function? Reserved word?
		//const FUNC_REGEX = new RegExp('^(' + FUNCS.join('|') + ')\\(');
		//var funcMatch = e.match(FUNC_REGEX);

		var funcMatch = e.match(/^([a-zA-Z_][a-zA-Z0-9_\-]*\s*)\(/);
		if (funcMatch) {
			var op = funcMatch[1].trim();

			// Valid function?
			if (FUNCS.indexOf(op) < 0) {
				barf("Invalid function name", e);
				return;
			}

			var parenGroup = findParenGroup(e, op.length);
			if (parenGroup) {
				console.log("got parenGroup:", parenGroup);
				var args = splitArgsInParens(parenGroup);

				// Enforce correct number of arguments
				var opObj = opWithName(op);
				var correctArgs = opObj['args'];

				// Arg count mismatch?
				if (args.length !== correctArgs) {

					function pluralize(str, count) {
						return str + ((count === 1) ? "" : "s");
					}

					barf(op + "(): expected <b>" + correctArgs + "</b> " + pluralize("arg", correctArgs) + ", got <b>" + args.length + "</b>", args);
					return;
				}

				// Parse the args as expressions
				var argTokens = _.map(args, function(arg){
					return parseExpression(arg);
				});

				tokens.push(addStep(op, argTokens[0], argTokens[1], argTokens[2]));
				stripToken(funcMatch[1] + parenGroup);
				continue;
			}

			barf("Function error, bad parens?", e);
			return;
		}

		// Var name / "step_234" / special var?
		var nameMatch = e.match(/^[a-zA-Z_\-][a-zA-Z0-9_\-]*/);
		if (nameMatch) {
			if (SPECIAL_VARS.hasOwnProperty(nameMatch[0])) {
				tokens.push('var_' + nameMatch[0]);
				stripToken(nameMatch[0]);
				continue;
			}

			if (!varNames.hasOwnProperty(nameMatch[0])) {
				barf("Unknown var name <b>" + nameMatch[0] + "</b>", e);
				return;
			}

			var dest = varNames[nameMatch[0]];
			tokens.push(dest);
			stripToken(nameMatch[0]);
			continue;
		}

		// Parens?
		if (e[0] === '(') {
			var match = findParenGroup(e, 0);
			if (match) {
				var inside = match.substr(1, match.length - 2);

				tokens.push(parseExpression(inside));
				stripToken(match);
				continue;
			}

			// Unbalanced parens
			barf("Missing close paren", e);
			return;
		}

		// Error?
		barf("Can't parse next token", e);
		break;
	}

	// Check for the existence of OPERATORs, in order of precedence.
	// Add steps for these.
	for (var op of OPERATORS) {
		// Ternary colon: We intercept the '?' op instead, see below...
		if (op === ':') continue;

		for (var i = tokens.length - ((op === '?') ? 4 : 2); i >= 1; i--) {
			if (tokens[i] === op) {
				// Ternary: Merge 5 tokens into one expression, like  (a ? b : c)
				if (op === '?') {
					if (tokens[i + 2] === ':') {
						var step = addStep('ternary', tokens[i - 1], tokens[i + 1], tokens[i + 3]);
						tokens.splice(i - 1, 5, step);

					} else {
						barf("Ternary op syntax error", tokens);
						return;
					}

				} else {
					// Merge 3 tokens into 1 token: [3, "*", 2] => "step_786"
					var step = addStep(op, tokens[i - 1], tokens[i + 1]);
					tokens.splice(i - 1, 3, step);
				}
			}
		}
	}

	// Ideally this expression has been reduced to a single step, or value.
	console.log("finally:", tokens, steps);
	if (tokens.length === 1) {
		return tokens[0];
	}

	if (tokens.length > 1) {
		barf("Expression finished with >1 token, hmm", tokens);
	}
}

// Format is always like: "myVar=123.0"
//                        "myVar=(otherVar-456)"
// Returns true on success.
function parseStatement(statement)
{
	if (statement.trim().length == 0) return true;

	console.log('"' + statement + '"');
	var sides = statement.match(/^([\s\S]*?)([-+*\/%]?=)([\s\S]*)$/);

	/*
	var left = statement.split(ASSIGN_REGEX)[0];
	var right = statement.split(/=(.+)/)[1];
	*/

	if (!sides) {
		barf("Expression needs '=' assignment", statement);
		return false;
	}

	var left = sides[1];
	var right = sides[3];

	var name = left.trim();
	if (SPECIAL_VARS.hasOwnProperty(name)) {
		barf("Cannot assign to special var <b>" + name + "</b>", statement);
		return false;
	}

	// Hack to support operator assignments like: '+=', '/='
	var assign = sides[2];
	if (assign.length > 1) {
		right = left + assign[0] + '(' + right + ')';
	}

	var step = parseExpression(right);
	if (((typeof step) === 'number') || step) {
		console.log("Storing:", name, '=', step);
		varNames[name] = step;
		return true;
	}
}

// Statements are split by semicolons ';'
function parseInput(input)
{
	// reset
	steps = [];
	varNames = {};

	// Strip comments
	input = input.replace(/\/\/.*\n/g, "\n");
	input = input.replace(/\/\/.*$/g, "");

	var statementAr = input.split(/;+/g);
	for (var i = 0; i < statementAr.length; i++) {
		var success = parseStatement(statementAr[i]);
		if (!success) {
			$('#steps').css('opacity', 0.4);
			return;
		}
	}

	// Show the steps
	var table = "<table>";
	table += '<tr class="title"><td></td><td class="title_op">op</td><td colspan="3" class="title_args">args . . .</td></tr>'

	_.each(steps, function(step, i){
		table += '<tr><td class="step_name">step_' + i + '</td>';
		_.each("op,a,b,c".split(","), function(key){
			var clss = (step[key] === 0) ? "dim" : "";
			table += '<td class="'+clss+'">' + step[key] + '</td>';
		});
		table += '</tr>';
	});
	table += "</table>";

	function quoteIfString(thing) {
		if (typeof thing === 'string') return '"' + thing + '"';
		return thing;
	}

	var vs = '<p class="vars">';
	vs += _.map(Object.keys(varNames), function(key){
		return '<b>' + quoteIfString(key) + '</b> =&gt; ' + quoteIfString(varNames[key]);
	}).join('<br/>');
	vs += '</p>';

	$('#steps').html(table + vs);
	$('#steps').css('opacity', 1.0);
	$('#status').html("");	// clear errors

	sendToServer();
}

function sendToServer()
{
	// Temporarily skip expecution
	sendMessageToRing("c!");

	for (var i = 0; i < steps.length; i++) {
		var line = 's' + String.fromCharCode(33 + i);

		var op = steps[i].op;
		line += (op.length == 1) ? op : opWithName(op)['code'];

		var args = _.map(['a','b','c'], function(key){
			var thing = steps[i][key];

			if (!thing) return 0.0;

			if ((typeof thing) === 'number') return thing.toString();

			var stepMatch = thing.match(/step_(\d+)/);
			if (stepMatch) {
				return 'v' + String.fromCharCode(33 + parseInt(stepMatch[1]));
			}

			var specialMatch = thing.match(/var_([A-Z][A-Z]?)/);
			if (specialMatch) {
				return (specialMatch[1] + '_').substr(0, 2);
			}

			console.warn("Can't handle this arg:", steps[i][key]);
		});

		// Remove zeros/invalid args at the end
		while (args.length && (!args[args.length - 1])) {
			args.pop();
		}

		line += args.join(',');

		sendMessageToRing(line);
	}

	// Send the number of steps
	sendMessageToRing("c" + String.fromCharCode(33 + steps.length));
}

var _lastInput = "";
function tryParseAgain()
{
	var input = $('#input').val();
	if (input === _lastInput) return;
	_lastInput = input;
	parseInput(input);
}

function sendEnabledChange(event) {
	// Send enabled? Immediately send state
	if (isClientAvailable()) {
		gammaBrightChange(null);
		blinkChange(null);

		// tryParseAgain() will kick in every 250ms or so..
	}
}

function resetTimeClick(event) {
	sendMessageToRing("t");
}

// Gamma + brightness:
// 0bx1xxxGBB 0bx1BBBBBB
function gammaBrightChange(event) {
	var isGamma = $('#isGamma').get(0).checked;
	var bright8 = parseInt($('#bright').val());

	var b0 = 0x40 | (isGamma ? 0x04 : 0) | (bright8 >> 6);
	var b1 = 0x40 | bright8 & 0x3f;

	var msg = 'g' + String.fromCharCode(b0) + String.fromCharCode(b1);

	sendMessageToRing(msg);
}

function blinkChange(event) {
	var msg = 'b' + $('#blink').val();
	sendMessageToRing(msg);
}

function showConnectionStatus(b) {
	$('#connectionStatus')
		.text(b ? "Connected" : "Not connected")
		.toggleClass("connected", b)
		.toggleClass("notConnected", !b)
	;
}

function startSocket() {
	client = new W3CWebSocket('ws://localhost:8080/', 'echo-protocol');

	/*
	if (client) {
		client.close();
	}
	*/

	client.onerror = function() {
		//console.log('Connection Error');
		showConnectionStatus(false);
	};

	client.onopen = function() {
		console.log('WebSocket Client Connected');
		showConnectionStatus(true);
	};

	client.onclose = function() {
		console.log('echo-protocol Client Closed. Will try reconnecting in ' + RECONNECT_INTERVAL_MS + ' ms...');
		showConnectionStatus(false);

		client = null;
		setTimeout(startSocket, RECONNECT_INTERVAL_MS);
	};

	client.onmessage = function(e) {
		if (typeof e.data === 'string') {
			console.log("Received: '" + e.data + "'");
		}
	};
}

function sendStationID() {
	sendMessageToRing("i");

	setTimeout(sendStationID, 5000);
}

$(document).ready(function(){
	var ref = '<p>HANDY REFERENCE</p>';
	ref += '<p><b>OPERATORS:</b> ' + OPERATOR_REF + '</p>';
	ref += '<p><b>ASSIGNS:</b> ' + ASSIGN_REF + '</p>';
	ref += '<p><b>FUNCTIONS:</b> ' + FUNCS.join(", ") + '</p>';
	ref += '<p><b>SPECIAL VARS</b><br/>';
	ref += _.map(Object.keys(SPECIAL_VARS), function(key){
		return '<b>' + key + '</b>: ' + SPECIAL_VARS[key];
	}).join('<br/>') + '</p>';
	$('#ref').html(ref);

	// Controls
	$('#sendEnabled').on('change', sendEnabledChange);
	$('#resetTime').on('click', resetTimeClick);
	$('#isGamma').on('change', gammaBrightChange);
	$('#bright').on('input', gammaBrightChange);
	$('#blink').on('change', blinkChange);

	startSocket();

	setInterval(tryParseAgain, AUTO_PARSE_INTERVAL_MS);

	sendStationID();
});
