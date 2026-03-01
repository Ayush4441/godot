# modules/wasm – WebAssembly Scripting for Godot

This module adds a WebAssembly (WASM) scripting language backend to Godot 4,
backed by the [WAMR](https://github.com/bytecodealliance/wasm-micro-runtime)
interpreter.  Scripts are authored in WAT (WebAssembly Text Format) with the
`.wscript` extension, compiled ahead-of-time to a `.wasm` binary at import
time, then loaded and instantiated per-object at runtime.

## Features

* `.wscript` files (WAT text) recognised as a first-class Script type.
* Ahead-of-time WAT → WASM compilation via `wat2wasm` (WABT) at editor import
  time, with the binary cached in `.godot/imported/`.
* Per-object WASM module instances (isolated linear memory per Node/Object).
* Minimal host bindings exposed to WASM:
  * `godot.godot_print(ptr: i32, len: i32)` – print a UTF-8 string.
* Lifecycle exports called by the engine:
  * `gd_ready(self_handle: i32)` – equivalent to `_ready()`.
  * `gd_process(self_handle: i32, delta: f32)` – equivalent to `_process()`.
* Interpreter-only mode (no JIT) for maximum portability (iOS, Web, Raspberry Pi).

## Requirements

### Runtime

* WAMR sources cloned into `thirdparty/wamr/` (see `thirdparty/wamr/README.md`).
  Without WAMR the module still compiles; script execution is disabled with a
  clear error message.

### Import / Editor

* [`wat2wasm`](https://github.com/WebAssembly/wabt) on `PATH`.
  Install via your system package manager or download from the WABT releases
  page.  Without it the importer emits a warning and stores a placeholder.

## Build

```bash
# 1. Clone WAMR
git clone --depth 1 https://github.com/bytecodealliance/wasm-micro-runtime \
    thirdparty/wamr

# 2. Build Godot normally – SCsub detects WAMR automatically
scons platform=linuxbsd target=editor

# To build WITHOUT WAMR (stubs only):
scons platform=linuxbsd target=editor module_wasm_enabled=yes
# (WAMR_ENABLED will not be set if thirdparty/wamr is absent)
```

## Usage

### Hello World (.wscript)

```wat
(module
  ;; Import godot_print from the host.
  (import "godot" "godot_print" (func $godot_print (param i32 i32)))

  ;; Static string in linear memory.
  (memory 1)
  (data (i32.const 0) "Hello from WASM!\n")

  ;; gd_ready(self_handle: i32) – called when the node enters the scene tree.
  (func (export "gd_ready") (param $self i32)
    i32.const 0    ;; ptr
    i32.const 17   ;; len ("Hello from WASM!\n")
    call $godot_print
  )

  ;; gd_process(self_handle: i32, delta: f32) – called every frame.
  (func (export "gd_process") (param $self i32) (param $delta f32)
  )
)
```

Attach the `.wscript` file to a Node just like any other Script resource.
The engine calls `gd_ready` on `NOTIFICATION_READY` and you can call
`gd_process` from a GDScript `_process` method by forwarding the delta.

### Host bindings reference

| Import | Signature | Description |
|---|---|---|
| `godot.godot_print` | `(ptr: i32, len: i32)` | Print UTF-8 string to Godot output |

### Exported lifecycle functions

| Export | Signature | Trigger |
|---|---|---|
| `gd_ready` | `(self: i32)` | `NOTIFICATION_READY` |
| `gd_process` | `(self: i32, delta: f32)` | Call from GDScript `_process` |

## Platform support

| Platform | Status |
|---|---|
| Linux x86-64 | ✅ |
| macOS | ✅ |
| Windows | ✅ |
| Android | ✅ |
| Raspberry Pi (ARM Linux) | ✅ |
| iOS | ✅ (interpreter only) |
| Web (Emscripten) | ⚠️ TODO |

Web export requires WAMR compiled to `wasm32` via Emscripten.
See `thirdparty/wamr/README.md` for details.

## Architecture

```
modules/wasm/
├── config.py                    # SCons module config
├── SCsub                        # Build rules
├── register_types.h/cpp         # Godot type registration
├── wasm_language.h/cpp          # ScriptLanguage backend
├── wasm_script.h/cpp            # Script resource + format loader/saver
├── wasm_instance.h/cpp          # Per-object ScriptInstance
└── editor/
    └── resource_importer_wasm.h/cpp  # WAT→WASM importer (editor only)

thirdparty/wamr/                 # WAMR sources (git-cloned, not vendored)
```
