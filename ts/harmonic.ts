import { Node, Window } from './types.ts';
import { conv, div, mul, sgn, shrink, sqrt, square } from './base.ts';
import { r } from './config.ts';
import { lse } from './memory.ts';
import { sum } from './vec.ts';

export const mostSignificantSignal = (
  signal: Node,
  seriesSizes: number[],
  makeSampler: (size: number) => Window,
  { lseSizeMul = 8, lseC = 0, stdSizeMul = 2, sigmas = 2, bfz = false },
) => {
  const split = seriesSizes.map((size, i) => {
    const mean = conv(makeSampler(size), signal, bfz);
    const std = sqrt(conv(makeSampler(size * stdSizeMul), square(signal), bfz));
    const norm = div(mean, std);
    return mul(norm, r(Math.pow(Math.log(size), -2)));
    const mem = lse(size * lseSizeMul, norm, lseC);
    return mem;
    const out = sgn(shrink(mem, r(sigmas)));
    return out;
    return mul(out, r(Math.pow(0.5, i)));
  });
  return sum(split);
};
