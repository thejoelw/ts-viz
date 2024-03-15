import { Node } from './types.ts';
import { conv, decayingSum, div, gt, max, min, windowSmooth } from './base.ts';
import { r } from './config.ts';
import { binaryReduce } from './hof.ts';
import { reg } from './reg.ts';
import { sum } from './vec.ts';

export const bandsSmootherConv = (data: Node, size: number) =>
  conv(windowSmooth(r(size), r(2)), data);
export const bandsSmootherDs = (data: Node, size: number) =>
  decayingSum(data, r(1 / (Math.E * size)));
export const bandsSmootherRegConv = (data: Node, size: number, offset = 0) =>
  reg(size, data, offset).y;
export const bandsSmootherRegDs = (data: Node, size: number, offset = 0) =>
  reg(size, data, offset, (s: Node) => decayingSum(s, r(1 / (Math.E * size))))
    .y;

export const bands = (
  data: Node,
  xScale = 32e4,
  makeSmoother: (data: Node, size: number) => Node = bandsSmootherConv,
) => {
  const ys = [
    -5,
    -4.5,
    -4,
    -3.5,
    -3,
    -2.5,
    -2,
    -1.5,
    -1,
    -0.7,
    -0.4,
    -0.2,
    -0.1,
    0,
  ].map((i) => makeSmoother(data, xScale * Math.pow(2, i)));

  return {
    ys,
    yMin: binaryReduce(min, ys),
    yMax: binaryReduce(max, ys),
    onlineRatio: div(sum(ys.map((y) => gt(y, r(-1e9)))), r(ys.length)),
  };
};
