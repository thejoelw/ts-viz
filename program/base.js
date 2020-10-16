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

const cumsum = (a) => ['cumsum', a];
const cumprod = (a) => ['cumprod', a];
const fwdFillZero = (a, init) => ['fwd_fill_zero', a, init];
const subDelta = (a) => ['sub_delta', a];
const divDelta = (a) => ['div_delta', a];

const windowRect = (scale_0) => ['window_rect', scale_0];
const windowSimple = (scale_0) => ['window_simple', scale_0];
const windowSmooth = (scale_0, scale_1_mult) => [
	'window_smooth',
	scale_0,
	scale_1_mult,
];
const windowDelta = (scale_0, scale_1_mult) => [
	'window_delta',
	scale_0,
	scale_1_mult,
];
const conv = (a, b) => ['conv', a, b];

const seq = (w) => ['seq', w];

const plot = (values, name, color) => [
	'plot',
	values,
	name || selectName(),
	...chroma(color || selectColor()).gl(),
];

module.exports = {
	input,
	f,
	d,
	sgn,
	abs,
	square,
	sqrt,
	exp,
	log,
	add,
	sub,
	mul,
	div,
	mod,
	lt,
	gt,
	min,
	max,
	shrink,
	cumsum,
	cumprod,
	fwdFillZero,
	subDelta,
	divDelta,
	windowRect,
	windowSimple,
	windowSmooth,
	windowDelta,
	conv,
	seq,
	plot,
};
