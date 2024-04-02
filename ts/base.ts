import { Node, Window } from './types.ts';
import { getHash } from './stringify.ts';

const debug = false;

const errorRegex = /at (\w+) \([^)]+\/([^)]+)\)/g;

const node = (...args: [string, ...Node[]]): Node => {
  if (debug) {
    const name = `${args[0]}/${args.length - 1}`;
    const bt = new Error().stack;
    let match;
    const stack = [];
    while (match = errorRegex.exec(bt)) {
      stack.push(`${match[1]}@${match[2]}`);
    }
    args = ['meta', args, name, stack.slice(1).join(' <- ')];
  }
  return args;
};

const colors = [
  0x1f77b4,
  0xff7f0e,
  0x2ca02c,
  0xd62728,
  0x9467bd,
  0x8c564b,
  0xe377c2,
  0x7f7f7f,
  0xbcbd22,
  0x17becf,
];
let nextColor = 0;
const selectColor = () => colors[nextColor++ % colors.length];
const colorToGl = (color: number) => [
  ((color >> 16) & 0xff) / 255,
  ((color >> 8) & 0xff) / 255,
  (color & 0xff) / 255,
  0,
];

export const input = (name: string): Node => node('input', name);

export const f = (num: number | Node): Node => node('cast_float', toConst(num) ?? num);
export const d = (num: number | Node): Node => node('cast_double', toConst(num) ?? num);
export const i64 = (num: number | Node): Node => node('cast_int64', toConst(num) ?? num);

export const toConst = (x: number | Node) =>typeof x === 'number' ? x :
  x[0] === 'meta'
    ? toConst(x[1])
    : (x[0] === 'cast_float' || x[0] === 'cast_double' ||
        x[0] === 'cast_int64') &&
        typeof x[1] === 'number'
    ? x[1]
    : undefined;

export const info = (msg: string, val: Node): Node => node('info', msg, val);

export const sgn = (a: Node): Node => node('sgn', a);
export const abs = (a: Node): Node => node('abs', a);
export const inv = (a: Node): Node => node('inv', a);
export const square = (a: Node): Node => node('square', a);
export const sqrt = (a: Node): Node => node('sqrt', a);
export const cbrt = (a: Node): Node => node('cbrt', a);
export const exp = (a: Node): Node => node('exp', a);
export const log = (a: Node): Node => node('log', a);
export const expm1 = (a: Node): Node => node('expm1', a);
export const log1p = (a: Node): Node => node('log1p', a);
export const sigmoid = (a: Node): Node => node('sigmoid', a);
export const sin = (a: Node): Node => node('sin', a);
export const cos = (a: Node): Node => node('cos', a);
export const tan = (a: Node): Node => node('tan', a);
export const asin = (a: Node): Node => node('asin', a);
export const acos = (a: Node): Node => node('acos', a);
export const atan = (a: Node): Node => node('atan', a);
export const sinh = (a: Node): Node => node('sinh', a);
export const cosh = (a: Node): Node => node('cosh', a);
export const tanh = (a: Node): Node => node('tanh', a);
export const asinh = (a: Node): Node => node('asinh', a);
export const acosh = (a: Node): Node => node('acosh', a);
export const atanh = (a: Node): Node => node('atanh', a);
export const erf = (a: Node): Node => node('erf', a);
export const erfc = (a: Node): Node => node('erfc', a);
export const lgamma = (a: Node): Node => node('lgamma', a);
export const tgamma = (a: Node): Node => node('tgamma', a);
export const floor = (a: Node): Node => node('floor', a);
export const ceil = (a: Node): Node => node('ceil', a);
export const round = (a: Node): Node => node('round', a);
export const not = (a: Node): Node => node('not', a);
export const isNan = (a: Node): Node => node('is_nan', a);
export const isNum = (a: Node): Node => node('is_num', a);

export const add = (a: Node, b: Node): Node =>
  toConst(a) === 0 ? b : toConst(b) === 0 ? a : node('add', a, b);
export const sub = (a: Node, b: Node): Node =>
  toConst(b) === 0 ? a : node('sub', a, b);
export const mul = (a: Node, b: Node): Node =>
  toConst(a) === 1
    ? b
    : toConst(b) === 1
    ? a
    : a === b
    ? square(a)
    : node('mul', a, b);
export const div = (a: Node, b: Node): Node =>
  toConst(a) === 1
    ? inv(b)
    : toConst(b) !== undefined
    ? mul(a, inv(b))
    : node('div', a, b);
export const mod = (a: Node, b: Node): Node => node('mod', a, b);
export const pow = (a: Node, b: Node): Node =>
  toConst(b) === -1
    ? inv(a)
    : toConst(b) === 0.5
    ? sqrt(a)
    : toConst(b) === 1 / 3
    ? cbrt(a)
    : toConst(b) === 1
    ? a
    : toConst(b) === 2
    ? square(a)
    : node('pow', a, b);
export const atan2 = (a: Node, b: Node): Node => node('atan2', a, b);
export const lt = (a: Node, b: Node): Node => node('lt', a, b);
export const lte = (a: Node, b: Node): Node => node('lte', a, b);
export const gt = (a: Node, b: Node): Node => node('gt', a, b);
export const gte = (a: Node, b: Node): Node => node('gte', a, b);
export const eq = (a: Node, b: Node): Node => node('eq', a, b);
export const neq = (a: Node, b: Node): Node => node('neq', a, b);
export const min = (a: Node, b: Node): Node => node('min', a, b);
export const max = (a: Node, b: Node): Node => node('max', a, b);
export const shrink = (a: Node, b: Node): Node => node('shrink', a, b);
export const clamp = (a: Node, b: Node): Node => node('clamp', a, b);

export const cond = (a: Node, b: Node, c: Node): Node =>
  b === c ? b : node('cond', a, b, c);
export const triCond = (
  a: Node,
  b: Node,
  c: Node,
  d: Node,
): Node => node('tri_cond', a, b, c, d);
export const nanTo = (a: Node, b: Node): Node => node('nan_to', a, b);

export const cumSum = (a: Node): Node => node('cum_sum', a);
export const cumSumClamped = (a: Node): Node => node('cum_sum_clamped', a);
export const decayingSum = (a: Node, b: Node): Node =>
  node('decaying_sum', a, b);
export const cumProd = (a: Node): Node => node('cum_prod', a);
export const fwdFillZero = (a: Node): Node => node('fwd_fill_zero', a);
export const subDelta = (a: Node): Node => node('sub_delta', a);
export const divDelta = (a: Node): Node => node('div_delta', a);
export const monotonify = (a: Node): Node => node('monotonify', a);
export const slowPassZero = (a: Node): Node => node('slow_pass_zero', a);
export const scanIf = (a: Node, b: Node, c: Node): Node =>
  node('scan_if', a, b, c);

export const dot = (a: Node[], b: Node[]): Node => node('dot', node('arr', ...a), node('arr', ...b));

export const windowRect = (scale_0: Node): Window => {
  const width = i64(scale_0);
  return { name: 'windowRect', width, kernel: norm(toTs(inv(scale_0)), width) };
};

export const windowSimple = (scale_0: Node, precision = 1e-9): Window => {
  const width = i64(
    add(mul(f(Math.sqrt(-Math.log(precision))), scale_0), f(1.0)),
  );
  return {
    name: 'windowSimple',
    width,
    kernel: norm(gaussian(scale_0), width),
  };
};

export const windowSmooth = (
  scale_0: Node,
  scale_1_mult: Node = f(2),
  precision = 1e-9,
): Window => {
  const scale_1 = mul(scale_0, scale_1_mult);
  const width = i64(
    add(mul(f(Math.sqrt(-Math.log(precision))), max(scale_0, scale_1)), f(1.0)),
  );
  return {
    name: 'windowSmooth',
    width,
    kernel: norm(sub(gaussian(scale_0), gaussian(scale_1)), width),
  };
};

export const windowDelta = (
  scale_0: Node,
  scale_1_mult: Node = f(2),
  precision = 1e-9,
): Window => {
  const scale_1 = mul(scale_0, scale_1_mult);
  const width = i64(
    add(mul(f(Math.sqrt(-Math.log(precision))), max(scale_0, scale_1)), f(1.0)),
  );
  return {
    name: 'windowDelta',
    width,
    kernel: sub(norm(gaussian(scale_0), width), norm(gaussian(scale_1), width)),
  };
};

export const conv = (
  { name, kernel, width }: Window,
  ts: Node,
  backfillZeros: boolean = false,
): Node =>
  node('conv', kernel, ts, info(`${name} width`, width), backfillZeros);

export const norm = (a: Node, size: Node, zeroOutside = true): Node =>
  node(
    'norm',
    a,
    size,
    zeroOutside,
  );

export const delay = (a: Node, d: Node): Node =>
  toConst(d) === 0 ? a : node('delay', a, d);

export const toTs = (a: Node): Node => node('to_ts', a);
export const seq = (scale: Node): Node => node('seq', scale);
export const gaussian = (wavelength: Node): Node =>
  node('gaussian', wavelength);

export const checkEq = (a: Node, b: Node): Node => node('check_eq', a, b);

export const plot = (
  name: string,
  value: Node,
  offset = false,
  enabled = true,
  color: undefined | number = undefined,
): Node =>
  node(
    'plot',
    name,
    value,
    offset,
    enabled,
    ...colorToGl(color || selectColor()),
  );
export const drawn = (
  name: string,
  trigger: Node,
): Node => node('drawn', name, trigger);
export const drawing = (
  name: string,
  enabled = true,
  color: undefined | number = undefined,
): Node =>
  node(
    'drawing',
    name,
    enabled,
    ...colorToGl(color || selectColor()),
  );
export const emit = (key: string, value: Node): Node =>
  node('emit', key, value, getHash(value));
export const meter = (key: string, value: Node): Node =>
  node('meter', key, value);
