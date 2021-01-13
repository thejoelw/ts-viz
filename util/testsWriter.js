const glob = require('glob');
const path = require('path');

const { stringify } = require('../program/stringify');

const variant = process.argv[2];

// Generate wisdom before so our tests don't time out
let isWisdomPrepared = false;
const writePrepareWisdom = () => {
	if (!isWisdomPrepared) {
		console.log(
			`: $(BIN_TARGET) |> bash util/prepare_wisdom.bash '${variant}' '$(BIN_TARGET)' |> fftw_wisdom_float.bin fftw_wisdom_double.bin`,
		);
		isWisdomPrepared = true;
	}
};

const processJsonStream = (spec, yields) => {
	const arr = [];
	Object.entries(spec).forEach(([k, v]) => (arr[parseInt(k)] = v));

	let acc = {};
	for (let i = 0; i < arr.length; i++) {
		arr[i] = acc = { ...acc, ...arr[i] };
	}

	(yields || [])
		.sort()
		.reverse()
		.forEach((k) => arr.splice(k, 0, 'yield'));

	return serializeLines(arr);
};

const processProgram = (spec) =>
	serializeLines([stringify([['emit', 'z', spec]])]);

const serializeLines = (lines) =>
	lines
		.map(
			(line) =>
				(typeof line === 'object' ? JSON.stringify(line) : line) + '\\n',
		)
		.join('');

(async () => {
	const testFiles = await new Promise((resolve, reject) =>
		glob('tests/**/*.js', {}, (err, files) =>
			err ? reject(err) : resolve(files),
		),
	);

	const reqs = testFiles.forEach((file) => {
		const tests = require(path.resolve(file));

		tests
			.filter((test) =>
				Array.isArray(test.variant)
					? test.variant.includes(variant)
					: test.variant === variant,
			)
			.forEach(({ name, input, yields, program, output }) => {
				input = processJsonStream(input, yields);
				program = processProgram(program);
				output = processJsonStream(output);

				writePrepareWisdom();
				console.log(
					`: $(BIN_TARGET) fftw_wisdom_float.bin fftw_wisdom_double.bin |> ^ bash util/run_test.bash '${variant} - ${file} - ${name}' [arguments omitted]^ bash util/run_test.bash '${variant} - ${file} - ${name}' '$(BIN_TARGET)' '${input}' '${program}' '${output}' |>`,
				);
			});
	});
})();
