const { input, f, d, i64, toTs, arr, norm } = require('../program/base');

const r = d;

module.exports = [
	{
		name: `Test arr #1`,
		variant: 'test-csl2-6',
		input: { 0: { x: 0 }, 300: {} },
		program: toTs(arr(r(5), r(6), r(7), r(8), r(9))),
		output: {
			0: { z: 5 },
			1: { z: 6 },
			2: { z: 7 },
			3: { z: 8 },
			4: { z: 9 },
			5: { z: 0 },
			300: {},
		},
	},
];
