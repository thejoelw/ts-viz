export const assert = (cond: boolean) => {
  if (!cond) {
    throw new Error(`Assertion failed`);
  }
};

export const range = (a: number, b: undefined | number = undefined, c = 1) => {
  if (b === undefined) {
    b = a;
    a = 0;
  }
  const res = [];
  for (let i = a; i < b; i += c) {
    res.push(i);
  }
  return res;
};

export const linspace = (a: number, b: number, steps: number) => {
  const res = [];
  for (let i = 0; i < steps; i++) {
    res.push(a + (b - a) * (i / (steps - 1)));
  }
  return res;
};

export const geomspace = (a: number, b: number, steps: number) =>
  linspace(Math.log(a), Math.log(b), steps).map(Math.exp);
