#include <iostream>
#include <ranges>
#include <filesystem>
#ifdef __cpp_lib_execution
#	include <execution>
#endif
#include <boost/process.hpp>
#include <boost/program_options.hpp>

using namespace std::string_literals;
namespace fs = std::filesystem;
namespace bp = boost::process;
namespace po = boost::program_options;

#define SDK_PLEASE \
	" - please make sure the Ecsact SDK is installed correctly\n";

constexpr auto HELP_MSG =
	"This tool uses the Ecsact SDK to run Ecsact Unreal codegen plugin. It does"
	"this by first searching for the Ecsact Unreal plugin in your projects "
	"associated Unreal Engine install directory and then searches for all Ecsact "
	"files in your projects Source folder.\n\n"

	// clang-format off
	"Typically this tool is used in your projects .uproject file in the "
	"PreBuildSteps. Here's an example:\n\n"
	"    \"PreBuildSteps\": {\n"
	"        \"Win64\": [\n"
	"            \"EcsactUnrealCodegen $(ProjectDir) --engine-dir $(EngineDir) || exit /b 1\"\n"
	"        ]\n"
	"    }\n\n"
	// clang-format on
	"See https://ecsact.dev/start/unreal for more details\n\n";

constexpr auto platform_exec_ext() -> std::string {
#ifdef _WIN32
	return ".exe";
#else
	return "";
#endif
}

/**
 * Find ecsact executable in priority order
 *  1. the executable in the same parent directory as this one
 *  2. the executable found in PATH
 *  3. (windows only workaround) in LocalAppData/Microsoft/WindowsApps
 */
auto find_ecsact_exe(const char* argv0) -> std::optional<fs::path> {
	auto ec = std::error_code{};
	auto argv0_canon = fs::canonical(argv0, ec);
	if(!ec) {
		if(argv0_canon.has_parent_path()) {
			auto exe_neighbour =
				argv0_canon.parent_path() / ("ecsact" + platform_exec_ext());
			if(fs::exists(exe_neighbour)) {
				return exe_neighbour;
			}
		}
	}

	auto found_exe = bp::search_path("ecsact").generic_string();
	if(!found_exe.empty()) {
		return found_exe;
	}

	auto env = bp::native_environment{};
	auto user_profile = env.find("USERPROFILE");
	if(user_profile != env.end()) {
		auto windows_apps = fs::path{user_profile->to_string()} / "AppData" /
			"Local" / "Microsoft" / "WindowsApps";

		if(fs::exists(windows_apps)) {
			return windows_apps / "ecsact.exe";
		}
	}

	return std::nullopt;
}

auto proc_stdout(auto&&... args) -> std::optional<std::string> {
	auto stdout_stream = bp::ipstream{};
	auto proc = bp::child{
		args...,
		bp::std_out > stdout_stream,
		bp::std_err > stderr,
	};

	auto full_stdout = std::string{};
	auto line = std::string{};
	while(std::getline(stdout_stream, line)) {
		full_stdout += line;
	}

	proc.wait();

	if(proc.exit_code() == 0) {
		return full_stdout;
	}

	return std::nullopt;
}

auto proc_stdout_list(auto&&... args) -> std::vector<std::string> {
	auto stdout_stream = bp::ipstream{};
	auto proc = bp::child{
		args...,
		bp::std_out > stdout_stream,
		bp::std_err > stderr,
	};

	auto lines = std::vector<std::string>{};
	auto line = std::string{};
	while(std::getline(stdout_stream, line)) {
		if(line.ends_with("\r")) {
			line.pop_back();
		}
		lines.emplace_back(line);
	}

	proc.wait();

	if(proc.exit_code() == 0) {
		return lines;
	}

	return {};
}

struct plugin_dir_info {
	fs::path    plugin_path;
	std::string plugin_name;
};

auto get_plugin_dir_info( //
	fs::path plugin_dir
) -> std::optional<plugin_dir_info> {
	for(auto entry : fs::directory_iterator(plugin_dir)) {
		if(entry.path().extension() == ".uplugin") {
			return plugin_dir_info{
				.plugin_path = entry.path(),
				.plugin_name = entry.path().filename().replace_extension("").string(),
			};
		}
	}

	return std::nullopt;
}

auto is_clang_formattable(const fs::path& p) -> bool {
	auto ext = p.extension();
	return ext == ".cpp" || ext == ".c" || ext == ".h" || ext == ".cc" ||
		ext == ".hh" || ext == ".hpp";
}

auto trim_prefix(std::string_view str) -> std::string_view {
	auto index = 0;
	for(auto& c : str) {
		if(!std::isspace(c)) {
			break;
		}
		index += 1;
	}

	return str.substr(0, index);
}

auto trim_suffix(std::string_view str) -> std::string_view {
	auto trim_count = 0;
	for(auto& c : str | std::views::reverse) {
		if(!std::isspace(c)) {
			break;
		}
		trim_count += 1;
	}

	return str.substr(0, str.size() - trim_count);
}

auto trim(std::string_view str) -> std::string_view {
	return trim_prefix(trim_suffix(str));
}

auto main(int argc, char* argv[]) -> int {
	auto desc = po::options_description{};
	auto pos_desc = po::positional_options_description{};

	// clang-format off
	desc.add_options()
		("help", "show this help message")
		("format", "run clang-format on generated c/c++ files")
		("engine-dir", po::value<std::string>(), "the unreal engine directory this project uses")
		("project-path", po::value<std::string>(), "path to unreal project file or directory");
	// clang-format on

	pos_desc.add("project-path", 1);

	auto vm = po::variables_map{};
	po::store(
		po::command_line_parser{argc, argv} //
			.options(desc)
			.positional(pos_desc)
			.run(),
		vm
	);
	po::notify(vm);

	if(vm.count("help")) {
		std::cout << HELP_MSG;
		std::cout << "USAGE: EcsactUnrealCodegen [options] <project-path>\n\n";
		std::cout << "OPTIONS:\n";
		std::cout << desc << "\n";
		return 1;
	}

	if(vm.count("engine-dir") == 0) {
		std::cerr << "ERROR: --engine-dir must be specified\n";
		return 1;
	}

	auto engine_dir = fs::path{vm.at("engine-dir").as<std::string>()};
	auto project_dir = fs::path{};
	auto project_file = fs::path{};
	if(vm.count("project-path") == 0) {
		std::cout << "INFO: project path not specified - using current directory\n";
		project_dir = fs::current_path();
	} else {
		project_dir = vm.at("project-path").as<std::string>();
	}

	if(fs::is_directory(project_dir)) {
		std::cout //
			<< "INFO: project path is directory - searching for uproject file\n";
		auto uproject_files = std::vector<fs::path>{};
		for(auto entry : fs::directory_iterator(project_dir)) {
			if(entry.path().extension() == ".uproject") {
				uproject_files.push_back(entry.path());
			}
		}

		if(uproject_files.empty()) {
			std::cerr //
				<< "ERROR: could not find any .uproject files in project path '"
				<< project_dir.generic_string() << "'\n";
			return 1;
		}

		if(uproject_files.size() > 1) {
			std::cerr //
				<< "ERROR: found multiple .uproject files in project path '"
				<< project_dir.generic_string() << "'\n";
			return 1;
		}

		project_file = fs::absolute(uproject_files.at(0));
		project_dir = project_file.parent_path();
	} else {
		if(project_dir.extension() != ".uproject") {
			std::cerr //
				<< "ERROR: project path '" << project_dir.generic_string()
				<< "' is invalid\n"
				<< "ERROR: project path must be a .uproject file or directory "
					 "containing a .uproject file\n";
			return 1;
		}

		project_file = fs::absolute(project_dir);
		project_dir = project_file.parent_path();
	}

	assert(!project_dir.empty());
	assert(!project_file.empty());

	auto engine_plugins_dir = engine_dir / "Plugins" / "Marketplace";
	auto project_plugins_dir = project_dir / "Plugins";
	std::cout //
		<< "INFO: engine directory is '" << engine_dir.generic_string() << "'\n";

	auto ecsact_plugin_info = std::optional<plugin_dir_info>{};

	if(fs::exists(project_plugins_dir)) {
		for(auto entry : fs::directory_iterator{project_plugins_dir}) {
			auto info = get_plugin_dir_info(entry.path());
			if(!info) {
				continue;
			}

			if(info->plugin_name == "Ecsact") {
				ecsact_plugin_info = info;
			}
		}
	}

	if(!ecsact_plugin_info && fs::exists(engine_plugins_dir)) {
		for(auto entry : fs::directory_iterator{engine_plugins_dir}) {
			auto info = get_plugin_dir_info(entry.path());
			if(!info) {
				continue;
			}

			if(info->plugin_name == "Ecsact") {
				ecsact_plugin_info = info;
			}
		}
	}

	if(!ecsact_plugin_info) {
		std::cerr //
			<< "ERROR: unable to find the Ecsact Unreal plugin in '"
			<< engine_plugins_dir.generic_string() << "'\n"
			<< "INFO: see https://ecsact.dev/start/unreal for installation "
				 "instructions\n";
		return 1;
	}

	auto ecsact_unreal_codegen_plugin =
		ecsact_plugin_info->plugin_path.parent_path() / "Binaries" / "Win64" /
		"UnrealEditor-EcsactUnrealCodegenPlugin.dll";

	if(!fs::exists(ecsact_unreal_codegen_plugin)) {
		std::cerr //
			<< "ERROR: the Ecsact Unreal plugin does not contain the built "
				 "EcsactUnrealCodegenPlugin - please make sure you have installed the "
				 "Ecsact Unreal plugin correctly\n";
		return 1;
	}

	auto env = boost::process::native_environment{};
	auto ecsact_cli = find_ecsact_exe(argv[0]);
	if(!ecsact_cli) {
		std::cerr << "ERROR: Cannot find 'ecsact' in PATH " SDK_PLEASE;
		return -1;
	}

	std::cout << "INFO: using " << ecsact_cli->string() << "\n";

	auto version = proc_stdout( //
		bp::exe(ecsact_cli->string()),
		bp::args("--version")
	);

	if(!version) {
		std::cerr << "ERROR: failed to get ecsact version" SDK_PLEASE;
		return 1;
	}

	std::cout << "INFO: ecsact version " << trim(*version) << "\n";

	auto source_dir = project_dir / "Source";
	if(!fs::exists(source_dir)) {
		std::cout << "WARN: project 'Source' directory does not exist\n";
		std::cout << "INFO: ecsact codegen was not executed\n";
		return 0;
	}

	auto ecsact_files = std::vector<fs::path>{};
	for(auto entry : fs::recursive_directory_iterator(source_dir)) {
		if(entry.path().extension() != ".ecsact") {
			continue;
		}

		ecsact_files.push_back(entry.path());
	}

	auto ecsact_codegen_args = std::vector<std::string>{"codegen"};

	ecsact_codegen_args.emplace_back("--plugin");
	ecsact_codegen_args.emplace_back("cpp_header");

	ecsact_codegen_args.emplace_back("--plugin");
	ecsact_codegen_args.emplace_back( //
		ecsact_unreal_codegen_plugin.generic_string()
	);

	for(auto ecsact_file : ecsact_files) {
		ecsact_codegen_args.push_back(ecsact_file.generic_string());
	}

	auto codegen_proc = bp::child{
		bp::exe(ecsact_cli->string()),
		bp::args(ecsact_codegen_args),
	};

	codegen_proc.wait();

	if(auto exit_code = codegen_proc.exit_code(); exit_code != 0) {
		std::cerr //
			<< "ERROR: ecsact codegen failed with exit code " << exit_code << "\n";
		return exit_code;
	}

	if(vm.count("format")) {
		auto clang_format = bp::search_path("clang-format");

		if(clang_format.empty()) {
			std::cerr //
				<< "ERROR: --format specified but clang-format was not found on PATH\n";
			return 1;
		}

		ecsact_codegen_args.emplace_back("--print-output-files");
		auto output_files = proc_stdout_list( //
			bp::exe(ecsact_cli->string()),
			bp::args(ecsact_codegen_args)
		);

		auto formattable_output_files = std::vector<fs::path>{};
		for(fs::path output_path : output_files) {
			if(is_clang_formattable(output_path)) {
				formattable_output_files.emplace_back(output_path);
			}
		}

		if(!formattable_output_files.empty()) {
			std::cout << "INFO: formatting code generated files\n";
			std::for_each(
#ifdef __cpp_lib_execution
				std::execution::par,
#endif
				formattable_output_files.begin(),
				formattable_output_files.end(),
				[&](fs::path p) {
					auto format_proc = bp::child{
						bp::exe(clang_format),
						bp::args({"-i"s, fs::absolute(p).string()}),
						bp::start_dir(project_dir.string()),
					};

					format_proc.wait();

					auto format_exit_code = format_proc.exit_code();
					if(format_exit_code != 0) {
						std::cerr //
							<< "ERROR: failed to format '" << p.generic_string() << "'\n"
							<< "ERROR: clang-format exit code " << format_exit_code << "\n";
					}
				}
			);
		}
	}

	std::cout << "SUCCESS: ecsact codegen finished\n";

	return 0;
}
