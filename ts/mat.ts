import { Node, Window } from './types.ts';
import { checkEq, delay, div, i64, mul, square, sub } from './base.ts';
import { r } from './config.ts';
import { zip } from './hof.ts';
import { decay } from './memory.ts';
import { assert, range } from './util.ts';
import { dot, matVecMul, norm } from './vec.ts';

export const mCov = (covWindow: Window | number, centeredSeries: Node[]) =>
  centeredSeries.map((s0, i0) =>
    centeredSeries.map((s1, i1) =>
      decay(
        covWindow,
        {
          '-1': mul(s0, s1),
          '0': square(s0),
          '1': mul(s1, s0),
        }[Math.sign(i0 - i1)]!,
      )
    )
  );

export const mPca = (
  covWindow: Window | number,
  centeredSeries: Node[],
  initialVec: Node[],
  iters = 8,
) => {
  const mat = mCov(covWindow, centeredSeries);

  return range(iters).reduce(
    (acc) => matVecMul(mat, norm(acc)),
    initialVec.map(r),
  );
};

export const mSolve = (A: Node[][], b: Node[]) => {
  assert(A.length === b.length);
  let mat = A.map((x, i) => {
    assert(x.length === b.length);
    return [...x, b[i]];
  });

  const ZERO = {} as Node;
  const ONE = {} as Node;

  for (let i = 0; i < b.length; i++) {
    const pivot = mat[i];
    mat = mat.map((row, j) =>
      i !== j
        ? zip(row, pivot, (rc, pc, k) => {
          if (k === i) {
            return ZERO;
          }
          if (pc === ZERO) {
            return rc;
          }
          return sub(rc, mul(pc, div(row[i], pivot[i])));
        })
        : row.map((rc, k) => {
          if (k === i) {
            return ONE;
          }
          if (rc === ZERO) {
            return ZERO;
          }
          return div(rc, pivot[i]);
        })
    );
  }

  mat.forEach((row, i) =>
    row.forEach((cell, j) =>
      assert(j === b.length || cell === (i === j ? ONE : ZERO))
    )
  );

  return mat.map((row) => row[b.length]);
};

export const mSolveTest = () => {
  const size = 4;
  const A = range(size).map(() =>
    range(size).map(() => r(Math.random() - 0.5))
  );
  const b = range(size).map(() => r(Math.random() - 0.5));
  const c = A.map((a) => dot(a, b));
  return range(size).map((i) => checkEq(b[i], mSolve(A, c)[i]));
};

export const mPredict = (
  centeredXs: Node[],
  y: Node,
  { covWindow, delayTicks }: { covWindow: Window | number; delayTicks: number },
) => {
  const train = centeredXs.map((x) => delay(x, i64(delayTicks)));

  const mat = mCov(covWindow, [y, ...train]);
  const coefs = mSolve(
    mat.slice(1).map((r) => r.slice(1)),
    mat.slice(1).map((r) => r[0]),
  );

  return coefs;
  // return dot(centeredXs, coefs);
};
