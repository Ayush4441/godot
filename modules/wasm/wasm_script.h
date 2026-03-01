/**************************************************************************/
/*  wasm_script.h                                                         */
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

#ifndef WASM_SCRIPT_H
#define WASM_SCRIPT_H

#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/script_language.h"

class WasmInstance;
class WasmLanguage;

// WasmScript represents a compiled WASM module.
// The source (.wscript / WAT text) is imported to a .wasm binary by
// ResourceImporterWasm (editor) which is then loaded here at runtime via WAMR.
class WasmScript : public Script {
	GDCLASS(WasmScript, Script);

	friend class WasmInstance;
	friend class WasmLanguage;

	String source_code; // WAT text source (stored in the .wscript file)
	String wasm_cache_path; // Path to compiled .wasm binary in .import cache
	bool valid = false;

	// Active instances (weak refs, instances remove themselves on destruction)
	Set<WasmInstance *> instances;

	// Loaded WASM binary bytes (populated from wasm_cache_path at load time)
	Vector<uint8_t> wasm_bytes;

#ifdef WAMR_ENABLED
	// WAMR module handle; populated in _load_wasm_binary().
	// Type is void* so we don't require WAMR headers when WAMR is absent.
	void *wamr_module = nullptr;
#endif

	bool _load_wasm_binary();
	void _unload_wasm_binary();

protected:
	static void _bind_methods();

public:
	// Script interface
	virtual bool can_instance() const override;
	virtual Ref<Script> get_base_script() const override { return Ref<Script>(); }
	virtual bool inherits_script(const Ref<Script> &p_script) const override { return false; }
	virtual StringName get_instance_base_type() const override { return StringName("Node"); }
	virtual ScriptInstance *instance_create(Object *p_this) override;
	virtual bool instance_has(const Object *p_this) const override;
	virtual bool has_source_code() const override { return !source_code.empty(); }
	virtual String get_source_code() const override { return source_code; }
	virtual void set_source_code(const String &p_code) override { source_code = p_code; }
	virtual Error reload(bool p_keep_state = false) override;
	virtual bool has_method(const StringName &p_method) const override;
	virtual MethodInfo get_method_info(const StringName &p_method) const override { return MethodInfo(); }
	virtual bool is_tool() const override { return false; }
	virtual bool is_valid() const override { return valid; }
	virtual ScriptLanguage *get_language() const override;
	virtual bool has_script_signal(const StringName &p_signal) const override { return false; }
	virtual void get_script_signal_list(List<MethodInfo> *r_signals) const override {}
	virtual bool get_property_default_value(const StringName &p_property, Variant &r_value) const override { return false; }
	virtual void get_script_method_list(List<MethodInfo> *p_list) const override;
	virtual void get_script_property_list(List<PropertyInfo> *p_list) const override {}

	// Path to compiled WASM binary cache
	void set_wasm_cache_path(const String &p_path) { wasm_cache_path = p_path; }
	String get_wasm_cache_path() const { return wasm_cache_path; }
	WasmScript();
	~WasmScript();
};

// Resource loader for .wscript files.
class ResourceFormatLoaderWasm : public ResourceFormatLoader {
public:
	virtual RES load(const String &p_path, const String &p_original_path = "", Error *r_error = nullptr, bool p_no_subresource_cache = false) override;
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual bool handles_type(const String &p_type) const override;
	virtual String get_resource_type(const String &p_path) const override;
};

// Resource saver for .wscript files.
class ResourceFormatSaverWasm : public ResourceFormatSaver {
public:
	virtual Error save(const String &p_path, const RES &p_resource, uint32_t p_flags = 0) override;
	virtual bool recognize(const RES &p_resource) const override;
	virtual void get_recognized_extensions(const RES &p_resource, List<String> *p_extensions) const override;
};

#endif // WASM_SCRIPT_H
