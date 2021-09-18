import {
  arr,
  conv,
  d,
  i64,
  input,
  norm,
  toTs,
  windowRect,
} from '../trader-exprs/modules/base.ts';
import { range } from '../trader-exprs/modules/util.ts';

const r = d;

const kernel = (spec) => {
  const els = [];
  Object.entries(spec).forEach(([k, v]) => (els[k] = v));
  for (let i = 0; i < els.length; i++) {
    els[i] = r(els[i] || 0);
  }
  const width = i64(els.length);
  return {
    name: 'customKernel',
    width,
    kernel: norm(toTs(arr(...els)), width),
  };
};

export default [
  ...[
    ...[2, 3, 4, 5, 6, 7, 8, 9],
    ...[10, 15, 16, 17, 31, 32, 33],
    ...[63, 64, 65, 100, 127, 128, 129, 255, 256, 257],
  ].map((n) => ({
    name: `Test spike input and kernelSize=${n}`,
    variant: ['test-csl2-6', 'debug', 'release'],
    input: { 0: { x: 0 }, 10: { x: 1 }, 11: { x: 0 }, 300: {} },
    program: conv(windowRect(r(n)), r(input('x')), true),
    output: { 0: { z: 0 }, 10: { z: 1 / n }, [10 + n]: { z: 0 }, 300: {} },
  })),

  ...[
    ...[2, 3, 4, 5, 6, 7, 8, 9],
    ...[10, 15, 16, 17, 31, 32, 33],
    ...[63, 64, 65, 100, 127, 128, 129, 255, 256, 257],
  ].map((n) => ({
    name: `Test spike input and yields every ${n}`,
    variant: ['test-csl2-6'],
    input: {
      0: { x: 0 },
      10: { x: 1 },
      11: { x: 0 },
      300: {},
    },
    yields: range(0, 300, n),
    program: conv(windowRect(r(1000)), r(input('x')), true),
    output: { 0: { z: 0 }, 10: { z: 0.001 }, 300: {} },
  })),

  {
    name: `Test kernel spikes`,
    variant: ['test-csl2-6', 'debug', 'release'],
    input: { 0: { x: 0 }, 10: { x: 1 }, 11: { x: 0 }, 300: {} },
    program: conv(kernel({ 7: 0.4, 14: 0.6 }), r(input('x')), true),
    output: {
      0: { z: 0 },
      17: { z: 0.4 },
      18: { z: 0 },
      24: { z: 0.6 },
      25: { z: 0 },
      300: {},
    },
  },

  {
    name: `Test nan handling #1`,
    variant: ['test-csl2-6', 'debug', 'release'],
    input: {
      0: { x: 1 },
      20: { x: 0 },
      150: { x: null },
      200: { x: 1 },
      300: {},
    },
    program: conv(windowRect(r(10)), r(input('x'))),
    output: {
      0: { z: null },
      9: { z: 1 },
      20: { z: 0.9 },
      21: { z: 0.8 },
      22: { z: 0.7 },
      23: { z: 0.6 },
      24: { z: 0.5 },
      25: { z: 0.4 },
      26: { z: 0.3 },
      27: { z: 0.2 },
      28: { z: 0.1 },
      29: { z: 0 },
      150: { z: null },
      209: { z: 1 },
      300: {},
    },
  },

  {
    name: `Test nan handling #2`,
    variant: ['test-csl2-6', 'debug', 'release'],
    input: { 0: { x: 1 }, 150: { x: null }, 180: { x: 1 }, 300: {} },
    program: conv(windowRect(r(100)), r(input('x'))),
    output: {
      0: { z: null },
      99: { z: 1 },
      150: { z: null },
      279: { z: 1 },
      300: {},
    },
  },

  {
    name: `Test nan handling #3`,
    variant: ['test-csl2-6', 'debug', 'release'],
    input: { 0: { x: 1 }, 150: { x: null }, 200: { x: 1 }, 300: {} },
    program: conv(windowRect(r(10)), r(input('x')), true),
    output: {
      0: { z: 0.1 },
      1: { z: 0.2 },
      2: { z: 0.3 },
      3: { z: 0.4 },
      4: { z: 0.5 },
      5: { z: 0.6 },
      6: { z: 0.7 },
      7: { z: 0.8 },
      8: { z: 0.9 },
      9: { z: 1 },
      150: { z: 0.9 },
      151: { z: 0.8 },
      152: { z: 0.7 },
      153: { z: 0.6 },
      154: { z: 0.5 },
      155: { z: 0.4 },
      156: { z: 0.3 },
      157: { z: 0.2 },
      158: { z: 0.1 },
      159: { z: 0 },
      200: { z: 0.1 },
      201: { z: 0.2 },
      202: { z: 0.3 },
      203: { z: 0.4 },
      204: { z: 0.5 },
      205: { z: 0.6 },
      206: { z: 0.7 },
      207: { z: 0.8 },
      208: { z: 0.9 },
      209: { z: 1 },
      300: {},
    },
  },

  {
    name: `Test nan handling #4`,
    variant: ['test-csl2-6', 'debug', 'release'],
    input: { 0: { x: 0 }, 150: { x: null }, 200: { x: 0 }, 300: {} },
    program: conv(windowRect(r(100)), r(input('x')), true),
    output: {
      0: { z: 0 },
      300: {},
    },
  },
];
