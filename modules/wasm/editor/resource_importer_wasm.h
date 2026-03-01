/**************************************************************************/
/*  resource_importer_wasm.h                                              */
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

#pragma once

#include "core/io/resource_importer.h"

// Imports a .wscript (WAT text) file and compiles it to a .wasm binary.
//
// The compilation step relies on an external `wat2wasm` tool from WABT
// (https://github.com/WebAssembly/wabt) being available on PATH.
// If wat2wasm is not found the import step stores the raw source bytes as
// a placeholder, and a clear error is emitted to help the user.
//
// TODO: Link against libwabt for in-process compilation so the tool is
//       not required as a separate install.
class ResourceImporterWasm : public ResourceImporter {
	GDCLASS(ResourceImporterWasm, ResourceImporter);

public:
	virtual String get_importer_name() const override { return "wasm_script"; }
	virtual String get_visible_name() const override { return "WebAssembly Script (WAT)"; }
	virtual void get_recognized_extensions(List<String> *p_extensions) const override;
	virtual String get_save_extension() const override { return "wasm"; }
	virtual String get_resource_type() const override { return "WasmScript"; }
	virtual float get_priority() const override { return 1.0f; }
	virtual int get_import_order() const override { return 0; }
	virtual int get_preset_count() const override { return 0; }
	virtual String get_preset_name(int p_idx) const override { return String(); }
	virtual void get_import_options(const String &p_path, List<ImportOption> *r_options, int p_preset = 0) const override {}
	virtual bool get_option_visibility(const String &p_path, const String &p_option, const HashMap<StringName, Variant> &p_options) const override { return true; }
	virtual bool can_import_threaded() const override { return false; }

	virtual Error import(ResourceUID::ID p_source_id, const String &p_source_file, const String &p_save_path,
			const HashMap<StringName, Variant> &p_options, List<String> *r_platform_variants,
			List<String> *r_gen_files = nullptr, Variant *r_metadata = nullptr) override;
};
