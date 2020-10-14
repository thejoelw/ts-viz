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

const input = (name) => ['input', name];

const f = (num) => ['cast_float', num];
const d = (num) => ['cast_double', num];

const sgn = (a) => ['sgn', a];
const abs = (a) => ['abs', a];
const square = (a) => ['square', a];
const sqrt = (a) => ['sqrt', a];
const exp = (a) => ['exp', a];
const log = (a) => ['log', a];

const add = (a, b) => ['add', a, b];
const sub = (a, b) => ['sub', a, b];
const mul = (a, b) => ['mul', a, b];
const div = (a, b) => ['div', a, b];
const mod = (a, b) => ['mod', a, b];
const lt = (a, b) => ['lt', a, b];
const gt = (a, b) => ['gt', a, b];
const min = (a, b) => ['min', a, b];
const max = (a, b) => ['max', a, b];
const shrink = (a, b) => ['shrink', a, b];

const windowRect = (scale_0) => ['window_rect', scale_0];
const windowSimple = (scale_0) => ['window_simple', scale_0];
const windowSmooth = (scale_0, scale_1_mult = f(2)) => [
	'window_smooth',
	scale_0,
	scale_1_mult,
];
const windowDelta = (scale_0, scale_1_mult = f(2)) => [
	'window_delta',
	scale_0,
	scale_1_mult,
];
const conv = (a, b) => ['conv', a, b];

const seq = (w) => ['seq', w];

const ps = [];
const plot = (values, name, color) =>
	ps.push([
		'plot',
		values,
		name || selectName(),
		...chroma(color || selectColor()).gl(),
	]);

const main = () => {
	const mid = input('bin_com.btcusdc.log_mid');
	plot(mid, 'mid price');

	plot(conv(windowRect(f(1000)), f(mid)));
	plot(conv(windowSimple(f(1000)), f(mid)));
	plot(conv(windowSmooth(f(500)), f(mid)));
	plot(add(f(9.3384), conv(windowDelta(f(500)), f(mid))));
};

console.log(JSON.stringify(main() || ps));
