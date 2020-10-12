const chroma = require('chroma-js');

let nextName = 0;
const selectName = () => `Unnamed ${nextName++}`;

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
const sub = (a, b) => ['sub', a, b];
const mul = (a, b) => ['mul', a, b];
const div = (a, b) => ['div', a, b];
// const filter = (data, mask) => ['filter', data, mask];

const seq = (w) => ['seq', w];

const windowRect = (scale_0) => ['window_rect', scale_0];
const windowSimple = (scale_0) => ['window_simple', scale_0];
const conv = (a, b) => ['conv', a, b];

const ps = [];
const plot = (values, name, color) =>
	ps.push([
		'plot',
		values,
		name || selectName(),
		...chroma(color || selectColor()).gl(),
	]);

const prg = () => {
	const mid = input('bin_com.btcusdc.log_mid');
	// plot(mid, 'mid price');

	plot(conv(windowRect(f(100)), f(mid)));
	// plot(
	// 	div(conv(windowSimple(f(100)), f(mid)), f(2)),
	// );
	// plot(conv(windowRect(f(100)), mid));
};

console.log(JSON.stringify(prg() || ps));
