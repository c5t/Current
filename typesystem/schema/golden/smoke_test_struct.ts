// Autogenerated TypeScript and io-ts types for C5T/Current JSON.
// peerDependencies: io-ts@0.5.1 c5t-current-schema-ts@0.1.0
// hash: b528c6626c40443d
/* tslint:disable */

import * as iots from 'io-ts';
import * as C5TCurrent from 'c5t-current-schema-ts';


export const Primitives_IO = iots.interface({
  // It's the "order" of fields that matters.
  a: C5TCurrent.UInt8_IO,

  // Field descriptions can be set in any order.
  b: C5TCurrent.UInt16_IO,
  c: C5TCurrent.UInt32_IO,
  d: C5TCurrent.UInt64_IO,
  e: C5TCurrent.Int8_IO,
  f: C5TCurrent.Int16_IO,
  g: C5TCurrent.Int32_IO,
  h: C5TCurrent.Int64_IO,
  i: C5TCurrent.Char_IO,
  j: C5TCurrent.String_IO,
  k: C5TCurrent.Float_IO,
  l: C5TCurrent.Double_IO,

  // Multiline
  // descriptions
  // can be used.
  m: C5TCurrent.Bool_IO,
  n: C5TCurrent.Microseconds_IO,
  o: C5TCurrent.Milliseconds_IO,
}, 'Primitives');
export type Primitives = iots.TypeOf<typeof Primitives_IO>;

export const A_IO = iots.interface({
  a: C5TCurrent.Int32_IO,
}, 'A');
export type A = iots.TypeOf<typeof A_IO>;

export const B_IO = iots.intersection([ A_IO, iots.interface({
  b: C5TCurrent.Int32_IO,
}) ], 'B');
export type B = iots.TypeOf<typeof B_IO>;

export const B2_IO = iots.intersection([ A_IO ], 'B2');
export type B2 = iots.TypeOf<typeof B2_IO>;

export const Empty_IO = iots.interface({}, 'Empty');
export type Empty = iots.TypeOf<typeof Empty_IO>;

export const X_IO = iots.interface({
  x: C5TCurrent.Int32_IO,
}, 'X');
export type X = iots.TypeOf<typeof X_IO>;

export const E_IO = C5TCurrent.UInt16_IO;
export type E = iots.TypeOf<typeof E_IO>;

export const Y_IO = iots.interface({
  e: C5TCurrent.Enum_IO('E'),
}, 'Y');
export type Y = iots.TypeOf<typeof Y_IO>;

export const MyFreakingVariant_IO = iots.union([
  A_IO,
  X_IO,
  Y_IO,
  iots.null,
], 'MyFreakingVariant');
export type MyFreakingVariant = iots.UnionType<[
  typeof A_IO,
  typeof X_IO,
  typeof Y_IO,
  typeof iots.null
], (
  iots.TypeOf<typeof A_IO> |
  iots.TypeOf<typeof X_IO> |
  iots.TypeOf<typeof Y_IO> |
  iots.TypeOf<typeof iots.null>
)>;

export const Variant_B_A_X_Y_E_IO = iots.union([
  A_IO,
  X_IO,
  Y_IO,
  iots.null,
], 'Variant_B_A_X_Y_E');
export type Variant_B_A_X_Y_E = iots.UnionType<[
  typeof A_IO,
  typeof X_IO,
  typeof Y_IO,
  typeof iots.null
], (
  iots.TypeOf<typeof A_IO> |
  iots.TypeOf<typeof X_IO> |
  iots.TypeOf<typeof Y_IO> |
  iots.TypeOf<typeof iots.null>
)>;

export const C_IO = iots.interface({
  e: Empty_IO,
  c: MyFreakingVariant_IO,
  d: Variant_B_A_X_Y_E_IO,
}, 'C');
export type C = iots.TypeOf<typeof C_IO>;

export const Variant_B_A_B_B2_C_Empty_E_IO = iots.union([
  A_IO,
  B_IO,
  B2_IO,
  C_IO,
  Empty_IO,
  iots.null,
], 'Variant_B_A_B_B2_C_Empty_E');
export type Variant_B_A_B_B2_C_Empty_E = iots.UnionType<[
  typeof A_IO,
  typeof B_IO,
  typeof B2_IO,
  typeof C_IO,
  typeof Empty_IO,
  typeof iots.null
], (
  iots.TypeOf<typeof A_IO> |
  iots.TypeOf<typeof B_IO> |
  iots.TypeOf<typeof B2_IO> |
  iots.TypeOf<typeof C_IO> |
  iots.TypeOf<typeof Empty_IO> |
  iots.TypeOf<typeof iots.null>
)>;

export const Templated_T9209980946934124423_IO = iots.interface({
  foo: C5TCurrent.Int32_IO,
  bar: X_IO,
}, 'Templated_T9209980946934124423');
export type Templated_T9209980946934124423 = iots.TypeOf<typeof Templated_T9209980946934124423_IO>;

export const Templated_T9227782344077896555_IO = iots.interface({
  foo: C5TCurrent.Int32_IO,
  bar: MyFreakingVariant_IO,
}, 'Templated_T9227782344077896555');
export type Templated_T9227782344077896555 = iots.TypeOf<typeof Templated_T9227782344077896555_IO>;

export const TemplatedInheriting_T9200000002835747520_IO = iots.intersection([ A_IO, iots.interface({
  baz: C5TCurrent.String_IO,
  meh: Empty_IO,
}) ], 'TemplatedInheriting_T9200000002835747520');
export type TemplatedInheriting_T9200000002835747520 = iots.TypeOf<typeof TemplatedInheriting_T9200000002835747520_IO>;

export const Templated_T9209626390174323094_IO = iots.interface({
  foo: C5TCurrent.Int32_IO,
  bar: TemplatedInheriting_T9200000002835747520_IO,
}, 'Templated_T9209626390174323094');
export type Templated_T9209626390174323094 = iots.TypeOf<typeof Templated_T9209626390174323094_IO>;

export const TemplatedInheriting_T9209980946934124423_IO = iots.intersection([ A_IO, iots.interface({
  baz: C5TCurrent.String_IO,
  meh: X_IO,
}) ], 'TemplatedInheriting_T9209980946934124423');
export type TemplatedInheriting_T9209980946934124423 = iots.TypeOf<typeof TemplatedInheriting_T9209980946934124423_IO>;

export const TemplatedInheriting_T9227782344077896555_IO = iots.intersection([ A_IO, iots.interface({
  baz: C5TCurrent.String_IO,
  meh: MyFreakingVariant_IO,
}) ], 'TemplatedInheriting_T9227782344077896555');
export type TemplatedInheriting_T9227782344077896555 = iots.TypeOf<typeof TemplatedInheriting_T9227782344077896555_IO>;

export const Templated_T9200000002835747520_IO = iots.interface({
  foo: C5TCurrent.Int32_IO,
  bar: Empty_IO,
}, 'Templated_T9200000002835747520');
export type Templated_T9200000002835747520 = iots.TypeOf<typeof Templated_T9200000002835747520_IO>;

export const TemplatedInheriting_T9201673071807149456_IO = iots.intersection([ A_IO, iots.interface({
  baz: C5TCurrent.String_IO,
  meh: Templated_T9200000002835747520_IO,
}) ], 'TemplatedInheriting_T9201673071807149456');
export type TemplatedInheriting_T9201673071807149456 = iots.TypeOf<typeof TemplatedInheriting_T9201673071807149456_IO>;

export const TrickyEvolutionCases_IO = iots.interface({
  o1: C5TCurrent.Optional_IO(C5TCurrent.String_IO),
  o2: C5TCurrent.Optional_IO(C5TCurrent.Int32_IO),
  o3: C5TCurrent.Optional_IO(C5TCurrent.Vector_IO(C5TCurrent.String_IO)),
  o4: C5TCurrent.Optional_IO(C5TCurrent.Vector_IO(C5TCurrent.Int32_IO)),
  o5: C5TCurrent.Optional_IO(C5TCurrent.Vector_IO(A_IO)),
  o6: C5TCurrent.Pair_IO(C5TCurrent.String_IO, C5TCurrent.Optional_IO(A_IO)),
  o7: C5TCurrent.PrimitiveMap_IO(C5TCurrent.Optional_IO(A_IO)),
}, 'TrickyEvolutionCases');
export type TrickyEvolutionCases = iots.TypeOf<typeof TrickyEvolutionCases_IO>;

export const FullTest_IO = iots.interface({
  // A structure with a lot of primitive types.
  primitives: Primitives_IO,
  v1: C5TCurrent.Vector_IO(C5TCurrent.String_IO),
  v2: C5TCurrent.Vector_IO(Primitives_IO),
  p: C5TCurrent.Pair_IO(C5TCurrent.String_IO, Primitives_IO),
  o: C5TCurrent.Optional_IO(Primitives_IO),

  // Field | descriptions | FTW !
  q: Variant_B_A_B_B2_C_Empty_E_IO,
  w1: Templated_T9209980946934124423_IO,
  w2: Templated_T9227782344077896555_IO,
  w3: Templated_T9209626390174323094_IO,
  w4: TemplatedInheriting_T9209980946934124423_IO,
  w5: TemplatedInheriting_T9227782344077896555_IO,
  w6: TemplatedInheriting_T9201673071807149456_IO,
  tsc: TrickyEvolutionCases_IO,
}, 'FullTest');
export type FullTest = iots.TypeOf<typeof FullTest_IO>;
