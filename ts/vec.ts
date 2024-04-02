import { Node } from './types.ts';
import { add, arr, div, max, min, mul, sqrt, square, dot as baseDot } from './base.ts';
import { r } from './config.ts';
import { binaryReduce, zip } from './hof.ts';
import { assert } from './util.ts';

export const vAdd = (vec: Node[]) => binaryReduce(add, vec);
export const vMin = (vec: Node[]) => binaryReduce(min, vec);
export const vMax = (vec: Node[]) => binaryReduce(max, vec);

export const sum = (vec: Node[]) => binaryReduce(add, vec);
export const avg = (vec: Node[]) => div(binaryReduce(add, vec), r(vec.length));
export const dot = (a: Node[], b: Node[]) => {
  assert(a.length === b.length);
  // return sum(zip(a, b, mul));
  return baseDot(a, b);
};
export const mag = (vec: Node[]) => sqrt(sum(vec.map(square)));
export const norm = (vec: Node[]) => {
  const quot = mag(vec);
  return vec.map((el) => div(el, quot));
};
export const matVecMul = (mat: Node[][], vec: Node[]) =>
  mat.map((row) => dot(row, vec));
