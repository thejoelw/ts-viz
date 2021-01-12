const { input, f, d, i64 } = require('../program/base');

const r = d;

module.exports = [
	{
		name: `Test input #1`,
		variant: 'test-csl2-6',
		input: {
			0: { x: NaN },
			10: { x: 4 },
			50: { x: 5 },
			51: { x: 6 },
			200: { x: 7 },
			300: {},
		},
		program: r(input('x')),
		output: {
			0: { z: NaN },
			10: { z: 4 },
			50: { z: 5 },
			51: { z: 6 },
			200: { z: 7 },
			300: {},
		},
	},
];
