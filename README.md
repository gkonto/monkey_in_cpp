# Monkey in C++

This repository is a C++ implementation of the Monkey programming language from Thorsten Ball's *Writing an Interpreter in Go*.

It is a tree-walking interpreter with:

- a lexer
- a Pratt parser
- an explicit AST
- an evaluator that walks the AST
- a small REPL
- a Catch2-based test suite

This project is inspired directly by the Go version from the book, but the implementation here is written in modern C++ and includes several runtime-oriented performance improvements beyond the original step-by-step version.

## What It Is

Monkey is a small, dynamically typed language designed for learning how interpreters work.

This implementation supports:

- integers and booleans
- prefix and infix expressions
- `if` / `else`
- `let` bindings
- `return`
- functions
- closures
- strings
- arrays
- hashes
- indexing
- builtin functions such as `len`, `first`, `last`, `rest`, and `push`

## Technical Overview

The parser is a Pratt parser. That means expression parsing is driven by token precedence and small prefix/infix parse functions rather than a large grammar table. It keeps precedence handling compact and makes it easy to extend expressions like:

```monkey
1 + 2 * 3
add(1, 2, 3)
array[1 + 1]
```

The parser produces an AST. Evaluation is then done by recursively walking that AST and producing runtime values.

At a high level:

1. The lexer turns source text into tokens.
2. The Pratt parser turns tokens into an AST.
3. A resolver assigns indexed symbols for globals, locals, and builtins.
4. The evaluator walks the AST and computes results.

## Very Brief AST Theory

The AST is a structured representation of the program after parsing.

Instead of treating:

```monkey
1 + 2 * 3
```

as raw text, the parser turns it into nodes such as:

- integer literal `1`
- infix `+`
- infix `*`

with the correct tree shape:

```text
(1 + (2 * 3))
```

That tree shape is what preserves precedence and lets the evaluator work semantically instead of textually.

## Language Support

### Bindings

```monkey
let a = 5;
let b = a * 2;
b;
```

### Arithmetic and Comparisons

```monkey
1 + 2 * 3;
10 / 2;
1 < 2;
1 == 1;
true != false;
```

### Conditionals

```monkey
if (1 < 2) {
  10
} else {
  20
}
```

### Functions and Closures

```monkey
let newAdder = fn(x) {
  fn(y) { x + y; };
};

let addTwo = newAdder(2);
addTwo(3);
```

### Strings

```monkey
"Hello" + " " + "World!";
len("hello");
```

### Arrays

```monkey
let xs = [1, 2, 3];
xs[1];
push(xs, 4);
rest(xs);
```

### Hashes

```monkey
let m = {
  "one": 1,
  "two": 2,
  true: 3
};

m["one"];
m[true];
```

## Example

Recursive Fibonacci works as expected:

```monkey
let fibonacci = fn(x) {
  if (x == 0) {
    0
  } else {
    if (x == 1) {
      1
    } else {
      fibonacci(x - 1) + fibonacci(x - 2);
    }
  }
};

fibonacci(10);
```

## Performance Notes

This is still an AST interpreter, not a compiler or VM. Even so, the recent changes significantly improved runtime behavior.

On the current machine, using the optimized preset `release-native-lto`, the standalone `fibonacci(33)` benchmark is currently around:

- `~1.3s - 1.6s` wall-clock

and peak memory is now roughly:

- `~1.6MB - 1.8MB`

These numbers are local measurements, not a formal benchmark suite, but they show the direction of the recent changes clearly.

### Profiling Observations

The optimization work in this repository was driven by repeated profiling of the `fibonacci(33)` benchmark.

The main observations were:

- early versions spent a lot of time in AST dispatch and RTTI-based type checks
- after that was improved, runtime name lookup became a visible cost
- after lookup was improved, object allocation and ownership churn became the dominant issue
- after the `Value`/arena refactor, the main memory issue became over-persisting recursive call frames
- after temporary call-frame separation, the main remaining cost became plain recursive AST evaluation, with a smaller residue from argument handling and general interpreter overhead

### Refactorings and Their Effect

#### 1. Removing expensive AST dispatch

Originally, AST handling relied more heavily on dynamic runtime dispatch. Profiling showed that repeated type checks and dispatch overhead were hot in the evaluator.

That was improved by moving to a cheaper explicit node-kind model:

- AST nodes carry a concrete `NodeType`
- evaluator logic dispatches by node kind
- cloning and evaluation avoid more expensive RTTI-style paths

This reduced overhead in the hottest recursive interpreter path without changing semantics.

#### 2. Indexed symbol resolution

The next bottleneck was variable lookup.

Instead of resolving names entirely at runtime through string-based environments, the interpreter now performs a symbol-resolution pass that assigns indexed references for:

- globals
- locals
- builtins

At runtime, identifiers are read by indexed slots rather than repeated name-based lookup. This means:

- fewer string operations in the hot path
- cheaper local and global access
- better scaling for deeply recursive programs

This is still an interpreter, not a bytecode VM, but it borrows an important compiler-style idea: name resolution is done once, while runtime access is indexed.

#### 3. `Value` as a tagged runtime handle

Another major cost came from the runtime object model.

The interpreter now uses a tagged `Value` handle instead of making every runtime result look like a heap-managed object reference. In particular:

- `int` is immediate
- `bool` is immediate
- `null` is immediate
- composite values still use heap-backed objects

This removed a large amount of small-object allocation from arithmetic-heavy code.

#### 4. Scratch arena and persistent store

Composite values do not all have the same lifetime, so using one ownership strategy everywhere was wasteful.

The runtime is now split into:

- `ScratchArena` for temporary composites created during evaluation
- `PersistentStore` for values that must outlive the current scratch lifetime

This gives temporary work a cheap allocation path while still allowing returned values and closure-captured values to survive safely.

#### 5. Explicit promotion and detachment

To make lifetime transitions explicit, the runtime now uses:

- `promote(Value, PersistentStore&)`
- `detach(Value)`

This makes the escape boundaries visible in the implementation:

- temporary values stay temporary by default
- values that must live longer are promoted deliberately
- final top-level results can be detached so they remain valid even after temporary storage is gone

This avoids dangling references while still keeping the fast path cheap.

#### 6. Temporary call frames instead of persistent recursive frames

One of the most important fixes was separating ordinary call frames from escaping closure state.

At one point, recursive calls were retaining far too much state because frames and values were being persisted too aggressively. The current runtime uses:

- temporary stack-like `Environment` frames for normal calls
- persistent storage only when closures or returned values actually need to escape

Conceptually:

- ordinary recursive calls do not need heap-retained lifetime
- closures do

This change dramatically reduced memory retention and made the arena design actually pay off.

#### 7. Small-arity argument fast path

After the larger lifetime issues were solved, profiling still showed avoidable overhead around function-call argument handling.

The interpreter now uses a small inline argument container for common cases such as:

- 0 arguments
- 1 argument
- 2 arguments

This avoids allocating a `std::vector<Value>` for the most common recursive call shapes, especially in the Fibonacci benchmark.

#### 8. Build-level optimizations

The project also supports stronger optimized builds through CMake presets:

- `release`
- `release-lto`
- `release-native-lto`

The fastest local preset uses:

- release optimization
- link-time optimization
- native CPU tuning

That does not change the interpreter design, but it matters enough that benchmark numbers should always be read together with the chosen build preset.

### Current Remaining Bottlenecks

At the current stage, the main remaining cost is not memory management but the nature of the interpreter itself:

- recursive AST evaluation
- repeated tree-walking through `eval_node`
- ordinary interpreter overhead around calls, branching, and expression evaluation

That is why the next major performance step, if desired, would likely be some form of compilation or VM rather than another large AST-evaluator refactor.

## Build

### Debug

```bash
cmake --preset debug
cmake --build --preset debug
```

### Release

```bash
cmake --preset release
cmake --build --preset release
```

### Release with LTO

```bash
cmake --preset release-lto
cmake --build --preset release-lto
```

### Fastest Local Build

This preset enables both LTO and native CPU tuning:

```bash
cmake --preset release-native-lto
cmake --build --preset release-native-lto
```

## Test

### Debug Tests

```bash
ctest --preset debug --output-on-failure
```

### Optimized Tests

```bash
ctest --preset release-native-lto --output-on-failure
```

## Run

### REPL

Debug build:

```bash
./build/debug/monkey_repl
```

Optimized build:

```bash
./build/release-native-lto/monkey_repl
```

Then type Monkey code directly:

```text
>> let add = fn(x, y) { x + y; };
>> add(2, 3);
5
```

### Performance Benchmark

```bash
./build/release-native-lto/monkey_performance_tests "Performance fibonacci(33)"
```

## Inspiration

The project follows the spirit and structure of Thorsten Ball's *Writing an Interpreter in Go*, but reimplements the interpreter in C++ with a different runtime design and a stronger focus on low-allocation execution paths.

If you know the book, this project will feel familiar:

- tokens
- lexer
- Pratt parser
- AST
- evaluator
- environments
- closures

The difference is mainly in the implementation language and the runtime optimization work done in this repository.
