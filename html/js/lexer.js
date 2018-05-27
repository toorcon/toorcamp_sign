// In order of precedence
const OPERATORS = "*,/,+,-".split(",");
const FUNCS = "sin,cos,tan,pow,abs,atan2,floor,ceil,round,sqrt,log,rand,randRange,randi,randRangei,min,max,lerp,clamp,tri,uni2bi,bi2uni".split(",").sort();

// Code will execute in order
// Value is object like:  (a,b,c are arguments)
//   {op: "+", a: "step_0", b: 123.4, c: 0}
var steps = [];

// Key: "myVar", value: "step_0"
var varNames = {};

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

	if (steps.length > 10) {
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
		var cIdx = OPERATORS.indexOf(e[0]);
		if (cIdx >= 0) {
			tokens.push(e[0]);
			stripToken(e[0]);
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

		// Var name / "step_234" ?
		var nameMatch = e.match(/^[a-zA-Z_\-]+/);
		if (nameMatch) {
			var dest = varNames[nameMatch[0]];
			console.log("checking for:", nameMatch[0], dest, varNames);
			if (dest) {
				tokens.push(dest);
				stripToken(nameMatch[0]);
				continue;
			}

			barf("Unknown var name", e);
			return;
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
	_.each(OPERATORS, function(op){
		for (var i = tokens.length - 2; i >= 1; i--) {
			// Merge 3 tokens into 1 token: [3, "*", 2] => "step_786"
			if (tokens[i] === op) {
				var step = addStep(op, tokens[i - 1], tokens[i + 1]);
				tokens.splice(i - 1, 3, step);
			}
		}
	});

	// Ideally this expression has been reduced to a single step, or value.
	console.log("finally:", tokens, steps);
	if (tokens.length === 1) {
		return tokens[0];
	}

	if (tokens.length > 1) {
		barf("Expression finished with >1 token, hmm: " + tokens);
	}
}

// Format is always like: "myVar=123.0"
//                        "myVar=(otherVar-456)"
// Returns true on success.
function parseStatement(statement)
{
	if (statement.trim().length == 0) return true;

	var sides = statement.split(/=/);
	if (sides.length !== 2) return false;

	var name = sides[0].trim();
	var step = parseExpression(sides[1]);
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
}

var _lastInput = "";
function tryParseAgain()
{
	var input = $('#input').val();
	if (input === _lastInput) return;
	_lastInput = input;
	parseInput(input);
}

$(document).ready(function(){
	var ref = '<p>HANDY REFERENCE</p>';
	ref += '<p><b>OPERATORS:</b> ' + OPERATORS.join(" ") + '</p>';
	ref += '<p><b>FUNCTIONS:</b> ' + FUNCS.join(", ") + '</p>';
	$('#ref').html(ref);

	setInterval(tryParseAgain, 500);
});
