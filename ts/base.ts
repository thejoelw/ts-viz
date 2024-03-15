import { Node, Window } from './types.ts';

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

export const input = (name: string): Node => ['input', name];

export const f = (num: number | Node): Node => ['cast_float', num];
export const d = (num: number | Node): Node => ['cast_double', num];
export const i64 = (num: number | Node): Node => ['cast_int64', num];

const toConst = (x: Node) =>
  (x[0] === 'cast_float' || x[0] === 'cast_double' || x[0] === 'cast_int64') &&
    typeof x[1] === 'number'
    ? x[1]
    : undefined;

export const info = (msg: string, val: Node): Node => ['info', msg, val];

export const arr = (...args: Node[]): Node => ['arr', ...args];

export const sgn = (a: Node): Node => ['sgn', a];
export const abs = (a: Node): Node => ['abs', a];
export const inv = (a: Node): Node => ['inv', a];
export const square = (a: Node): Node => ['square', a];
export const sqrt = (a: Node): Node => ['sqrt', a];
export const cbrt = (a: Node): Node => ['cbrt', a];
export const exp = (a: Node): Node => ['exp', a];
export const log = (a: Node): Node => ['log', a];
export const expm1 = (a: Node): Node => ['expm1', a];
export const log1p = (a: Node): Node => ['log1p', a];
export const sigmoid = (a: Node): Node => ['sigmoid', a];
export const sin = (a: Node): Node => ['sin', a];
export const cos = (a: Node): Node => ['cos', a];
export const tan = (a: Node): Node => ['tan', a];
export const asin = (a: Node): Node => ['asin', a];
export const acos = (a: Node): Node => ['acos', a];
export const atan = (a: Node): Node => ['atan', a];
export const sinh = (a: Node): Node => ['sinh', a];
export const cosh = (a: Node): Node => ['cosh', a];
export const tanh = (a: Node): Node => ['tanh', a];
export const asinh = (a: Node): Node => ['asinh', a];
export const acosh = (a: Node): Node => ['acosh', a];
export const atanh = (a: Node): Node => ['atanh', a];
export const erf = (a: Node): Node => ['erf', a];
export const erfc = (a: Node): Node => ['erfc', a];
export const lgamma = (a: Node): Node => ['lgamma', a];
export const tgamma = (a: Node): Node => ['tgamma', a];
export const floor = (a: Node): Node => ['floor', a];
export const ceil = (a: Node): Node => ['ceil', a];
export const round = (a: Node): Node => ['round', a];
export const not = (a: Node): Node => ['not', a];
export const isNan = (a: Node): Node => ['is_nan', a];
export const isNum = (a: Node): Node => ['is_num', a];

export const add = (a: Node, b: Node): Node =>
  toConst(a) === 0 ? b : toConst(b) === 0 ? a : ['add', a, b];
export const sub = (a: Node, b: Node): Node =>
  toConst(b) === 0 ? a : ['sub', a, b];
export const mul = (a: Node, b: Node): Node =>
  toConst(a) === 1
    ? b
    : toConst(b) === 1
    ? a
    : a === b
    ? square(a)
    : ['mul', a, b];
export const div = (a: Node, b: Node): Node =>
  toConst(a) === 1
    ? inv(b)
    : toConst(b) !== undefined
    ? mul(a, inv(b))
    : ['div', a, b];
export const mod = (a: Node, b: Node): Node => ['mod', a, b];
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
    : ['pow', a, b];
export const atan2 = (a: Node, b: Node): Node => ['atan2', a, b];
export const lt = (a: Node, b: Node): Node => ['lt', a, b];
export const lte = (a: Node, b: Node): Node => ['lte', a, b];
export const gt = (a: Node, b: Node): Node => ['gt', a, b];
export const gte = (a: Node, b: Node): Node => ['gte', a, b];
export const eq = (a: Node, b: Node): Node => ['eq', a, b];
export const neq = (a: Node, b: Node): Node => ['neq', a, b];
export const min = (a: Node, b: Node): Node => ['min', a, b];
export const max = (a: Node, b: Node): Node => ['max', a, b];
export const shrink = (a: Node, b: Node): Node => ['shrink', a, b];
export const clamp = (a: Node, b: Node): Node => ['clamp', a, b];

export const cond = (a: Node, b: Node, c: Node): Node =>
  b === c ? b : ['cond', a, b, c];
export const triCond = (
  a: Node,
  b: Node,
  c: Node,
  d: Node,
): Node => ['tri_cond', a, b, c, d];
export const nanTo = (a: Node, b: Node): Node => ['nan_to', a, b];

export const cumSum = (a: Node): Node => ['cum_sum', a];
export const cumSumClamped = (a: Node): Node => ['cum_sum_clamped', a];
export const decayingSum = (a: Node, b: Node): Node => ['decaying_sum', a, b];
export const cumProd = (a: Node): Node => ['cum_prod', a];
export const fwdFillZero = (a: Node): Node => ['fwd_fill_zero', a];
export const subDelta = (a: Node): Node => ['sub_delta', a];
export const divDelta = (a: Node): Node => ['div_delta', a];
export const monotonify = (a: Node): Node => ['monotonify', a];
export const slowPassZero = (a: Node): Node => ['slow_pass_zero', a];
export const scanIf = (a: Node, b: Node, c: Node): Node => ['scan_if', a, b, c];

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
): Node => ['conv', kernel, ts, info(`${name} width`, width), backfillZeros];

export const norm = (a: Node, size: Node, zeroOutside = true): Node => [
  'norm',
  a,
  size,
  zeroOutside,
];

export const delay = (a: Node, d: Node): Node => ['delay', a, d];

export const toTs = (a: Node): Node => ['to_ts', a];
export const seq = (scale: Node): Node => ['seq', scale];
export const gaussian = (wavelength: Node): Node => ['gaussian', wavelength];

export const checkEq = (a: Node, b: Node): Node => ['check_eq', a, b];

export const plot = (
  name: string,
  value: Node,
  offset = false,
  enabled = true,
  color: undefined | number = undefined,
): Node => [
  'plot',
  name,
  value,
  offset,
  enabled,
  ...colorToGl(color || selectColor()),
];
export const drawn = (
  name: string,
  trigger: Node,
): Node => ['drawn', name, trigger];
export const drawing = (
  name: string,
  enabled = true,
  color: undefined | number = undefined,
): Node => [
  'drawing',
  name,
  enabled,
  ...colorToGl(color || selectColor()),
];
export const emit = (key: string, value: Node): Node => ['emit', key, value];
export const meter = (key: string, value: Node): Node => ['meter', key, value];
