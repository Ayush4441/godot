/**************************************************************************/
/*  resource_importer_wasm.cpp                                            */
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

#include "resource_importer_wasm.h"

#include "core/os/dir_access.h"
#include "core/os/file_access.h"
#include "core/os/os.h"

void ResourceImporterWasm::get_recognized_extensions(List<String> *p_extensions) const {
	p_extensions->push_back("wscript");
}

Error ResourceImporterWasm::import(
		const String &p_source_file,
		const String &p_save_path,
		const Map<StringName, Variant> &p_options,
		List<String> *r_platform_variants,
		List<String> *r_gen_files,
		Variant *r_metadata) {
	// Read the WAT source text.
	Error err;
	String wat_source = FileAccess::get_file_as_string(p_source_file, &err);
	if (err != OK) {
		ERR_PRINT("ResourceImporterWasm: cannot read source file '" + p_source_file + "'.");
		return err;
	}

	// Attempt to compile WAT -> WASM using the `wat2wasm` command-line tool
	// from WABT (https://github.com/WebAssembly/wabt).
	//
	// TODO: Link against libwabt for in-process compilation to remove the
	//       dependency on an external binary.

	String tmp_wat = OS::get_singleton()->get_cache_path().plus_file("_tmp_import.wat");
	String out_wasm = p_save_path + ".wasm";

	// Write WAT to a temp file.
	{
		FileAccess *fa = FileAccess::open(tmp_wat, FileAccess::WRITE);
		ERR_FAIL_COND_V_MSG(!fa, ERR_CANT_CREATE, "ResourceImporterWasm: cannot create temporary WAT file.");
		fa->store_string(wat_source);
		memdelete(fa);
	}

	// Run wat2wasm.
	List<String> args;
	args.push_back(tmp_wat);
	args.push_back("-o");
	args.push_back(out_wasm);

	int exit_code = -1;
	// p_blocking=true so we wait for the tool to finish.
	err = OS::get_singleton()->execute("wat2wasm", args, true, nullptr, nullptr, &exit_code);

	// Clean up temp file.
	DirAccess::remove_file_or_error(tmp_wat);

	if (err != OK) {
		// wat2wasm binary not found on PATH.
		WARN_PRINT(
				"ResourceImporterWasm: 'wat2wasm' not found. "
				"Install WABT (https://github.com/WebAssembly/wabt) and ensure "
				"'wat2wasm' is available on PATH. Storing raw WAT source as placeholder.");
		Vector<uint8_t> src_bytes = wat_source.to_utf8();
		FileAccess *fa = FileAccess::open(out_wasm, FileAccess::WRITE);
		if (!fa) {
			ERR_PRINT("ResourceImporterWasm: cannot write output '" + out_wasm + "'.");
			return ERR_CANT_CREATE;
		}
		fa->store_buffer(src_bytes.ptr(), src_bytes.size());
		memdelete(fa);
		return OK;
	}

	if (exit_code != 0) {
		// wat2wasm found but compilation failed (syntax error in WAT source).
		ERR_PRINT("ResourceImporterWasm: 'wat2wasm' failed with exit code " + itos(exit_code) +
				". Check your WAT source for errors: " + p_source_file);
		return ERR_PARSE_ERROR;
	}

	return OK;
}

