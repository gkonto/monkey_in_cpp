# Monkey Interpreter and VM Plan (`plan.md`)

## Summary
Create a greenfield C++ implementation of Monkey based on Thorsten Ball’s books, starting with the tree-walking interpreter and then extending to the compiler and virtual machine. Use `CMake + Catch2`, keep each milestone small, and add tests at every stage so each feature is locked before the next one begins.

## Defaults And Assumptions
- Language target: `C++20`
- Build system: `CMake`
- Test framework: `Catch2`
- Scope includes:
  1. Interpreter from *Writing an Interpreter in Go*
  2. Macro system from the later interpreter chapters
  3. Compiler and VM from *Writing a Compiler in Go*
- Initial deliverable is planning only; no code should be written before this plan is accepted and saved to `/Users/g.kontogiannis/dev/Monkey/plan.md`

## Public Interfaces
- Provide a REPL executable for interactive Monkey evaluation.
- Provide a file-runner executable for executing Monkey source files.
- Keep the implementation split into modules: lexer, parser, AST, object system, evaluator, compiler, VM, builtins, and tests.

## Step-By-Step Plan
1. [DONE]Create `plan.md` and copy this plan into it verbatim.
2. [DONE]Define the top-level project layout: `src/`, `include/`, `tests/`, `examples/`, and `CMakeLists.txt`.
3. Set the compiler standard, warnings, and debug/release presets in CMake.
4. Add Catch2 through CMake and verify the test target can be discovered.
5. Add a minimal smoke test so the test runner is working before language code exists.
6. Define the token enum and token struct.
7. List all Monkey keywords, operators, and delimiters in one central token table.
8. Write lexer tests for single-character tokens.
9. Implement basic lexer scanning for punctuation and operators.
10. Add lexer tests for identifiers, integers, and keywords.
11. Implement identifier and integer scanning.
12. Add lexer tests for multi-character operators like `==` and `!=`.
13. Implement lookahead support in the lexer.
14. Add lexer tests for strings.
15. Implement string literal scanning.
16. Add lexer tests for arrays, hashes, and complex mixed input.
17. Define the AST base types and statement/expression node hierarchy.
18. Add AST stringification tests to make tree output easy to inspect.
19. Implement the AST node classes with stable `to_string()` behavior.
20. Build the Pratt parser skeleton with prefix and infix parse function registration.
21. Add parser tests for `let` and `return` statements.
22. Implement statement parsing for `let` and `return`.
23. Add parser tests for identifiers, integers, booleans, and prefix expressions.
24. Implement prefix parsing.
25. Add parser tests for infix precedence handling.
26. Implement infix parsing and precedence rules.
27. Add parser tests for grouped expressions and `if` expressions.
28. Implement grouped expressions, blocks, and conditional parsing.
29. Add parser tests for function literals and function calls.
30. Implement parameter parsing and call expressions.
31. Add parser tests for strings, arrays, index expressions, and hashes.
32. Implement those remaining expression forms.
33. Define the runtime object model: integer, boolean, null, return value, error, function, string, array, hash, builtin, quote, macro, compiled function, closure.
34. Add unit tests for object inspection and hash-key behavior.
35. Implement the environment with nested scope support.
36. Add evaluator tests for integers, booleans, and bang/minus operators.
37. Implement base expression evaluation.
38. Add evaluator tests for conditionals and return propagation.
39. Implement block evaluation and return unwinding.
40. Add evaluator tests for error handling.
41. Implement evaluator error propagation with readable messages.
42. Add evaluator tests for `let` bindings and identifier lookup.
43. Implement environment-backed variable resolution.
44. Add evaluator tests for function objects and function application.
45. Implement function calls and argument binding.
46. Add evaluator tests for closures.
47. Implement closure capture in the interpreter.
48. Add evaluator tests for strings and string concatenation.
49. Implement string runtime support.
50. Add evaluator tests for builtins like `len`, `first`, `last`, `rest`, `push`, and `puts`.
51. Implement builtin dispatch and argument validation.
52. Add evaluator tests for arrays and index expressions.
53. Implement array evaluation and indexing.
54. Add evaluator tests for hashes and hash indexing.
55. Implement hash literals and hash lookups.
56. Create a basic REPL wired to the lexer, parser, and evaluator.
57. Add integration tests that feed source snippets and compare printed results.
58. Add parser and evaluator tests for `quote` and `unquote`.
59. Implement quoted AST wrapping and unquote evaluation.
60. Add macro expansion tests.
61. Implement macro definitions, macro environment, and AST expansion.
62. Freeze the interpreter milestone with a full regression test pass.

63. Define the bytecode instruction enum and encoding helpers.
64. Add unit tests for instruction encoding and decoding.
65. Implement bytecode assembly utilities.
66. Define the symbol table for globals, locals, builtins, free symbols, and function names.
67. Add symbol table tests for nested scopes and free-variable resolution.
68. Implement compiler scope tracking.
69. Add compiler tests for integer and boolean expressions.
70. Implement compilation of constants and simple operations.
71. Add compiler tests for conditionals.
72. Implement jump patching and conditional bytecode generation.
73. Add compiler tests for global `let` statements.
74. Implement global symbol definition and loading.
75. Add compiler tests for strings, arrays, and hashes.
76. Implement compilation for those literals.
77. Add compiler tests for index expressions.
78. Implement bytecode generation for indexing.
79. Add compiler tests for functions and returns.
80. Implement compiled functions and return handling.
81. Add compiler tests for function calls and arguments.
82. Implement call emission and arity handling.
83. Add compiler tests for local bindings.
84. Implement local symbol resolution.
85. Add compiler tests for builtins.
86. Implement builtin symbol support in the compiler.
87. Add compiler tests for closures and free variables.
88. Implement closure compilation and free symbol loading.
89. Define the VM stack, globals store, frames, and constants pool.
90. Add VM tests for arithmetic and boolean execution.
91. Implement the core instruction dispatch loop.
92. Add VM tests for conditionals and jumps.
93. Implement jump execution.
94. Add VM tests for global bindings.
95. Implement global set/get opcodes.
96. Add VM tests for strings, arrays, hashes, and indexing.
97. Implement those runtime VM operations.
98. Add VM tests for function calls and local bindings.
99. Implement frame management and local slots.
100. Add VM tests for builtins.
101. Implement builtin invocation in the VM.
102. Add VM tests for closures and recursive functions.
103. Implement closure objects, free variables, and recursive call support.
104. Add end-to-end tests that run the same programs through interpreter and VM and compare results.
105. Add a benchmark harness for a few representative Monkey programs.
106. Create a final CLI split: interpreter mode and VM mode.
107. Document build, test, and run commands in `README.md`.
108. Mark the project complete only after all lexer, parser, evaluator, compiler, VM, and integration tests pass.

## Test Plan
- Lexer tests: token streams for simple and mixed source inputs.
- Parser tests: AST shape and operator precedence.
- Evaluator tests: semantic correctness, errors, closures, arrays, hashes, builtins, macros.
- Compiler tests: exact emitted bytecode and constants.
- VM tests: runtime execution results, frames, closures, recursion, builtins.
- Integration tests: REPL or file execution for representative Monkey programs.
- Regression rule: each new feature must land with tests before the next feature begins.

## Acceptance Criteria
- The interpreter can run the full Monkey language from the first book, including macros.
- The compiler and VM can run the same core programs with matching observable results.
- All tests pass from a clean build using CMake and Catch2.
- `plan.md` exists and contains this numbered implementation guide.
