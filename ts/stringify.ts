export const stringify = (code: unknown, prettify = false) => {
  const idCache = new Map();
  let nextId = 0;

  const getId = (value: unknown) => {
    if (typeof value === 'object' && value !== null) {
      if ('$id' in value) {
        return (value as { $id: unknown }).$id;
      } else {
        let key;
        if (Array.isArray(value)) {
          key = value.map(getId);
        } else {
          key = Object.fromEntries(
            Object.entries(value).map(([k, v]) => [k, getId(v)]),
          );
        }
        key = JSON.stringify(key);

        let id;
        if (idCache.has(key)) {
          id = idCache.get(key);
        } else {
          id = { $id: nextId++ };
          idCache.set(key, id);
        }

        Object.defineProperty(value, '$id', {
          value: id,
          enumerable: false,
          writable: false,
        });

        return id;
      }
    } else {
      return value;
    }
  };

  const pathCache = new WeakMap();
  const dedup = (path: string, value: unknown): unknown => {
    const id = getId(value);

    if (typeof id === 'object') {
      if (pathCache.has(id)) {
        return { $ref: pathCache.get(id) };
      } else {
        pathCache.set(id, path);

        if (Array.isArray(value)) {
          return value.map((el, k) => dedup(`${path}/${k}`, el));
        } else {
          return Object.fromEntries(
            Object.entries(value as Record<string, unknown>).map(([k, v]) => [
              k,
              dedup(`${path}/${k}`, v),
            ]),
          );
        }
      }
    } else {
      return value;
    }
  };

  return JSON.stringify(dedup('#', code), null, prettify ? 2 : undefined);
};
