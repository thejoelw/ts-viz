const child_process = require('child_process');
const readline = require('readline');

if (process.argv.length !== 5) {
	throw new Error(
		`Usage: node ${process.argv[1]} [binary] [program file] [lines per second]`,
	);
}

const binary = process.argv[2];
const programFile = process.argv[3];
const lps = process.argv[4];

const tsViz = child_process.spawn(
	binary,
	[
		// '--require-existing-wisdom',
		'--dont-write-wisdom',
		'--log-level',
		'warn',
		programFile,
		'-',
	],
	{ stdio: ['pipe', 'pipe', 'inherit'] },
);

const rl = readline.createInterface({
	input: tsViz.stdout,
	terminal: false,
});

const start = Date.now();
let lastLog = Date.now();
let sum = 0;
let count = 0;
const lats = [];
rl.on('line', (line) => {
	const data = JSON.parse(line);
	const latency = Date.now() - start - data.time;

	sum += latency;
	count += 1;

	lats.push(latency);
	const avg1k =
		lats.slice(-1000).reduce((acc, cur) => acc + cur, 0) /
		Math.min(1000, lats.length);

	if (Date.now() > lastLog + 1000) {
		console.log(
			`avgFullLatencyMs=${sum /
				count}, avgLast1kLatencyMs=${avg1k} count=${count}`,
		);
		lastLog = Date.now();
	}
});

const factor = -1000 / lps;
let nextEvent = Date.now();
setInterval(() => {
	while (nextEvent < Date.now()) {
		nextEvent += Math.log(Math.random()) * factor;
		tsViz.stdin.write(JSON.stringify({ time: Date.now() - start }) + '\n');
	}
}, Math.max(1, Math.min(100 / lps, 100)));
