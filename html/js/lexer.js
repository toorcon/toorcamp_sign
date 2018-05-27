// In order of precedence
const OPERATORS = "*/+-";

// Ops will run in order
// Value is object like:  (a,b,c are arguments)
//   {op: "add", a: "step_0", b: 123.4, c: 0}
var steps = [];

// Key: "myVar", value: "step_0"
var varNames = {};

// Returns index step
function addStep(op, a, b, c) {
	var idx = steps.length;
	steps.push({op: op, a: a || 0, b: b || 0, c: c || 0});
	return "step_" + idx;
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

		// Parens?
		if (e[0] === '(') {
			// You can match balanced parens with a regex, but it's a known hard problem.
			// Here's a simpler way: Count paren pairs.
			var depth = 0;
			for (var i = 0; i < e.length; i++) {
				if (e[i] === '(') depth++;
				if (e[i] === ')') depth--;

				if (depth === 0) {
					var match = e.substr(0, i + 1);
					var inside = e.substr(1, match.length - 2);

					tokens.push(parseExpression(inside));
					stripToken(match);
					break;
				}
			}

			// Success
			if (depth == 0) {
				continue;
			}

			// Unbalanced parens
			console.warn("Open paren detected:", e);
			return;
		}

		// Error?
		console.warn("Can't parse next token:", e);
		break;
	}

	// Check for the existence of OPERATORs, in order of precedence.
	// Add steps for these.
	_.each(OPERATORS.split(""), function(op){
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
		console.warn("Expression finished with >1 token, hmm:", tokens);
	}
}

// Format is always like: "myVar=123.0"
//                        "myVar=(otherVar-456)"
function parseStatement(statement)
{
	var sides = statement.split(/=/);
	if (sides.length !== 2) return;

	var name = sides[0];
	var opIndex = parseExpression(sides[1]);
	varNames[name]
}

// Statements are split by semicolons ';'
function parseInput(input)
{
	// Remove all whitespace. We don't need it
	input = input.replace(/[\s\n]/g, '');

	// reset
	steps = [];
	varNames = {};

	var statementAr = input.split(/;+/g);
	_.each(statementAr, function(statement){
		parseStatement(statement);
	});
}

var _lastInput = "";
function tryParseAgain()
{
	var input = $('#input').val();
	if (input === _lastInput) return;
	_lastInput = input;
	parseInput(input);
}

setInterval(tryParseAgain, 1000);
