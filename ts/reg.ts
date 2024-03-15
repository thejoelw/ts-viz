import { Node, Window } from './types.ts';
import {
  add,
  conv,
  div,
  gt,
  mul,
  seq,
  sqrt,
  square,
  sub,
  windowDelta,
  windowSmooth,
} from './base.ts';
import { r } from './config.ts';

export const normMean = (x: Node, meanWindow: Window) => conv(meanWindow, x);
export const normStd = (x: Node, meanWindow: Window, stdWindow: Window) =>
  sqrt(conv(stdWindow, square(sub(x, normMean(x, meanWindow)))));
export const normVal = (
  x: Node,
  meanWindow: Window,
  stdWindow: Window,
  eps: number,
) =>
  div(
    sub(x, normMean(x, meanWindow)),
    add(normStd(x, meanWindow, stdWindow), r(eps)),
  );
export const deltaEnergy = (x: Node, deltaSize: number, smoothWindow: Window) =>
  sqrt(conv(smoothWindow, square(conv(windowDelta(r(deltaSize)), x))));
export const corr = (x: Node, y: Node, smoothWindow: Window) =>
  conv(smoothWindow, mul(x, y));
export const aboveMeanRatio = (
  x: Node,
  meanWindow: Window,
  smoothWindow: Window,
) => conv(smoothWindow, gt(x, conv(meanWindow, x)));

// From http://www.pgccphy.net/Linreg/linreg.pdf
// y = mx + b
export const regSs = (x: Node, y: Node, makeMeanSampler: (x: Node) => Node) => {
  const S_xx = sub(makeMeanSampler(square(x)), square(makeMeanSampler(x)));
  const S_xy = sub(
    makeMeanSampler(mul(x, y)),
    mul(makeMeanSampler(x), makeMeanSampler(y)),
  );
  const S_yy = sub(makeMeanSampler(square(y)), square(makeMeanSampler(y)));
  return { S_xx, S_xy, S_yy };
};
export const regM = (x: Node, y: Node, makeMeanSampler: (x: Node) => Node) => {
  const { S_xx, S_xy } = regSs(x, y, makeMeanSampler);
  return div(S_xy, S_xx);
};
export const regB = (x: Node, y: Node, makeMeanSampler: (x: Node) => Node) => {
  const { S_xx } = regSs(x, y, makeMeanSampler);
  const top = sub(
    mul(makeMeanSampler(y), makeMeanSampler(square(x))),
    mul(makeMeanSampler(x), makeMeanSampler(mul(x, y))),
  );
  return div(top, S_xx);
};
export const regR = (x: Node, y: Node, makeMeanSampler: (x: Node) => Node) => {
  const { S_xx, S_xy, S_yy } = regSs(x, y, makeMeanSampler);
  const top = S_xy;
  const bottom = sqrt(mul(S_xx, S_yy));
  return div(top, bottom);
};
export const regRsq = (
  x: Node,
  y: Node,
  makeMeanSampler: (x: Node) => Node,
) => {
  const { S_xx, S_xy, S_yy } = regSs(x, y, makeMeanSampler);
  return div(square(S_xy), mul(S_xx, S_yy));
};
export const regResidualStd = (
  x: Node,
  y: Node,
  makeMeanSampler: (x: Node) => Node,
) => {
  const { S_xx, S_xy, S_yy } = regSs(x, y, makeMeanSampler);
  return sqrt(sub(S_yy, div(square(S_xy), S_xx)));
};

export const reg = (
  regSize: number,
  data: Node,
  offset = 0,
  sampler: (s: Node) => Node = (s: Node) =>
    conv(windowSmooth(r(regSize), r(2)), s),
) => {
  const tr = r;

  const x = seq(tr(1e-6));
  const m = regM(x, tr(data), sampler);
  const b = regB(x, tr(data), sampler);
  const y = add(mul(m, offset ? add(x, tr(1e-6 * offset)) : x), b);
  return { x: r(x), m: r(m), b: r(b), y: r(y), sampler };
};
