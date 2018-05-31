const $ = require("jquery");
const _ = require("lodash");
const W3CWebSocket = require('websocket').w3cwebsocket;

const MAX_CODE_STEPS = 100;

const PORT = 8080;

// In order of precedence
const OPERATORS = "*,/,%,+,-,<,<=,>,>=,==,!=,?,:".split(",");

const OPERATOR_REF = "* / % + - < <= > >= == != ?:";

const ASSIGN_REF = '= += -= *= /= %=';

const FUNCS = "sin,cos,tan,pow,abs,atan2,floor,ceil,round,frac,sqrt,log,logBase,rand,randRange,min,max,lerp,clamp,tri,uni2bi,bi2uni,ternary".split(",").sort();

// All operations/functions must be sent as single-char. These are overrides:
const OP_SERIAL_CHARS = {
	'<=': '{',
	'>=': '}',
	'==': '=',
	'!=': '!',
	'ternary': '?',
	'sin': 'S',
	'cos': 'C',
	'tan': 'T',
	'pow': 'P',
	'abs': 'a',
	'atan2': '2',
	'floor': 'f',
	'ceil': 'c',
	'round': 'r',
	'frac': '.',
	'sqrt': 'Q',
	'log': 'L',
	'logBase': 'B',
	'rand': 'z',
	'randRange': 'Z',
	'min': 'm',
	'max': 'M',
	'lerp': 'p',
	'clamp': 'x',
	'tri': '3',
	'uni2bi': 'b',
	'bi2uni': 'u'
};

// FIXME: check for duplicate chars

const SPECIAL_VARS = {
	"T": "time, in seconds",
	"L": "letter id: T=0, O=1, O=2, R=3, ...",
	"I": "index of LED on strand",
	"C": "number of LEDs on strand",
	"P": "ratio of LED on strand (==I/C)",
	"X": "global X position",
	"Y": "global Y position",
	"LX": "X position within the letter",
	"A": "angle that LED is facing (theta)",
	"GA": "global angle (from center of sign)",
	"LA": "local angle (from center of letter)",
	"IN": "true if inside letter (hole)",
	"OT": "true if outside letter (outer edge)",
};

// Code will execute in order
// Value is object like:  (a,b,c are arguments)
//   {op: "+", a: "step_0", b: 123.4, c: 0}
var steps = [];

// Key: "myVar", value: "step_0"
var varNames = {};

var client = null;

function isClientOpen() {
	return client && (client.readyState === WebSocket.OPEN);
}

function barf(reason, expr) {
	var html = '<p class="barf">Error: <b>';
	html += reason + '</b></p>';
	html += '<blockquote class="barf">' + expr + '</blockquote>';
	$('#steps').html(html);

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
	// You can match balanced parens with a regex, but it's a known hard problem.
	// Here's a simpler way: Count paren pairs.
	var depth = 0;

	for (var i = fromIdx; i < str.length; i++) {
		if (str[i] === '(') depth++;
		if (str[i] === ')') depth--;

		if (depth === 0) {
			return str.substr(fromIdx, i + 1);
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
		if ((depth > 0) && (str[i] !== ',')) {
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
				var args = splitArgsInParens(parenGroup);

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
	if (step) {
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

	var statementAr = input.split(/;+/g);
	for (var i = 0; i < statementAr.length; i++) {
		var success = parseStatement(statementAr[i]);
		if (!success) return;
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

	sendToServer();
}

function sendToServer()
{
	// Temporarily skip expecution
	if (isClientOpen()) {
		client.send("c!\n");
	}

	for (var i = 0; i < steps.length; i++) {
		var line = 's' + String.fromCharCode(33 + i);

		var op = steps[i].op;
		line += (op.length == 1) ? op : OP_SERIAL_CHARS[op];

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
		line += "\n";

		console.log(line);

		if (isClientOpen()) {
			client.send(line);
		}
	}

	// Send the number of steps
	if (isClientOpen()) {
		client.send("c" + String.fromCharCode(33 + steps.length) + "\n");
	}

}

var _lastInput = "";
function tryParseAgain()
{
	var input = $('#input').val();
	if (input === _lastInput) return;
	_lastInput = input;
	parseInput(input);
}

function portButtonClick(event) {
	console.log("portButtonClick()");
}

function startSocket() {
	client = new W3CWebSocket('ws://localhost:8080/', 'echo-protocol');

	client.onerror = function() {
		console.log('Connection Error');
	};

	client.onopen = function() {
		console.log('WebSocket Client Connected');
	};

	client.onclose = function() {
		console.log('echo-protocol Client Closed');
	};

	client.onmessage = function(e) {
		if (typeof e.data === 'string') {
			console.log("Received: '" + e.data + "'");
		}
	};
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

	$('#port button').on('click', portButtonClick);

	startSocket();

	setInterval(tryParseAgain, 500);
});
