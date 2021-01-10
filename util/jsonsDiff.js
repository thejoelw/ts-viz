const fs = require('fs');
const readline = require('readline');

if (process.argv.length !== 4) {
	throw new Error(`Usage: node ${process.argv[1]} [file A] [file B]`);
}

const EPSILON = 1e-9;

const isFuzzyEqual = (a, b) => {
	if (typeof a === 'number' && typeof b === 'number') {
		return (isNaN(a) && isNaN(b)) || Math.abs(a - b) < EPSILON;
	} else if (typeof a === 'object' && typeof b === 'object') {
		for (key in a) {
			if (!isFuzzyEqual(a[key], b[key])) {
				return false;
			}
		}
		for (key in b) {
			if (!isFuzzyEqual(a[key], b[key])) {
				return false;
			}
		}
		return true;
	} else {
		return a === b;
	}
};

const cmpLines = (index, a, b) => {
	if (!isFuzzyEqual(a, b)) {
		a && console.log('\x1b[31m%s\x1b[0m', `${index}: - ${JSON.stringify(a)}`);
		b && console.log('\x1b[32m%s\x1b[0m', `${index}: + ${JSON.stringify(b)}`);
		process.exitCode = 1;
	}
};

const lines = [[], []];
const filePromises = [process.argv[2], process.argv[3]].map(
	(filename, fileIndex) => {
		const rl = readline.createInterface({
			input: fs.createReadStream(filename),
			terminal: false,
		});

		let lineIndex = 0;
		rl.on('line', (line) => {
			const data = JSON.parse(line);
			lines[fileIndex][lineIndex] = data;
			if (lines[1 - fileIndex].length > lineIndex) {
				cmpLines(lineIndex, lines[0][lineIndex], lines[1][lineIndex]);
			}

			lineIndex++;
		});

		return new Promise((resolve) => rl.on('close', resolve));
	},
);

Promise.all(filePromises).then(() => {
	const min = Math.min(lines[0].length, lines[1].length);
	const max = Math.max(lines[0].length, lines[1].length);
	for (let i = min; i < max; i++) {
		cmpLines(i, lines[0][i], lines[1][i]);
	}
});
