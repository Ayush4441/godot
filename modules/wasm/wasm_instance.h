/**************************************************************************/
/*  wasm_instance.h                                                       */
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

#ifndef WASM_INSTANCE_H
#define WASM_INSTANCE_H

#include "core/io/multiplayer_api.h"
#include "core/script_language.h"
#include "wasm_script.h"

// Per-Object WASM instance.
// Each Node/Object that has a WasmScript attached gets its own WasmInstance,
// which wraps a separate WAMR module instance so state is isolated.
class WasmInstance : public ScriptInstance {
	Object *owner = nullptr;
	Ref<WasmScript> script;

#ifdef WAMR_ENABLED
	// WAMR module instance and execution environment.
	// Typed as void* to avoid requiring WAMR headers at every include site.
	void *wamr_instance = nullptr; // wasm_module_inst_t
	void *wamr_exec_env = nullptr; // wasm_exec_env_t
#endif

	bool _call_wasm_function(const char *p_name, uint32_t *p_argv, uint32_t p_argc);

public:
	virtual bool set(const StringName &p_name, const Variant &p_value) override { return false; }
	virtual bool get(const StringName &p_name, Variant &r_ret) const override { return false; }
	virtual void get_property_list(List<PropertyInfo> *p_properties) const override {}
	virtual Variant::Type get_property_type(const StringName &p_name, bool *r_is_valid = nullptr) const override {
		if (r_is_valid) {
			*r_is_valid = false;
		}
		return Variant::NIL;
	}

	virtual Object *get_owner() override { return owner; }
	virtual void get_method_list(List<MethodInfo> *p_list) const override;
	virtual bool has_method(const StringName &p_method) const override;
	virtual Variant call(const StringName &p_method, const Variant **p_args, int p_argcount, Variant::CallError &r_error) override;
	virtual void notification(int p_notification) override;
	virtual Ref<Script> get_script() const override { return script; }
	virtual ScriptLanguage *get_language() override;

	// Godot 3.x requires these RPC mode virtuals on ScriptInstance
	virtual MultiplayerAPI::RPCMode get_rpc_mode(const StringName &p_method) const override { return MultiplayerAPI::RPC_MODE_DISABLED; }
	virtual MultiplayerAPI::RPCMode get_rset_mode(const StringName &p_variable) const override { return MultiplayerAPI::RPC_MODE_DISABLED; }

	bool initialize(Object *p_owner, const Ref<WasmScript> &p_script);

	WasmInstance();
	~WasmInstance();
};

#endif // WASM_INSTANCE_H
