import { Node } from './types.ts';
import {
  cumSumClamped,
  gt,
  gte,
  lt,
  lte,
  mul,
  sgn,
  shrink,
  sub,
  subDelta,
} from './base.ts';
import { r } from './config.ts';

export const moves = (signal: Node, shrinkAmt = 1e-5) =>
  shrinkAmt ? shrink(subDelta(signal), r(shrinkAmt)) : subDelta(signal);

export const runs = (
  signal: Node,
  resetStrength: number,
  zeroResets = false,
) => {
  const upReset = (zeroResets ? lte : lt)(signal, r(0));
  const upRuns = cumSumClamped(sub(
    gt(signal, r(0)),
    resetStrength !== 1 ? mul(r(resetStrength), upReset) : upReset,
  ));

  const downReset = (zeroResets ? gte : gt)(signal, r(0));
  const downRuns = cumSumClamped(sub(
    lt(signal, r(0)),
    resetStrength !== 1 ? mul(r(resetStrength), downReset) : downReset,
  ));

  return sub(upRuns, downRuns);
};
