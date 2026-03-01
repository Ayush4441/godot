/**************************************************************************/
/*  wasm_script.cpp                                                       */
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

#include "wasm_script.h"

#include "core/os/file_access.h"
#include "core/os/os.h"
#include "wasm_instance.h"
#include "wasm_language.h"

#ifdef WAMR_ENABLED
#include "wasm_export.h"
#endif

// ---------------------------------------------------------------------------
// WasmScript
// ---------------------------------------------------------------------------

WasmScript::WasmScript() {}

WasmScript::~WasmScript() {
	_unload_wasm_binary();
}

void WasmScript::_bind_methods() {
	ClassDB::bind_method(D_METHOD("set_wasm_cache_path", "path"), &WasmScript::set_wasm_cache_path);
	ClassDB::bind_method(D_METHOD("get_wasm_cache_path"), &WasmScript::get_wasm_cache_path);
	ADD_PROPERTY(PropertyInfo(Variant::STRING, "wasm_cache_path"), "set_wasm_cache_path", "get_wasm_cache_path");
}

bool WasmScript::_load_wasm_binary() {
#ifndef WAMR_ENABLED
	valid = false;
	ERR_PRINT("WasmScript: WAMR runtime not available. Rebuild with WAMR enabled.");
	return false;
#else
	if (wasm_bytes.size() == 0) {
		ERR_PRINT("WasmScript: no WASM binary data loaded.");
		valid = false;
		return false;
	}

	char error_buf[128];
	wasm_module_t mod = wasm_runtime_load(
			(uint8_t *)wasm_bytes.ptr(),
			(uint32_t)wasm_bytes.size(),
			error_buf,
			sizeof(error_buf));
	if (!mod) {
		ERR_PRINT(String("WasmScript: failed to load WASM module: ") + error_buf);
		valid = false;
		return false;
	}

	wamr_module = (void *)mod;
	valid = true;
	return true;
#endif
}

void WasmScript::_unload_wasm_binary() {
#ifdef WAMR_ENABLED
	if (wamr_module) {
		wasm_runtime_unload((wasm_module_t)wamr_module);
		wamr_module = nullptr;
	}
#endif
	valid = false;
}

bool WasmScript::can_instance() const {
	return valid;
}

ScriptInstance *WasmScript::instance_create(Object *p_this) {
	if (!valid) {
		return nullptr;
	}
	WasmInstance *inst = memnew(WasmInstance);
	if (!inst->initialize(p_this, Ref<WasmScript>(this))) {
		memdelete(inst);
		return nullptr;
	}
	instances.insert(inst);
	return inst;
}

bool WasmScript::instance_has(const Object *p_this) const {
	for (const Set<WasmInstance *>::Element *E = instances.front(); E; E = E->next()) {
		if (E->get()->get_owner() == p_this) {
			return true;
		}
	}
	return false;
}

Error WasmScript::reload(bool p_keep_state) {
	_unload_wasm_binary();

	// If a compiled WASM binary cache path is set, load from there.
	if (!wasm_cache_path.empty()) {
		Error err;
		// FileAccess::get_file_as_array() was available in Godot 3.x to read
		// a file's raw bytes into a Vector<uint8_t>.
		wasm_bytes = FileAccess::get_file_as_array(wasm_cache_path, &err);
		if (err != OK || wasm_bytes.size() == 0) {
			ERR_PRINT("WasmScript: could not read WASM binary at '" + wasm_cache_path + "'.");
			return ERR_FILE_NOT_FOUND;
		}
		if (!_load_wasm_binary()) {
			return FAILED;
		}
		return OK;
	}

	// No binary available yet (e.g. import hasn't run).
	return ERR_UNAVAILABLE;
}

bool WasmScript::has_method(const StringName &p_method) const {
	// Supported lifecycle exports.
	return p_method == StringName("gd_ready") || p_method == StringName("gd_process");
}

void WasmScript::get_script_method_list(List<MethodInfo> *p_list) const {
	MethodInfo ready;
	ready.name = "gd_ready";
	ready.arguments.push_back(PropertyInfo(Variant::INT, "self_handle"));
	p_list->push_back(ready);

	MethodInfo process;
	process.name = "gd_process";
	process.arguments.push_back(PropertyInfo(Variant::INT, "self_handle"));
	process.arguments.push_back(PropertyInfo(Variant::REAL, "delta"));
	p_list->push_back(process);
}

ScriptLanguage *WasmScript::get_language() const {
	return WasmLanguage::get_singleton();
}

// ---------------------------------------------------------------------------
// ResourceFormatLoaderWasm
// ---------------------------------------------------------------------------

RES ResourceFormatLoaderWasm::load(const String &p_path, const String &p_original_path, Error *r_error, bool p_no_subresource_cache) {
	Ref<WasmScript> script;
	script.instance();

	Error err;
	String source = FileAccess::get_file_as_string(p_path, &err);
	if (err != OK) {
		if (r_error) {
			*r_error = err;
		}
		ERR_PRINT("WasmScript: failed to open script file '" + p_path + "'.");
		return RES();
	}

	script->set_source_code(source);
	script->set_path(p_original_path.empty() ? p_path : p_original_path);

	if (r_error) {
		*r_error = OK;
	}
	return script;
}

void ResourceFormatLoaderWasm::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("wscript");
}

bool ResourceFormatLoaderWasm::handles_type(const String &p_type) const {
	return p_type == "WasmScript" || p_type == "Script";
}

String ResourceFormatLoaderWasm::get_resource_type(const String &p_path) const {
	if (p_path.get_extension().to_lower() == "wscript") {
		return "WasmScript";
	}
	return String();
}

// ---------------------------------------------------------------------------
// ResourceFormatSaverWasm
// ---------------------------------------------------------------------------

Error ResourceFormatSaverWasm::save(const String &p_path, const RES &p_resource, uint32_t p_flags) {
	Ref<WasmScript> script = p_resource;
	ERR_FAIL_COND_V(script.is_null(), ERR_INVALID_PARAMETER);

	FileAccess *fa = FileAccess::open(p_path, FileAccess::WRITE);
	ERR_FAIL_COND_V_MSG(!fa, ERR_CANT_CREATE, "WasmScript: cannot write to '" + p_path + "'.");

	String src = script->get_source_code();
	fa->store_string(src);
	memdelete(fa);
	return OK;
}

bool ResourceFormatSaverWasm::recognize(const RES &p_resource) const {
	return p_resource->get_class_name() == StringName("WasmScript");
}

void ResourceFormatSaverWasm::get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const {
	if (recognize(p_resource)) {
		p_extensions->push_back("wscript");
	}
}

