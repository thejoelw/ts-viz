# Ts-viz

Ts-viz is an execution engine for streaming data, specializing in realtime convolutions. It provides both a headless streaming interface and visualizer GUI for rapid prototyping.

## Building

```sh
git clone git@github.com:thejoelw/ts-viz.git
cd ts-viz
tup init
tup variant configs/*
tup
```

Note that if you get this error: `Unable to write to a file outside of the tup hierarchy: /ts-viz`, you need to update your tup version to 0.8.

## Usage

```sh
echo '[["emit", "y", ["square", ["cast_double", ["input", "x"]]]]]' > /tmp/program.json
seq 1000000 \
  | jq --compact-output '{x: (. / 1000000)}' \
  | ./build-release-headless/ts-viz --emit-format json /tmp/program.json -
# Prints 1000000 lines of {"y": ...}
```

Ts-viz combines a dataset stream of json records with a lisp-like program encoded in json, producing an output stream (of json records or binary data, depending on your use case).

The program is not meant to be written by hand; instead, a TypeScript "sdk" is provided. Here's an example of a program generator:

```ts
import { conv, d, emit, input, windowSmooth } from '../ts-viz/ts/base.ts';
import { r } from '../ts-viz/ts/config.ts';
import { stringify } from '../ts-viz/ts/stringify.ts';

const main = () => {
  let res: Array<Node> = [];

  const price = d(input('price'));
  const smoothedPrice = conv(windowSmooth(r(1e6), r(2)), price);
  res.push(emit('smoothedPrice', smoothedPrice));

  return res;
};

console.log(stringify(main()));
```

Run with:
```sh
deno run /Users/joel/proj/trader-2/main.ts > /tmp/program.json
```

[ts/base.ts](https://github.com/thejoelw/ts-viz/blob/master/ts/base.ts) is a good showcase of the available operations. `stringify` is `JSON.stringify` augmented with deduplication, using json references to keep the generated lisp small.

## Internals

Ts-viz parses each program into a DAG of time series. Each time series performs some operation (multiplication, cumulative sum, convolution, etc...). Each time series has chunks of 65536 elements * 8bytes/double = 0.5mb; chunks are loaded lazily and may be garbage collected if memory is low (flag `--gc-memory-limit`).

Ts-viz uses [FFTW](https://www.fftw.org/) to perform fast convolutions. The first time you load a program using convolutions, [wisdom](https://www.fftw.org/fftw-wisdom.1.html) will be generated automatically for powers of 2 under the kernel sizes you're using. This may take a while, but the wisdom will be cached for next time. You can modify this behavior using the `--wisdom-dir`, `--require-existing-wisdom`, and `--dont-write-wisdom` flags.

## Visualization

In addition to realtime streaming of the dataset, the program can be streamed in realtime too. New programs will re-use the series and chunks from previous programs where it can, ensuring only a minimal amount of computation is performed. For example, we can consume a live stream of bitcoin price data and experiment with different convolutions on it:

```sh
websocat 'wss://data-stream.binance.com:9443/stream?streams=btcusdt@bookTicker' \
  | jq --unbuffered --compact-output '{bid: .data.b | tonumber, ask: .data.a | tonumber}' \
  | ./build-release/ts-viz <(deno run --watch --no-clear-screen test.ts) -
```

[Demo](/media/ts-viz-demo.webm)

## Options

```text
> ./build-release/ts-viz --help
Usage: ts-viz [options] program-path data-path 

A time series visualizer and processor

Positional arguments:
program-path                            The path to the program file or stream [required]
data-path                               The path to the data file or stream [required]

Optional arguments:
-h --help                               shows help message and exits
-v --version                            prints version information and exits
--title                                 The window title to show [default: "ts-viz"]
--log-level                             Minimum logging level to output [default: 2]
--wisdom-dir                            The directory to load and save wisdom to/from [default: "."]
--require-existing-wisdom               Disable generating fftw's wisdom; exit if they don't exist in filesystem [default: false]
--dont-write-wisdom                     Disable writing fftw's wisdom files [default: false]
--conv-min-compute-log2                 For calculating convolutions, advance in (2 ^ value) element increments [default: 0]
--gc-memory-limit                       Enable garbage collector above this value [default: 18446744073709551615]
--print-memory-usage-output-index       Prints the memory usage required to compute and output the nth record [default: 18446744073709551615]
--debug-series-to-file                  Outputs per-chunk debugging information to a file [default: ""]
--emit-format                           Sets the format of emitted records: none, json, floats, or doubles [default: 0]
--meter-indices                         Output meter records at these indices [default: <not representable>]
--max-fps                               Cap frames per second at this value, or zero to disable [default: 0]
--dont-exit                             Don't exit, even if the program pipe and data pipes end [default: false]
```
