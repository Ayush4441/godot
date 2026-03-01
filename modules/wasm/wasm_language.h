/**************************************************************************/
/*  wasm_language.h                                                       */
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

#ifndef WASM_LANGUAGE_H
#define WASM_LANGUAGE_H

#include "core/script_language.h"

class WasmLanguage : public ScriptLanguage {
	static WasmLanguage *singleton;

public:
	static WasmLanguage *get_singleton() { return singleton; }

	WasmLanguage();
	~WasmLanguage();

	virtual String get_name() const override { return "Wasm"; }

	/* LANGUAGE FUNCTIONS */
	virtual void init() override;
	virtual String get_type() const override { return "WasmScript"; }
	virtual String get_extension() const override { return "wscript"; }
	virtual Error execute_file(const String &p_path) override { return ERR_UNAVAILABLE; }
	virtual void finish() override;

	/* EDITOR FUNCTIONS */
	virtual void get_reserved_words(List<String> *p_words) const override;
	virtual bool is_control_flow_keyword(String p_string) const override { return false; }
	virtual void get_comment_delimiters(List<String> *p_delimiters) const override;
	virtual void get_string_delimiters(List<String> *p_delimiters) const override;
	virtual Ref<Script> get_template(const String &p_class_name, const String &p_base_class_name) const override { return Ref<Script>(); }
	virtual bool validate(const String &p_script, int &r_line_error, int &r_col_error, String &r_test_error, const String &p_path = "", List<String> *r_functions = nullptr, List<Warning> *r_warnings = nullptr, Set<int> *r_safe_lines = nullptr) const override;
	virtual Script *create_script() const override;
	virtual bool has_named_classes() const override { return false; }
	virtual bool supports_builtin_mode() const override { return false; }
	virtual int find_function(const String &p_function, const String &p_code) const override { return -1; }
	virtual String make_function(const String &p_class, const String &p_name, const PoolStringArray &p_args) const override { return String(); }
	virtual void auto_indent_code(String &p_code, int p_from_line, int p_to_line) const override {}
	virtual void add_global_constant(const StringName &p_variable, const Variant &p_value) override {}

	/* DEBUGGER FUNCTIONS */
	virtual String debug_get_error() const override { return String(); }
	virtual int debug_get_stack_level_count() const override { return 0; }
	virtual int debug_get_stack_level_line(int p_level) const override { return -1; }
	virtual String debug_get_stack_level_function(int p_level) const override { return String(); }
	virtual String debug_get_stack_level_source(int p_level) const override { return String(); }
	virtual void debug_get_stack_level_locals(int p_level, List<String> *p_locals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {}
	virtual void debug_get_stack_level_members(int p_level, List<String> *p_members, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {}
	virtual void debug_get_globals(List<String> *p_globals, List<Variant> *p_values, int p_max_subitems = -1, int p_max_depth = -1) override {}
	virtual String debug_parse_stack_level_expression(int p_level, const String &p_expression, int p_max_subitems = -1, int p_max_depth = -1) override { return String(); }

	/* LOADER FUNCTIONS */
	virtual void reload_all_scripts() override {}
	virtual void reload_tool_script(const Ref<Script> &p_script, bool p_soft_reload) override {}

	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual void get_public_functions(List<MethodInfo> *p_functions) const override {}
	virtual void get_public_constants(List<Pair<String, Variant>> *p_constants) const override {}

	/* PROFILING */
	virtual void profiling_start() override {}
	virtual void profiling_stop() override {}
	virtual int profiling_get_accumulated_data(ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }
	virtual int profiling_get_frame_data(ProfilingInfo *p_info_arr, int p_info_max) override { return 0; }
};

#endif // WASM_LANGUAGE_H
