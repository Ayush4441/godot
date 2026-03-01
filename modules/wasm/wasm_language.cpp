/**************************************************************************/
/*  wasm_language.cpp                                                     */
/**************************************************************************/
/*                         This file is part of:                          */
/*                             GODOT ENGINE                               */
/*                        https://godotengine.org                         */
/**************************************************************************/
/* Copyright (c) 2014-present Godot Engine contributors (see AUTHORS.md). */
/* Copyright (c) 2007-2014 Juan Linietsky, Ariel Manzur.                  */
/*                                                                        */
/* Permission is hereby granted, free of charge, to any person obtaining  */
/* a copy of this software and associated documentation files (the        */
/* "Software"), to deal in the Software without restriction, including    */
/* without limitation the rights to use, copy, modify, merge, publish,    */
/* distribute, sublicense, and/or sell copies of the Software, and to     */
/* permit persons to whom the Software is furnished to do so, subject to  */
/* the following conditions:                                              */
/*                                                                        */
/* The above copyright notice and this permission notice shall be         */
/* included in all copies or substantial portions of the Software.        */
/*                                                                        */
/* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,        */
/* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF     */
/* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. */
/* IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY   */
/* CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,   */
/* TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE      */
/* SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                 */
/**************************************************************************/

#include "wasm_language.h"
#include "wasm_script.h"

#ifdef WAMR_ENABLED
#include "wasm_export.h"

// ---------------------------------------------------------------------------
// Host import: godot_print
// Called from WASM as: (import "godot" "godot_print" (func (param i32 i32)))
// argv[0] = UTF-8 string pointer in WASM linear memory, argv[1] = byte length
// ---------------------------------------------------------------------------
static void _wasm_host_godot_print(wasm_exec_env_t exec_env, int32_t ptr, int32_t len) {
	wasm_module_inst_t inst = wasm_runtime_get_module_inst(exec_env);
	if (len <= 0) {
		return;
	}
	if (!wasm_runtime_validate_app_addr(inst, (uint32_t)ptr, (uint32_t)len)) {
		ERR_PRINT("WasmScript: godot_print called with out-of-bounds memory pointer.");
		return;
	}
	const char *native_ptr = (const char *)wasm_runtime_addr_app_to_native(inst, (uint32_t)ptr);
	String text = String::utf8(native_ptr, len);
	print_line(text);
}

static NativeSymbol _wasm_native_symbols[] = {
	{ "godot_print", (void *)_wasm_host_godot_print, "(ii)", nullptr },
};
#endif // WAMR_ENABLED

WasmLanguage *WasmLanguage::singleton = nullptr;

WasmLanguage::WasmLanguage() {
	singleton = this;
}

WasmLanguage::~WasmLanguage() {
	singleton = nullptr;
}

void WasmLanguage::init() {
#ifdef WAMR_ENABLED
	RuntimeInitArgs init_args;
	memset(&init_args, 0, sizeof(RuntimeInitArgs));
	// Use interpreter mode for maximum portability (no JIT).
	init_args.running_mode = Mode_Interp;
	init_args.mem_alloc_type = Alloc_With_Allocator;
	// WAMR's RuntimeInitArgs stores allocator function pointers as void* fields
	// to remain C-compatible. The casts are required and match WAMR's usage.
	init_args.mem_alloc_option.allocator.malloc_func = (void *)malloc;
	init_args.mem_alloc_option.allocator.realloc_func = (void *)realloc;
	init_args.mem_alloc_option.allocator.free_func = (void *)free;
	if (!wasm_runtime_full_init(&init_args)) {
		ERR_PRINT("WasmLanguage: failed to initialize WAMR runtime.");
		return;
	}
	// Register host bindings (module name "godot").
	if (!wasm_runtime_register_natives("godot", _wasm_native_symbols, sizeof(_wasm_native_symbols) / sizeof(_wasm_native_symbols[0]))) {
		ERR_PRINT("WasmLanguage: failed to register native symbols.");
	}
#endif
}

void WasmLanguage::finish() {
#ifdef WAMR_ENABLED
	wasm_runtime_destroy();
#endif
}

void WasmLanguage::get_reserved_words(List<String> *p_words) const {
	// WAT keywords
	static const char *_reserved[] = {
		"module", "func", "param", "result", "local", "global",
		"memory", "table", "import", "export", "start",
		"data", "elem", "type", "if", "then", "else", "end",
		"block", "loop", "br", "br_if", "br_table", "return",
		"call", "call_indirect", "drop", "select",
		"get_local", "set_local", "tee_local",
		"get_global", "set_global",
		"i32", "i64", "f32", "f64",
		"i32.const", "i64.const", "f32.const", "f64.const",
		nullptr
	};
	for (int i = 0; _reserved[i]; i++) {
		p_words->push_back(_reserved[i]);
	}
}

void WasmLanguage::get_comment_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back(";; "); // Line comment
	p_delimiters->push_back("(; ;)"); // Block comment
}

void WasmLanguage::get_string_delimiters(List<String> *p_delimiters) const {
	p_delimiters->push_back("\" \"");
}

bool WasmLanguage::validate(const String &p_script, int &r_line_error, int &r_col_error, String &r_test_error, const String &p_path, List<String> *r_functions, List<Warning> *r_warnings, Set<int> *r_safe_lines) const {
	// Basic validation: check that the text starts with a WASM module definition.
	String stripped = p_script.strip_edges();
	if (!stripped.begins_with("(module")) {
		r_line_error = 1;
		r_col_error = 1;
		r_test_error = "WAT script must begin with '(module ...)'";
		return false;
	}
	return true;
}

Script *WasmLanguage::create_script() const {
	return memnew(WasmScript);
}

void WasmLanguage::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("wscript");
}

