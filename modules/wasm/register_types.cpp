/**************************************************************************/
/*  register_types.cpp                                                    */
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

#include "register_types.h"

#include "core/class_db.h"
#include "core/io/resource_loader.h"
#include "core/io/resource_saver.h"
#include "core/script_language.h"
#include "wasm_language.h"
#include "wasm_script.h"

#ifdef TOOLS_ENABLED
#include "core/io/resource_importer.h"
#include "editor/editor_node.h"
#include "editor/resource_importer_wasm.h"
#endif

static WasmLanguage *wasm_language = nullptr;
static Ref<ResourceFormatLoaderWasm> resource_loader_wasm;
static Ref<ResourceFormatSaverWasm> resource_saver_wasm;

#ifdef TOOLS_ENABLED
static void _editor_init() {
	Ref<ResourceImporterWasm> wasm_import;
	wasm_import.instance();
	ResourceFormatImporter::get_singleton()->add_importer(wasm_import);
}
#endif

void register_wasm_types() {
	ClassDB::register_class<WasmScript>();

	wasm_language = memnew(WasmLanguage);
	ScriptServer::register_language(wasm_language);

	resource_loader_wasm.instance();
	ResourceLoader::add_resource_format_loader(resource_loader_wasm);

	resource_saver_wasm.instance();
	ResourceSaver::add_resource_format_saver(resource_saver_wasm);

#ifdef TOOLS_ENABLED
	ClassDB::register_class<ResourceImporterWasm>();
	EditorNode::add_init_callback(_editor_init);
#endif
}

void unregister_wasm_types() {
	if (wasm_language) {
		ScriptServer::unregister_language(wasm_language);
		memdelete(wasm_language);
		wasm_language = nullptr;
	}

	ResourceLoader::remove_resource_format_loader(resource_loader_wasm);
	resource_loader_wasm.unref();

	ResourceSaver::remove_resource_format_saver(resource_saver_wasm);
	resource_saver_wasm.unref();
}

