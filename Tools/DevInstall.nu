use Package.nu package-plugin

def main [--ue-install-dir: string] {
	let info = package-plugin --ue-install-dir $ue_install_dir;
	let engine_plugins_dir = [$info.ue_install, "Engine", "Plugins", "Marketplace"] | path join;
	let plugin_extract_dir = [$engine_plugins_dir, $info.plugin_name] | path join;

	print $"Extracting ($info.plugin_name) to ($plugin_extract_dir)";
	mkdir $plugin_extract_dir;
	tar -xf $info.plugin_archive -C $plugin_extract_dir;
	print $"(ansi green)Unreal ($info.plugin_name) successfully installed to ($engine_plugins_dir)(ansi reset)";
}
