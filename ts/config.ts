import { Node } from './types.ts';
import { add, d, mul } from './base.ts';

export const r = d;
export const raise = (x: Node, m: number, yOffset = 10.3) =>
  add(mul(x, r(m)), r(yOffset));
