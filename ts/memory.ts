import { Node, Window } from './types.ts';
import { add, conv, decayingSum, div, exp, inv, log, mul } from './base.ts';
import { r } from './config.ts';

export const decay = (
  window: number | Window | Node,
  data: Node,
  backfillZeros?: boolean,
) =>
  typeof window === 'number'
    ? decayingSum(data, r(1 / window))
    : window instanceof Array
    ? decayingSum(data, window)
    : conv(window, data, backfillZeros);

export const lse = (
  window: number | Window | Node,
  data: Node,
  c: Node | number,
  backfillZeros?: boolean,
) =>
  c
    ? mul(log(decay(window, exp(mul(data, r(c))), backfillZeros)), inv(r(c)))
    : decay(window, data, backfillZeros);

export const persist = (
  window: number | Window | Node,
  trigger: Node,
  value: Node,
  eps = 1e-9,
  backfillZeros?: boolean,
) =>
  div(
    decay(window, mul(trigger, value), backfillZeros),
    add(r(eps), decay(window, trigger, backfillZeros)),
  );

export const persistLse = (
  window: number | Window | Node,
  trigger: Node,
  value: Node,
  c: Node | number,
  eps = 1e-9,
  backfillZeros?: boolean,
) =>
  c
    ? mul(
      log(persist(window, trigger, exp(mul(value, r(c))), eps, backfillZeros)),
      inv(r(c)),
    )
    : persist(window, trigger, value, eps, backfillZeros);
