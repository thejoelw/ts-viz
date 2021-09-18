import makeDefines from '../defines.ts';

const variant = Deno.args[0];

Object.entries(makeDefines(variant)).forEach(([k, v]) =>
  console.log(
    `: |> echo "#define ${k} ${v}" > %o |> src/defs/${k}.h $(ROOT)/<gen_headers>`,
  ),
);
