const {
	input,
	f,
	d,
	i64,
	toTs,
	arr,
	norm,
	conv,
	windowRect,
} = require('../program/base');

const r = d;

const kernel = (spec) => {
	const els = [];
	Object.entries(spec).forEach(([k, v]) => (els[k] = v));
	for (let i = 0; i < els.length; i++) {
		els[i] = r(els[i] || 0);
	}
	const width = i64(els.length);
	return {
		width,
		kernel: norm(toTs(arr(...els)), width),
	};
};

module.exports = [
	...[
		...[2, 3, 4, 5, 6, 7, 8, 9],
		...[10, 15, 16, 17, 31, 32, 33],
		...[63, 64, 65, 100, 127, 128, 129, 255, 256, 257],
	].map((n) => ({
		name: `Test spike input and kernelSize=${n}`,
		variant: ['test-csl2-6', 'debug', 'release'],
		input: { 0: { x: 0 }, 10: { x: 1 }, 11: { x: 0 }, 300: {} },
		program: conv(windowRect(r(n)), r(input('x')), true),
		output: { 0: { z: 0 }, 10: { z: 1 / n }, [10 + n]: { z: 0 }, 300: {} },
	})),

	{
		name: `Test kernel spikes`,
		variant: ['test-csl2-6', 'debug', 'release'],
		input: { 0: { x: 0 }, 10: { x: 1 }, 11: { x: 0 }, 300: {} },
		program: conv(kernel({ 7: 0.4, 14: 0.6 }), r(input('x')), true),
		output: {
			0: { z: 0 },
			17: { z: 0.4 },
			18: { z: 0 },
			24: { z: 0.6 },
			25: { z: 0 },
			300: {},
		},
	},
];
