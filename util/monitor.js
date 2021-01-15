const child_process = require('child_process');
const readline = require('readline');

if (process.argv.length < 4) {
	throw new Error(
		`Usage: node ${
			process.argv[1]
		} [generate lines per second] [command] [args]...`,
	);
}

const lps = parseFloat(process.argv[2]);
const command = process.argv[3];
const args = process.argv.slice(4);

const tsViz = child_process.spawn(command, args, {
	stdio: [lps ? 'pipe' : 'inherit', 'pipe', 'inherit'],
});

const rl = readline.createInterface({
	input: tsViz.stdout,
	terminal: false,
});

let latSum = 0;
let count = 0;
const lats = [];
rl.on('line', (line) => {
	const { time, ...rest } = JSON.parse(line);
	console.log(JSON.stringify(rest));

	const latency = time ? Date.now() - time : 0;
	lats.push(latency);

	latSum += latency;
	count++;
});

const runCmd = (cmd, args) => {
	const proc = child_process.spawn(cmd, args, {
		stdio: ['pipe', 'pipe', 'inherit'],
	});

	var stdout = '';
	proc.stdout.on('data', function(data) {
		stdout += data.toString();
	});

	return new Promise((resolve, reject) => {
		proc.on('error', (err) => reject(err.message));

		proc.on('close', (code) =>
			code ? reject(`Process failed with code ${code}`) : resolve(stdout),
		);
	});
};

const psKeys = ['pcpu', 'pmem', 'rss', 'vsize'];
const getStats = (pid) =>
	Promise.all([
		runCmd('ps', ['-p', pid, '-o', psKeys.map((k) => `${k}=`).join(',')]).then(
			(out) => {
				const values = out.trim().split(/\s+/);
				if (values.length !== psKeys.length) {
					throw new Error(`Invalid ps stdout: ${out}`);
				}
				return Object.fromEntries(
					psKeys.map((k, i) => [k, parseFloat(values[i])]),
				);
			},
		),
		runCmd('vmmap', ['-summary', pid])
			.then((out) => {
				const parseMemEntry = (regex, skipCount) => {
					regex = new RegExp(
						regex.source +
							/[\d.]+[BKMGTPE]?\s+/.source.repeat(skipCount || 0) +
							/([\d.]+)([BKMGTPE])/.source,
						'gm',
					);
					const matches = [...out.matchAll(regex)];
					if (matches.length === 0) {
						throw new Error(`Can't find ${regex.source}`);
					}
					return matches.reduce((acc, [, num, unit]) => {
						switch (unit) {
							case 'E':
								num *= 1024;
							case 'P':
								num *= 1024;
							case 'T':
								num *= 1024;
							case 'G':
								num *= 1024;
							case 'M':
								num *= 1024;
							case 'K':
								num *= 1024;
							case 'B':
								num *= 1;
								break;
							default:
								throw new Error('Bad regex');
						}
						return acc + num;
					}, 0);
				};

				// const suffix = '_mb';
				// const factor = 1 / Math.pow(1024, 2);
				const suffix = '';
				const factor = 1;

				return {
					// Physical footprint:         55.2M
					[`physical_footprint${suffix}`]:
						parseMemEntry(/^Physical footprint:\s*/) * factor,

					// Writable regions: Total=117.2M written=54.4M(46%) resident=54.4M(46%) swapped_out=0K(0%) unallocated=62.8M(54%)
					[`writable_total${suffix}`]:
						parseMemEntry(/^Writable regions:.* Total=/) * factor,
					[`writable_written${suffix}`]:
						parseMemEntry(/^Writable regions:.* written=/) * factor,
					[`writable_resident${suffix}`]:
						parseMemEntry(/^Writable regions:.* resident=/) * factor,
					[`writable_swapped${suffix}`]:
						parseMemEntry(/^Writable regions:.* swapped_out=/) * factor,
					[`writable_unalloc${suffix}`]:
						parseMemEntry(/^Writable regions:.* unallocated=/) * factor,

					// Stack                             8720K      64K      64K       0K       0K       0K       0K        3
					[`stack_virtual${suffix}`]: parseMemEntry(/^Stack\s+/) * factor,
					[`stack_resident${suffix}`]: parseMemEntry(/^Stack\s+/, 1) * factor,
					[`stack_dirty${suffix}`]: parseMemEntry(/^Stack\s+/, 2) * factor,

					// TOTAL                            812.2M   236.9M    55.2M       0K       0K       0K       0K      680
					[`total_virtual${suffix}`]: parseMemEntry(/^TOTAL\s+/) * factor,
					[`total_resident${suffix}`]: parseMemEntry(/^TOTAL\s+/, 1) * factor,
					[`total_dirty${suffix}`]: parseMemEntry(/^TOTAL\s+/, 2) * factor,

					// DefaultMallocZone_0x109b5a000     126.7M      75.1M      75.1M         0K      27553      74.6M       497K      1%     154
					[`malloc_virtual${suffix}`]:
						parseMemEntry(/^DefaultMallocZone_0x[0-9a-f]+\s+/) * factor,
					[`malloc_resident${suffix}`]:
						parseMemEntry(/^DefaultMallocZone_0x[0-9a-f]+\s+/, 1) * factor,
					[`malloc_dirty${suffix}`]:
						parseMemEntry(/^DefaultMallocZone_0x[0-9a-f]+\s+/, 2) * factor,
					[`malloc_swapped${suffix}`]:
						parseMemEntry(/^DefaultMallocZone_0x[0-9a-f]+\s+/, 3) * factor,
					[`malloc_allocated${suffix}`]:
						parseMemEntry(/^DefaultMallocZone_0x[0-9a-f]+\s+/, 5) * factor,
				};
			})
			.catch((err) => ({ error: `vmmap unavailable: ${err}` })),
	]).then((objs) => Object.assign({}, ...objs));

setInterval(
	() =>
		getStats(tsViz.pid).then((stats) =>
			console.error(
				JSON.stringify({
					line_count: count,
					avg_full_latency_ms: latSum / count,
					avg_last_1k_latency_ms:
						lats.slice(-1000).reduce((acc, cur) => acc + cur, 0) /
						Math.min(1000, lats.length),
					...stats,
				}),
			),
		),
	1000,
);

if (lps) {
	const factor = -1000 / lps;
	let nextEvent = Date.now();
	setInterval(() => {
		while (nextEvent < Date.now()) {
			nextEvent += Math.log(Math.random()) * factor;
			tsViz.stdin.write(JSON.stringify({ time: Date.now() }) + '\n');
		}
	}, Math.max(1, Math.min(100 / lps, 100)));
}
