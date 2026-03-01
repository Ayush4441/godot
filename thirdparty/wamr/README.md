# WAMR – WebAssembly Micro Runtime

This directory is the integration point for the
[WAMR](https://github.com/bytecodealliance/wasm-micro-runtime) runtime used by
the `modules/wasm/` Godot module.

## Status

WAMR sources are **not** vendored in this repository to keep the tree size
manageable.  The module compiles and links without WAMR present (runtime
execution is disabled, and a clear error message is emitted).  To enable full
WASM execution, populate this directory with WAMR sources as described below.

## Obtaining WAMR

```bash
# From the repository root
git clone --depth 1 https://github.com/bytecodealliance/wasm-micro-runtime \
    thirdparty/wamr
```

The `modules/wasm/SCsub` build script looks for
`thirdparty/wamr/core/iwasm/include/` to detect WAMR and sets the
`WAMR_ENABLED` compile definition when found.

## Recommended build flags

When building WAMR as a static library to link into Godot, use:

```cmake
cmake \
  -DWAMR_BUILD_INTERP=1 \
  -DWAMR_BUILD_AOT=0 \
  -DWAMR_BUILD_JIT=0 \
  -DWAMR_BUILD_LIBC_BUILTIN=1 \
  -DWAMR_BUILD_LIBC_WASI=0 \
  ..
```

`WAMR_BUILD_INTERP=1` (interpreter mode) is required for portability across
iOS, Web (Emscripten), and Raspberry Pi targets where JIT is not available.

## Platform notes

| Platform | Status |
|---|---|
| Linux x86-64 | ✅ Interpreter |
| macOS x86-64 / arm64 | ✅ Interpreter |
| Windows x86-64 | ✅ Interpreter |
| Linux ARM (Raspberry Pi) | ✅ Interpreter |
| Android | ✅ Interpreter |
| iOS | ✅ Interpreter (JIT disabled) |
| Web (Emscripten) | ⚠️ TODO – compile WAMR for `wasm32` target |

For Web export, WAMR must itself be compiled to WebAssembly via Emscripten so
that the guest WASM execution stays inside the engine rather than being
delegated to the browser JS engine.  This is staged work; see the TODO
comments in `modules/wasm/wasm_language.cpp`.
