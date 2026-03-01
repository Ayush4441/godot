/**************************************************************************/
/*  wasm_instance.cpp                                                     */
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

#include "wasm_instance.h"

#include "core/os/os.h"
#include "scene/main/node.h"
#include "wasm_language.h"

#ifdef WAMR_ENABLED
#include "wasm_export.h"

// Stack size and heap size per WASM instance (in bytes).
// These values are intentionally conservative defaults. Increase them if
// your WASM modules require more memory (e.g. complex data structures).
// TODO: Expose as ProjectSettings keys (wasm/instance/stack_size_kb etc.)
static constexpr uint32_t WASM_STACK_SIZE = 64 * 1024; // 64 KB
static constexpr uint32_t WASM_HEAP_SIZE = 64 * 1024; // 64 KB
#endif

WasmInstance::WasmInstance() {}

WasmInstance::~WasmInstance() {
#ifdef WAMR_ENABLED
	if (wamr_exec_env) {
		wasm_runtime_destroy_exec_env((wasm_exec_env_t)wamr_exec_env);
		wamr_exec_env = nullptr;
	}
	if (wamr_instance) {
		wasm_runtime_deinstantiate((wasm_module_inst_t)wamr_instance);
		wamr_instance = nullptr;
	}
#endif
	if (script.is_valid()) {
		script->instances.erase(this);
	}
}

bool WasmInstance::initialize(Object *p_owner, const Ref<WasmScript> &p_script) {
	owner = p_owner;
	script = p_script;

#ifndef WAMR_ENABLED
	ERR_PRINT("WasmInstance: WAMR runtime not available.");
	return false;
#else
	ERR_FAIL_COND_V(!script.is_valid() || !script->wamr_module, false);

	char error_buf[128];
	wasm_module_inst_t inst = wasm_runtime_instantiate(
			(wasm_module_t)script->wamr_module,
			WASM_STACK_SIZE,
			WASM_HEAP_SIZE,
			error_buf,
			sizeof(error_buf));
	if (!inst) {
		ERR_PRINT(vformat("WasmInstance: failed to instantiate WASM module: %s", error_buf));
		return false;
	}
	wamr_instance = (void *)inst;

	wasm_exec_env_t exec_env = wasm_runtime_create_exec_env(inst, WASM_STACK_SIZE);
	if (!exec_env) {
		ERR_PRINT("WasmInstance: failed to create WASM execution environment.");
		wasm_runtime_deinstantiate(inst);
		wamr_instance = nullptr;
		return false;
	}
	wamr_exec_env = (void *)exec_env;
	return true;
#endif
}

bool WasmInstance::_call_wasm_function(const char *p_name, uint32_t *p_argv, uint32_t p_argc) {
#ifndef WAMR_ENABLED
	return false;
#else
	if (!wamr_instance || !wamr_exec_env) {
		return false;
	}
	wasm_function_inst_t fn = wasm_runtime_lookup_function(
			(wasm_module_inst_t)wamr_instance, p_name, nullptr);
	if (!fn) {
		return false; // Function not exported – not an error.
	}
	if (!wasm_runtime_call_wasm((wasm_exec_env_t)wamr_exec_env, fn, p_argc, p_argv)) {
		const char *err = wasm_runtime_get_exception((wasm_module_inst_t)wamr_instance);
		ERR_PRINT(vformat("WasmInstance: exception in '%s': %s", p_name, err ? err : "(unknown)"));
		wasm_runtime_clear_exception((wasm_module_inst_t)wamr_instance);
		return false;
	}
	return true;
#endif
}

void WasmInstance::get_method_list(List<MethodInfo> *p_list) const {
	if (script.is_valid()) {
		script->get_script_method_list(p_list);
	}
}

bool WasmInstance::has_method(const StringName &p_method) const {
	return script.is_valid() && script->has_method(p_method);
}

Variant WasmInstance::callp(const StringName &p_method, const Variant **p_args, int p_argcount, Callable::CallError &r_error) {
	r_error.error = Callable::CallError::CALL_OK;

#ifdef WAMR_ENABLED
	// Use the Object's stable 64-bit ID as the self handle.
	// We pass the lower 32 bits as an i32 argument since WASM MVP only has i32/i64.
	// NOTE: On 64-bit platforms this truncates the ID to 32 bits. For a production
	// implementation, switch the WASM function signatures to use i64 and pass two
	// i32 argv words, or maintain an index-based object table in WASM memory.
	uint32_t self_handle = (uint32_t)(owner->get_instance_id() & 0xFFFFFFFF);

	if (p_method == StringName("gd_ready")) {
		uint32_t argv[1] = { self_handle };
		_call_wasm_function("gd_ready", argv, 1);
		return Variant();
	}

	if (p_method == StringName("gd_process")) {
		if (p_argcount < 1) {
			r_error.error = Callable::CallError::CALL_ERROR_TOO_FEW_ARGUMENTS;
			r_error.expected = 1;
			return Variant();
		}
		float delta = (float)(double)(*p_args[0]);
		// Pack float bits into uint32 for WAMR argv.
		uint32_t delta_bits;
		memcpy(&delta_bits, &delta, sizeof(delta_bits));
		uint32_t argv[2] = { self_handle, delta_bits };
		_call_wasm_function("gd_process", argv, 2);
		return Variant();
	}
#endif

	r_error.error = Callable::CallError::CALL_ERROR_INVALID_METHOD;
	return Variant();
}

void WasmInstance::notification(int p_notification, bool p_reversed) {
	// Map engine notifications to WASM lifecycle exports.
	if (p_notification == Node::NOTIFICATION_READY) {
		Callable::CallError ce;
		callp(StringName("gd_ready"), nullptr, 0, ce);
	}
}

ScriptLanguage *WasmInstance::get_language() {
	return WasmLanguage::get_singleton();
}
