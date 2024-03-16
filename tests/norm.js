import { arr, d, i64, norm, toTs } from '../ts/base.ts';

const r = d;

export default [
  {
    name: `Test norm #1`,
    variant: 'test-csl2-6',
    input: { 0: { x: 0 }, 300: {} },
    program: norm(toTs(r(1e9)), i64(4)),
    output: {
      0: { z: 0.25 },
      4: { z: 0 },
      300: {},
    },
  },
  {
    name: `Test norm #2`,
    variant: 'test-csl2-6',
    input: { 0: { x: 0 }, 300: {} },
    program: norm(toTs(r(1e-9)), i64(200)),
    output: {
      0: { z: 0.005 },
      200: { z: 0 },
      300: {},
    },
  },
  {
    name: `Test norm #3`,
    variant: 'test-csl2-6',
    input: { 0: { x: 0 }, 300: {} },
    program: norm(toTs(r(20)), i64(100), false),
    output: {
      0: { z: 0.01 },
      300: {},
    },
  },
  {
    name: `Test norm #4`,
    variant: 'test-csl2-6',
    input: { 0: { x: 0 }, 300: {} },
    program: norm(toTs(arr(r(1), r(2), r(5), r(1), r(1))), i64(50)),
    output: {
      0: { z: 0.1 },
      1: { z: 0.2 },
      2: { z: 0.5 },
      3: { z: 0.1 },
      4: { z: 0.1 },
      5: { z: 0 },
      300: {},
    },
  },
];
