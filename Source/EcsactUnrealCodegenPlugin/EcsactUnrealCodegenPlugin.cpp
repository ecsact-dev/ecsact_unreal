#include <format>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using ecsact::cpp_codegen_plugin_util::block;
using ecsact::cpp_codegen_plugin_util::inc_header;
using ecsact::cpp_codegen_plugin_util::inc_package_header;

constexpr int32_t GENERATED_HEADER_INDEX = 0;
constexpr int32_t GENERATED_SOURCE_INDEX = 1;

inline auto inc_package_header_no_ext( //
	ecsact::codegen_plugin_context& ctx,
	ecsact_package_id               pkg_id,
	std::string                     extension = ".hh"
) -> void {
	namespace fs = std::filesystem;

	auto main_ecsact_file_path = ecsact::meta::package_file_path(ctx.package_id);
	if(ctx.package_id == pkg_id) {
		main_ecsact_file_path.replace_extension(
			main_ecsact_file_path.extension().string() + extension
		);

		inc_header(ctx, main_ecsact_file_path.filename().string());
	} else {
		auto cpp_header_path =
			ecsact::meta::package_file_path(pkg_id).replace_extension("");
		cpp_header_path.replace_extension(
			cpp_header_path.extension().string() + extension
		);
		if(main_ecsact_file_path.has_parent_path()) {
			cpp_header_path =
				fs::relative(cpp_header_path, main_ecsact_file_path.parent_path());
		}
		inc_header(ctx, cpp_header_path.filename().string());
	}
}

auto ecsact_codegen_output_filenames( //
	ecsact_package_id package_id,
	char* const*      out_filenames,
	int32_t           max_filenames,
	int32_t           max_filename_length,
	int32_t*          out_filenames_length
) -> void {
	auto pkg_basename = //
		ecsact::meta::package_file_path(package_id)
			.filename()
			.replace_extension("")
			.string();
	ecsact::set_codegen_plugin_output_filenames(
		{
			pkg_basename + "__ecsact__ue.h", // GENERATED_HEADER_INDEX
			pkg_basename + "__ecsact__ue.cpp", // GENERATED_SOURCE_INDEX
		},
		out_filenames,
		max_filenames,
		max_filename_length,
		out_filenames_length
	);
}

auto ecsact_codegen_plugin_name() -> const char* {
	return "Unreal";
}

static auto generate_header(ecsact::codegen_plugin_context& ctx) -> void {
	ctx.writef("#pragma once\n\n");

	inc_header(ctx, "CoreMinimal.h");
	inc_header(ctx, "UObject/Interface.h");
	inc_package_header(ctx, package_id, ".hh");
	inc_package_header_no_ext(ctx, package_id, "__ecsact__ue.generated.h");

	ctx.writef("\n\n");

	for(auto comp_id : ecsact::meta::get_component_ids(package_id)) {
		auto comp_name = ecsact::meta::component_name(comp_id);
		ctx.writef("UINTERFACE(MinimalAPI, Blueprintable)\n");
		block(
			ctx,
			std::format("class U{}Interface : public UInterface", comp_name),
			[&] { ctx.writef("GENERATED_BODY()"); }
		);
		ctx.writef(";\n");

		block(
			ctx,
			std::format("class I{}Interface : public UInterface", comp_name),
			[&] {
				ctx.writef("GENERATED_BODY()\n");
				ctx.indentation -= 1;
				ctx.writef("\n");
				ctx.writef("public:");
				ctx.indentation += 1;
				ctx.writef("\n");
			}
		);
		ctx.writef(";\n");
	}
}

static auto generate_source(ecsact::codegen_plugin_context& ctx) -> void {
}

auto ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) -> void {
	generate_header({package_id, GENERATED_HEADER_INDEX, write_fn, report_fn});
	generate_source({package_id, GENERATED_SOURCE_INDEX, write_fn, report_fn});
}
