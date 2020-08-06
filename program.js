const chroma = require('chroma-js');

const colors = [
	'#1f77b4',
	'#ff7f0e',
	'#2ca02c',
	'#d62728',
	'#9467bd',
	'#8c564b',
	'#e377c2',
	'#7f7f7f',
	'#bcbd22',
	'#17becf',
];
let nextColor = 0;
const selectColor = () => colors[nextColor++ % colors.length];

const f = (num) => ['cast_float', num];
const d = (num) => ['cast_double', num];
const input = (name) => ['input', name];
const add = (a, b) => ['add', a, b];
// const filter = (data, mask) => ['filter', data, mask];

const seq = (w) => ['seq', w];

const windowSimpleDouble = (scale_0) => ['window_simple_double', scale_0];

const plot = (x, y, color) => [
	'plot',
	x,
	y,
	['color', ...chroma(color || selectColor()).gl()],
];

const prg = () => {
	return {
		a: windowSimpleDouble(add(f(4), d(10))),
	};

	const a = input('a');
	const b = input('b');
	const c = add(a, b);

	return { a, b, c };
};

console.log(JSON.stringify(prg()));
