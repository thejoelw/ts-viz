import { walk } from 'https://deno.land/std@0.107.0/fs/walk.ts';
import { Node } from '../trader-exprs/types/types.ts';
import { stringify } from '../trader-exprs/util/stringify.ts';

const variant = Deno.args[0];

const computeWisdom = false;

// Generate wisdom before so our tests don't time out
let isWisdomPrepared = false;
const writePrepareWisdom = () => {
  if (!isWisdomPrepared) {
    console.log(
      `: $(BIN_TARGET) |> bash util/prepare_wisdom.bash '${variant}' '$(BIN_TARGET)' |> fftw_wisdom_float.bin fftw_wisdom_double.bin`,
    );
    isWisdomPrepared = true;
  }
};

const processJsonStream = (
  spec: { [key: number]: { [key: string]: number } },
  yields?: number[],
) => {
  const arr1: { [key: string]: number }[] = [];
  Object.entries(spec).forEach(([k, v]) => (arr1[parseInt(k)] = v));

  let acc = {};
  for (let i = 0; i < arr1.length; i++) {
    arr1[i] = acc = { ...acc, ...arr1[i] };
  }

  let arr2: ({ [key: string]: number } | 'yield')[] = arr1;

  (yields || [])
    .sort()
    .reverse()
    .forEach((k) => arr2.splice(k, 0, 'yield'));

  return serializeLines(arr2);
};

const processProgram = (spec: Node) => serializeLines([[['emit', 'z', spec]]]);

const serializeLines = (lines: any[]) =>
  lines
    .map((line) => (typeof line === 'object' ? stringify(line) : line) + '\\n')
    .join('');

(async () => {
  for await (const entry of walk('./tests', {
    includeDirs: false,
    exts: ['.ts'],
  })) {
    const file = entry.path;

    const tests: {
      variant: string | string[];
      name: string;
      input: any;
      yields: any;
      program: any;
      output: any;
    }[] = (await import(`../${file}`)).default;

    tests
      .filter((test) =>
        Array.isArray(test.variant)
          ? test.variant.includes(variant)
          : test.variant === variant,
      )
      .forEach(({ name, input, yields, program, output }) => {
        input = processJsonStream(input, yields);
        program = processProgram(program);
        output = processJsonStream(output);

        computeWisdom && writePrepareWisdom();
        console.log(
          `: $(BIN_TARGET) ${
            computeWisdom ? 'fftw_wisdom_float.bin fftw_wisdom_double.bin' : ''
          } |> ^ bash util/run_test.bash '${variant} - ${file} - ${name}' [arguments omitted]^ bash util/run_test.bash '${variant} - ${file} - ${name}' '$(BIN_TARGET)' '${input}' '${program}' '${output}' |>`,
        );
      });
  }
})();
