import { Sha256 } from 'https://deno.land/std@0.160.0/hash/sha256.ts';
import { encodeHex } from 'https://deno.land/std@0.221.0/encoding/hex.ts';

const encoder = new TextEncoder();

export const getHash = (obj: unkonwn, cache = new WeakMap()) => {
  if (typeof obj !== 'object' || obj === null) {
    throw new Error(`Cannot get the hash of a ${typeof obj}!`);
  }
  let hash = cache.get(obj);
  if (hash === undefined) {
    let key;
    if (Array.isArray(obj)) {
      key = obj.map((x) => getPrimitive(x, cache));
    } else {
      key = Object.fromEntries(
        Object.entries(obj).map(([k, v]) => [k, getPrimitive(v, cache)]),
      );
    }

    const algo = new Sha256();
    algo.update(encoder.encode(JSON.stringify(key)));
    hash = encodeHex(new Uint8Array(algo.digest()));

    cache.set(obj, hash);
  }
  return hash;
};

const getPrimitive = (value: unknown, cache = new WeakMap()) => {
  if (typeof value === 'function') {
    throw new Error(`Cannot serialize a function!`);
  } else if (typeof value === 'object' && value !== null) {
    return getHash(value, cache);
  } else {
    return `${value}`;
  }
};

export const stringify = (root: unknown) => {
  const hashCache = new WeakMap();
  const pathCache = new Map();

  return JSON.stringify(root, function (key, value) {
    if (typeof value !== 'object' || value === null) {
      return value;
    }

    const hash = getHash(value, hashCache);
    const priorPath = pathCache.get(hash);
    if (priorPath !== undefined) {
      return { $ref: priorPath };
    }

    if (value === root) {
      pathCache.set(hash, '#');
    } else {
      // JSON.stringify sets `this` to the parent object
      const parentPath = pathCache.get(getHash(this, hashCache));
      if (parentPath === undefined) {
        throw new Error(`Missing parent path!`);
      }
      pathCache.set(hash, `${parentPath}/${key}`);
    }

    return value;
  });
};
