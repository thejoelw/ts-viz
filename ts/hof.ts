import { Node } from './types.ts';
import { assert } from './util.ts';

export const binaryReduce = <T>(
  op: (a: T, b: T) => T,
  vec: T[],
): T => {
  assert(vec.length > 0);
  if (vec.length === 1) {
    return vec[0];
  } else {
    const half = Math.floor(vec.length / 2);
    return op(
      binaryReduce(op, vec.slice(0, half)),
      binaryReduce(op, vec.slice(half)),
    );
  }
};

export const zip = (
  a: Node[],
  b: Node[],
  func: (a: Node, b: Node, i: number) => Node,
) => {
  assert(a.length === b.length);
  return a.map((_, i) => func(a[i], b[i], i));
};
