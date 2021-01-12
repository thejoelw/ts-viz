const glob = require('glob');
const path = require('path');

const { stringify } = require('../program/stringify');

const variant = process.argv[2];

const processJsonStream = (spec) => {
	const arr = [];
	Object.entries(spec).forEach(([k, v]) => (arr[k] = v));
	let acc = {};
	for (let i = 0; i < arr.length; i++) {
		arr[i] = acc = { ...acc, ...arr[i] };
	}
	return serializeLines(arr.map((x) => JSON.stringify(x)));
};

const processProgram = (spec) =>
	serializeLines([stringify([['emit', 'z', spec]])]);

const serializeLines = (lines) => lines.map((line) => line + '\\n').join('');

(async () => {
	const testFiles = await new Promise((resolve, reject) =>
		glob('tests/**/*.js', {}, (err, files) =>
			err ? reject(err) : resolve(files),
		),
	);

	const reqs = testFiles.forEach((file) => {
		const tests = require(path.resolve(file));

		tests
			.filter((test) => test.variant === variant)
			.forEach(({ name, input, program, output }) => {
				input = processJsonStream(input);
				program = processProgram(program);
				output = processJsonStream(output);

				console.log(
					`: $(BIN_TARGET) |> bash util/run_test.bash '${file} - ${name}' '$(BIN_TARGET)' '${input}' '${program}' '${output}' |>`,
				);
			});
	});
})();
