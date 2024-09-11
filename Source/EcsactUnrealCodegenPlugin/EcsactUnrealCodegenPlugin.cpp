#include <format>
#include <array>
#include <sstream>
#include <string>
#include "ecsact/runtime/meta.hh"
#include "ecsact/codegen/plugin.hh"
#include "ecsact/lang-support/lang-cc.hh"
#include "ecsact/cpp_codegen_plugin_util.hh"

using namespace std::string_literals;

using ecsact::cc_lang_support::c_identifier;
using ecsact::cc_lang_support::cpp_identifier;
using ecsact::cpp_codegen_plugin_util::block;
using ecsact::cpp_codegen_plugin_util::inc_header;
using ecsact::cpp_codegen_plugin_util::inc_package_header;

constexpr int32_t GENERATED_HEADER_INDEX = 0;
constexpr int32_t GENERATED_SOURCE_INDEX = 1;

inline auto inc_package_header_no_ext( //
	ecsact::codegen_plugin_context& ctx,
	ecsact_package_id               pkg_id,
	std::string                     suffix,
	std::string                     header_extension = ".h"
) -> void {
	namespace fs = std::filesystem;

	auto main_ecsact_file_path = ecsact::meta::package_file_path(ctx.package_id);
	if(ctx.package_id == pkg_id) {
		main_ecsact_file_path =
			fs::path{main_ecsact_file_path.replace_extension("").string() + suffix}
				.replace_extension(header_extension);

		inc_header(ctx, main_ecsact_file_path.filename().string());
	} else {
		auto cpp_header_path = fs::path{
			ecsact::meta::package_file_path(pkg_id).replace_extension("").string() +
			suffix
		};
		cpp_header_path.replace_extension(header_extension);
		if(main_ecsact_file_path.has_parent_path()) {
			cpp_header_path =
				fs::relative(cpp_header_path, main_ecsact_file_path.parent_path());
		}
		inc_header(ctx, cpp_header_path.filename().string());
	}
}

auto ecsact_decl_name_to_pascal(const std::string& input) -> std::string {
	auto result = std::stringstream{};
	auto capitalize_next = true;

	for(char ch : input) {
		if(ch == '.' || ch == '_') {
			capitalize_next = true; // Capitalize the next character after a separator
		} else if(capitalize_next) {
			result << static_cast<char>(std::toupper(ch));
			capitalize_next = false;
		} else {
			result << static_cast<char>(std::tolower(ch));
		}
	}

	return result.str();
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
		std::array{
			pkg_basename + "__ecsact__ue.h"s, // GENERATED_HEADER_INDEX
			pkg_basename + "__ecsact__ue.cpp"s, // GENERATED_SOURCE_INDEX
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

static auto ecsact_type_to_unreal_type(
	ecsact::codegen_plugin_context& ctx,
	ecsact_builtin_type             type
) -> std::string {
	switch(type) {
		case ECSACT_BOOL:
			return "bool";
		case ECSACT_I8:
		case ECSACT_I16:
		case ECSACT_I32:
			return "int32";
		case ECSACT_U8:
		case ECSACT_U16:
		case ECSACT_U32:
			return "uint32";
		case ECSACT_F32:
			return "float";
		case ECSACT_ENTITY_TYPE:
			break;
		default:
			ctx.fatal(
				"Ecsact Unreal codegen plugin unknown builtin type ({}). Cannot "
				"proceed. Are you using the correct version of the Ecsact SDK with the "
				"correct version of the Ecsact Unreal codegen plugin?",
				static_cast<int>(type)
			);
			break;
	}

	return "";
}

static auto ecsact_type_to_unreal_type(
	ecsact::codegen_plugin_context& ctx,
	ecsact_field_type               type
) -> std::string {
	switch(type.kind) {
		case ECSACT_TYPE_KIND_BUILTIN:
			if(type.length == 1) {
				return ecsact_type_to_unreal_type(ctx, type.type.builtin);
			}
			ctx.fatal("Ecsact builtin array fields not supported yet");
			break;
		case ECSACT_TYPE_KIND_ENUM:
			ctx.fatal("Esact enum type not supported in yet");
			break;
		case ECSACT_TYPE_KIND_FIELD_INDEX:
			ctx.fatal("Ecsact field index not supported in unreal yet");
			break;
		default:
			ctx.fatal(
				"Ecsact Unreal codegen plugin unknown field type (kind={}). Cannot "
				"proceed. Are you using the correct version of the Ecsact SDK with the "
				"correct version of the Ecsact Unreal codegen plugin?",
				static_cast<int>(type.kind)
			);
			break;
	}

	return "";
}

static auto ecsact_ustruct_name(auto decl_id) -> std::string {
	auto name = ecsact::meta::decl_full_name(decl_id);
	auto pascal_name = ecsact_decl_name_to_pascal(name);
	return std::format("F{}", pascal_name);
}

template<typename T>
static auto uproperty_clamp_min_max() -> std::string {
	return std::format(
		R"(, Meta = (ClampMin = "{}", ClampMax = "{}"))",
		std::numeric_limits<T>::min(),
		std::numeric_limits<T>::max()
	);
}

static auto print_ecsact_type_uproperty(
	ecsact::codegen_plugin_context& ctx,
	ecsact_field_type               field_type
) -> void {
	ctx.write("UPROPERTY(EditAnywhere, BlueprintReadWrite");
	switch(field_type.kind) {
		case ECSACT_TYPE_KIND_BUILTIN:
			switch(field_type.type.builtin) {
				case ECSACT_BOOL:
					break;
				case ECSACT_I8:
					ctx.write(uproperty_clamp_min_max<int8_t>());
					break;
				case ECSACT_U8:
					ctx.write(uproperty_clamp_min_max<uint8_t>());
					break;
				case ECSACT_I16:
					ctx.write(uproperty_clamp_min_max<int16_t>());
					break;
				case ECSACT_U16:
					ctx.write(uproperty_clamp_min_max<uint16_t>());
					break;
				case ECSACT_I32:
					break;
				case ECSACT_U32:
					break;
				case ECSACT_F32:
					break;
				case ECSACT_ENTITY_TYPE:
					break;
			}
			break;
		case ECSACT_TYPE_KIND_ENUM:
			break;
		case ECSACT_TYPE_KIND_FIELD_INDEX:
			break;
	}

	ctx.write(")\n");
}

static auto print_ustruct(ecsact::codegen_plugin_context& ctx, auto in_compo_id)
	-> void {
	auto compo_id = ecsact_id_cast<ecsact_composite_id>(in_compo_id);
	auto compo_name = ecsact::meta::decl_full_name(compo_id);
	auto compo_pascal_name = ecsact_ustruct_name(compo_id);

	ctx.writef("USTRUCT(BlueprintType)\n");
	block(ctx, std::format("struct {}", compo_pascal_name), [&] {
		ctx.writef("GENERATED_BODY()\n\n");
		auto fields = ecsact::meta::get_field_ids(compo_id);

		ctx.write(std::format(
			"static {} FromEcsactComponentData(const void*);\n",
			compo_pascal_name
		));

		for(auto field_id : fields) {
			auto field_type = ecsact::meta::get_field_type(compo_id, field_id);
			auto field_unreal_type = ecsact_type_to_unreal_type(ctx, field_type);
			auto field_name = ecsact::meta::field_name(compo_id, field_id);
			auto field_pascal_name = ecsact_decl_name_to_pascal(field_name);

			print_ecsact_type_uproperty(ctx, field_type);
			ctx.write(std::format("{} {};\n", field_unreal_type, field_pascal_name));
		}
	});
	ctx.writef(";\n\n");
}

static auto generate_header(ecsact::codegen_plugin_context ctx) -> void {
	ctx.writef("#pragma once\n\n");

	inc_header(ctx, "CoreMinimal.h");
	inc_header(ctx, "UObject/Interface.h");
	inc_header(ctx, "EcsactUnreal/EcsactRunnerSubsystem.h");
	inc_package_header(ctx, ctx.package_id, ".hh");
	inc_package_header_no_ext(ctx, ctx.package_id, "__ecsact__ue.generated.h");

	auto prefix =
		ecsact_decl_name_to_pascal(ecsact::meta::package_name(ctx.package_id));

	ctx.writef("\n\n");

	for(auto comp_id : ecsact::meta::get_component_ids(ctx.package_id)) {
		print_ustruct(ctx, comp_id);
	}

	ctx.write(std::format(
		"UCLASS(Abstract, Blueprintable, meta = "
		"(DisplayName = \"Ecsact Runner Package Subsystem ({})\"))\n",
		ecsact::meta::package_name(ctx.package_id)
	));
	block(
		ctx,
		std::format(
			"class U{}EcsactRunnerSubsystem : public UEcsactRunnerSubsystem",
			prefix
		),
		[&] {
			ctx.writef("GENERATED_BODY() // NOLINT\n\n");

			ctx.write(
				"TArray<void(ThisClass::*)(int32, const void*)> InitComponentFns;\n"
				"TArray<void(ThisClass::*)(int32, const void*)> UpdateComponentFns;\n"
				"TArray<void(ThisClass::*)(int32, const void*)> RemoveComponentFns;\n"
			);

			for(auto comp_id : ecsact::meta::get_component_ids(ctx.package_id)) {
				auto comp_full_name = ecsact::meta::decl_full_name(comp_id);
				auto comp_type_cpp_name = cpp_identifier(comp_full_name);
				auto comp_name = ecsact::meta::component_name(comp_id);
				auto comp_pascal_name = ecsact_decl_name_to_pascal(comp_name);

				ctx.write(std::format(
					"void RawInit{0}(int32 Entity, const void* Component);\n",
					comp_pascal_name
				));

				ctx.write(std::format(
					"void RawUpdate{0}(int32 Entity, const void* Component);\n",
					comp_pascal_name
				));

				ctx.write(std::format(
					"void RawRemove{0}(int32 Entity, const void* Component);\n",
					comp_pascal_name
				));
			}

			ctx.indentation -= 1;
			ctx.writef("\n");
			ctx.writef("protected:");
			ctx.indentation += 1;
			ctx.writef("\n");

			ctx.write(
				"void InitComponentRaw("
				"ecsact_entity_id, ecsact_component_id, const void*) override;\n"
				"void UpdateComponentRaw("
				"ecsact_entity_id, ecsact_component_id, const void*) override;\n"
				"void RemoveComponentRaw("
				"ecsact_entity_id, ecsact_component_id, const void*) override;\n\n"
			);

			ctx.indentation -= 1;
			ctx.writef("\n");
			ctx.writef("public:");
			ctx.indentation += 1;
			ctx.writef("\n");

			ctx.write(std::format("U{}EcsactRunnerSubsystem();\n", prefix));

			for(auto comp_id : ecsact::meta::get_component_ids(ctx.package_id)) {
				auto comp_full_name = ecsact::meta::decl_full_name(comp_id);
				auto comp_type_cpp_name = cpp_identifier(comp_full_name);
				auto comp_name = ecsact::meta::component_name(comp_id);
				auto comp_pascal_name = ecsact_decl_name_to_pascal(comp_name);
				auto comp_ustruct_name = ecsact_ustruct_name(comp_id);
				ctx.write(std::format(
					"UFUNCTION(BlueprintNativeEvent, Category = \"Ecsact Runner\", "
					"meta = (DisplayName = \"Init {}\"))\n",
					comp_full_name
				));
				ctx.write(std::format(
					"void Init{0}(int32 Entity, {1} {0});\n",
					comp_pascal_name,
					comp_ustruct_name
				));
				ctx.write(std::format(
					"virtual void Init{0}_Implementation(int32 Entity, {1} {0});\n",
					comp_pascal_name,
					comp_ustruct_name
				));

				ctx.write(std::format(
					"UFUNCTION(BlueprintNativeEvent, Category = \"Ecsact Runner\", "
					"meta = (DisplayName = \"Update {}\"))\n",
					comp_full_name
				));
				ctx.write(std::format(
					"void Update{0}(int32 Entity, {1} {0});\n",
					comp_pascal_name,
					comp_ustruct_name
				));
				ctx.write(std::format(
					"virtual void Update{0}_Implementation(int32 Entity, {1} {0});\n",
					comp_pascal_name,
					comp_ustruct_name
				));

				ctx.write(std::format(
					"UFUNCTION(BlueprintNativeEvent, Category = \"Ecsact Runner\", "
					"meta = (DisplayName = \"Remove {}\"))\n",
					comp_full_name
				));
				ctx.write(std::format(
					"void Remove{0}(int32 Entity, {1} {0});\n",
					comp_pascal_name,
					comp_ustruct_name
				));
				ctx.write(std::format(
					"virtual void Remove{0}_Implementation(int32 Entity, {1} {0});\n",
					comp_pascal_name,
					comp_ustruct_name
				));
			}
		}
	);
	ctx.writef(";\n");
}

static auto generate_source(ecsact::codegen_plugin_context ctx) -> void {
	inc_package_header_no_ext(ctx, ctx.package_id, "__ecsact__ue.h");

	auto package_pascal_name =
		ecsact_decl_name_to_pascal(ecsact::meta::package_name(ctx.package_id));

	auto comp_ids = ecsact::meta::get_component_ids(ctx.package_id);
	auto largest_comp_id = 0;
	for(auto comp_id : comp_ids) {
		if(static_cast<int>(comp_id) > largest_comp_id) {
			largest_comp_id = static_cast<int>(comp_id);
		}
	}

	for(auto comp_id : comp_ids) {
		auto comp_full_name = ecsact::meta::decl_full_name(comp_id);
		auto comp_name = ecsact::meta::component_name(comp_id);
		auto comp_type_cpp_name = cpp_identifier(comp_full_name);
		auto comp_pascal_name = ecsact_decl_name_to_pascal(comp_name);
		auto comp_ustruct_name = ecsact_ustruct_name(comp_id);
		block(
			ctx,
			std::format(
				"{0} {0}::FromEcsactComponentData(const void* component_data)",
				comp_ustruct_name
			),
			[&] {
				ctx.write(std::format("auto result = {0}{{}};\n", comp_ustruct_name));

				for(auto field_id : ecsact::meta::get_field_ids(comp_id)) {
					auto field_type = ecsact::meta::get_field_type(comp_id, field_id);
					auto field_unreal_type = ecsact_type_to_unreal_type(ctx, field_type);
					auto field_name = ecsact::meta::field_name(comp_id, field_id);
					auto field_pascal_name = ecsact_decl_name_to_pascal(field_name);

					ctx.write(std::format( //
						"result.{0} = static_cast<const {1}*>(component_data)->{2};\n",
						field_pascal_name,
						comp_type_cpp_name,
						field_name
					));
				}

				ctx.write("return result;");
			}
		);
		ctx.write("\n");
	}

	block(
		ctx,
		std::format(
			"U{0}EcsactRunnerSubsystem::U{0}EcsactRunnerSubsystem()",
			package_pascal_name
		),
		[&] {
			ctx.write("InitComponentFns.Init(nullptr, ", largest_comp_id + 1, ");\n");
			ctx.write(
				"UpdateComponentFns.Init(nullptr, ",
				largest_comp_id + 1,
				");\n"
			);
			ctx.write(
				"RemoveComponentFns.Init(nullptr, ",
				largest_comp_id + 1,
				");\n"
			);

			for(auto comp_id : comp_ids) {
				auto comp_full_name = ecsact::meta::decl_full_name(comp_id);
				auto comp_name = ecsact::meta::component_name(comp_id);
				auto comp_pascal_name = ecsact_decl_name_to_pascal(comp_name);
				ctx.write(std::format(
					"InitComponentFns[{}] = &ThisClass::RawInit{};\n",
					static_cast<int>(comp_id),
					comp_pascal_name
				));
				ctx.write(std::format(
					"UpdateComponentFns[{}] = &ThisClass::RawUpdate{};\n",
					static_cast<int>(comp_id),
					comp_pascal_name
				));
				ctx.write(std::format(
					"RemoveComponentFns[{}] = &ThisClass::RawRemove{};\n",
					static_cast<int>(comp_id),
					comp_pascal_name
				));
			}
		}
	);
	ctx.write("\n\n");

	block(
		ctx,
		std::format(
			"void U{0}EcsactRunnerSubsystem::InitComponentRaw"
			"( ecsact_entity_id entity"
			", ecsact_component_id component_id"
			", const void* component_data)",
			package_pascal_name
		),
		[&] {
			ctx.write(
				"(this->*InitComponentFns[static_cast<int32>(component_id)])"
				"(static_cast<int32>(entity), component_data);"
			);
		}
	);
	ctx.writef("\n\n");

	block(
		ctx,
		std::format(
			"void U{0}EcsactRunnerSubsystem::UpdateComponentRaw"
			"( ecsact_entity_id entity"
			", ecsact_component_id component_id"
			", const void* component_data)",
			package_pascal_name
		),
		[&] {
			ctx.write(
				"(this->*UpdateComponentFns[static_cast<int32>(component_id)])"
				"(static_cast<int32>(entity), component_data);"
			);
		}
	);
	ctx.writef("\n\n");

	block(
		ctx,
		std::format(
			"void U{0}EcsactRunnerSubsystem::RemoveComponentRaw"
			"( ecsact_entity_id entity"
			", ecsact_component_id component_id"
			", const void* component_data)",
			package_pascal_name
		),
		[&] {
			ctx.write(
				"(this->*RemoveComponentFns[static_cast<int32>(component_id)])"
				"(static_cast<int32>(entity), component_data);"
			);
		}
	);
	ctx.writef("\n\n");

	for(auto comp_id : comp_ids) {
		auto comp_full_name = ecsact::meta::decl_full_name(comp_id);
		auto comp_name = ecsact::meta::component_name(comp_id);
		auto comp_pascal_name = ecsact_decl_name_to_pascal(comp_name);
		auto comp_ustruct_name = ecsact_ustruct_name(comp_id);
		auto comp_type_cpp_name = cpp_identifier(comp_full_name);

		block(
			ctx,
			std::format(
				"void U{0}EcsactRunnerSubsystem::RawInit{1}"
				"(int32 entity, const void* component)",
				package_pascal_name,
				comp_pascal_name
			),
			[&] {
				ctx.write(std::format(
					"Init{0}(entity, {1}::FromEcsactComponentData(component));",
					comp_pascal_name,
					comp_ustruct_name
				));
			}
		);
		ctx.write("\n");

		block(
			ctx,
			std::format(
				"void U{0}EcsactRunnerSubsystem::RawUpdate{1}"
				"(int32 entity, const void* component)",
				package_pascal_name,
				comp_pascal_name
			),
			[&] {
				ctx.write(std::format(
					"Update{0}(entity, {1}::FromEcsactComponentData(component));",
					comp_pascal_name,
					comp_ustruct_name
				));
			}
		);
		ctx.write("\n");

		block(
			ctx,
			std::format(
				"void U{0}EcsactRunnerSubsystem::RawRemove{1}"
				"(int32 entity, const void* component)",
				package_pascal_name,
				comp_pascal_name
			),
			[&] {
				ctx.write(std::format(
					"Remove{0}(entity, {1}::FromEcsactComponentData(component));",
					comp_pascal_name,
					comp_ustruct_name
				));
			}
		);
		ctx.write("\n");
	}

	for(auto comp_id : comp_ids) {
		auto comp_full_name = ecsact::meta::decl_full_name(comp_id);
		auto comp_name = ecsact::meta::component_name(comp_id);
		auto comp_pascal_name = ecsact_decl_name_to_pascal(comp_name);
		auto comp_ustruct_name = ecsact_ustruct_name(comp_id);

		block(
			ctx,
			std::format(
				"void U{0}EcsactRunnerSubsystem::Init{1}_Implementation"
				"(int32 Entity, {2} {1})",
				package_pascal_name,
				comp_pascal_name,
				comp_ustruct_name
			),
			[&] {

			}
		);
		ctx.writef("\n\n");

		block(
			ctx,
			std::format(
				"void U{0}EcsactRunnerSubsystem::Update{1}_Implementation"
				"(int32 Entity, {2} {1})",
				package_pascal_name,
				comp_pascal_name,
				comp_ustruct_name
			),
			[&] {

			}
		);
		ctx.writef("\n\n");

		block(
			ctx,
			std::format(
				"void U{0}EcsactRunnerSubsystem::Remove{1}_Implementation"
				"(int32 Entity, {2} {1})",
				package_pascal_name,
				comp_pascal_name,
				comp_ustruct_name
			),
			[&] {

			}
		);
		ctx.writef("\n\n");
	}
}

auto ecsact_codegen_plugin(
	ecsact_package_id          package_id,
	ecsact_codegen_write_fn_t  write_fn,
	ecsact_codegen_report_fn_t report_fn
) -> void {
	generate_header({package_id, GENERATED_HEADER_INDEX, write_fn, report_fn});
	generate_source({package_id, GENERATED_SOURCE_INDEX, write_fn, report_fn});
}
