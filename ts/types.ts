export type Node = [string, ...(null | boolean | number | string | Node)[]];
export type Window = { name: string; kernel: Node; width: Node };
