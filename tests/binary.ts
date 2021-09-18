import {
  add,
  d,
  div,
  gt,
  input,
  lt,
  max,
  min,
  mod,
  mul,
  shrink,
  sub,
} from '../../trader-exprs/modules/base.ts';

const r = d;

export default [
  {
    name: `Test add`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: add(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: 11 },
      2: { z: NaN },
      3: { z: -3 },
      4: { z: NaN },
      5: { z: 15 },
      6: { z: 7 },
      7: { z: 123 },
      8: { z: 14 },
      9: { z: 234 },
    },
  },
  {
    name: `Test sub`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: sub(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: -3 },
      2: { z: NaN },
      3: { z: 7 },
      4: { z: NaN },
      5: { z: -29 },
      6: { z: 5 },
      7: { z: 123 },
      8: { z: 0 },
      9: { z: -234 },
    },
  },
  {
    name: `Test mul`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: mul(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: 28 },
      2: { z: NaN },
      3: { z: -10 },
      4: { z: NaN },
      5: { z: -154 },
      6: { z: 6 },
      7: { z: 0 },
      8: { z: 49 },
      9: { z: 0 },
    },
  },
  {
    name: `Test div`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: div(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: 4 / 7 },
      2: { z: NaN },
      3: { z: -0.4 },
      4: { z: NaN },
      5: { z: -7 / 22 },
      6: { z: 6 },
      7: { z: NaN },
      8: { z: 1 },
      9: { z: 0 },
    },
  },
  {
    name: `Test mod`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: mod(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: 4 },
      2: { z: NaN },
      3: { z: 2 },
      4: { z: NaN },
      5: { z: -7 },
      6: { z: 0 },
      7: { z: NaN },
      8: { z: 0 },
      9: { z: 0 },
    },
  },
  {
    name: `Test lt`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: lt(r(input('x')), r(input('y'))),
    output: {
      0: { z: 0 },
      1: { z: 1 },
      2: { z: 0 },
      3: { z: 0 },
      4: { z: 0 },
      5: { z: 1 },
      6: { z: 0 },
      7: { z: 0 },
      8: { z: 0 },
      9: { z: 1 },
    },
  },
  {
    name: `Test gt`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: gt(r(input('x')), r(input('y'))),
    output: {
      0: { z: 0 },
      1: { z: 0 },
      2: { z: 0 },
      3: { z: 1 },
      4: { z: 0 },
      5: { z: 0 },
      6: { z: 1 },
      7: { z: 1 },
      8: { z: 0 },
      9: { z: 0 },
    },
  },
  {
    name: `Test min`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: min(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: 4 },
      2: { z: 8 },
      3: { z: -5 },
      4: { z: 6 },
      5: { z: -7 },
      6: { z: 1 },
      7: { z: 0 },
      8: { z: 7 },
      9: { z: 0 },
    },
  },
  {
    name: `Test max`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: max(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: 7 },
      2: { z: 8 },
      3: { z: 2 },
      4: { z: 6 },
      5: { z: 22 },
      6: { z: 6 },
      7: { z: 123 },
      8: { z: 7 },
      9: { z: 234 },
    },
  },
  {
    name: `Test shrink`,
    variant: 'test-csl2-6',
    input: {
      0: { x: NaN, y: NaN },
      1: { x: 4, y: 7 },
      2: { x: NaN, y: 8 },
      3: { x: 2, y: -5 },
      4: { x: 6, y: NaN },
      5: { x: -7, y: 22 },
      6: { x: 6, y: 1 },
      7: { x: 123, y: 0 },
      8: { x: 7, y: 7 },
      9: { x: 0, y: 234 },
    },
    program: shrink(r(input('x')), r(input('y'))),
    output: {
      0: { z: NaN },
      1: { z: 0 },
      2: { z: NaN },
      3: { z: 7 },
      4: { z: NaN },
      5: { z: 0 },
      6: { z: 5 },
      7: { z: 123 },
      8: { z: 0 },
      9: { z: 0 },
    },
  },
];
