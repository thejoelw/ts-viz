const f = (num) => ['cast_float', num];
const d = (num) => ['cast_double', num];
const input = (name) => ['input', name];
const add = (a, b) => ['add', a, b];
const filter = (data, mask) => ['filter', data, mask];

const windowSimpleDouble = (scale_0) => ['window_simple_double', scale_0];

const prg = () => {
	return {
		a: windowSimpleDouble(add(f(4), d(6))),
	};

	const a = input('a');
	const b = input('b');
	const c = add(a, b);

	return { a, b, c };
};

console.log(JSON.stringify(prg()));
