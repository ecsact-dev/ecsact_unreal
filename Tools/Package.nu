def get-ue-install-dirs [] {
	if (sys host | get name) != "Windows" {
		print "Automatic unreal detection only supported on Windows"
		print "Please specify the --ue-install-dir option"
		exit 1
	}

	let eng_reg_version_keys = ^reg.exe query 'HKLM\SOFTWARE\EpicGames\Unreal Engine' | str trim | lines;
	$eng_reg_version_keys | each {|key|
		^reg.exe query $key /v 'InstalledDirectory' | str trim | lines | get 1 | str trim | split column '    ' key type value | get value
	} | flatten
}

def get-ue-os [] {
	match (sys host | get name) {
		"Windows" => "Win64",
		"Ubuntu" => "Linux",
		_ => {
			print $"unhandled host (sys host)"
			exit 1
		}
	}
}

def ue-tool-extension [] {
	match (sys host | get name) {
		"Windows" => "bat",
		_ => "sh",
	}
}

export def package-plugin [--ue-install-dir: string, --keep-binaries] {
	let install_dirs = if $ue_install_dir != null { [$ue_install_dir] } else { get-ue-install-dirs };
	let plugin_dir = $env.FILE_PWD | path join '..' | path expand;
	let dist_dir = [$plugin_dir, 'Dist'] | path join;
	mkdir $dist_dir;
	cd $plugin_dir;
	let plugin_descriptor_filename = (ls *.uplugin).0.name;
	let plugin_name = $plugin_descriptor_filename | split row ".uplugin" | get 0;
	let dist_archive = [$plugin_dir, 'Dist', $"($plugin_name)Unreal-(get-ue-os).zip"] | path join;
	let plugin_descriptor = [$plugin_dir, $plugin_descriptor_filename] | path join;
	let temp_package_dir = mktemp -d --suffix $"($plugin_name)UnrealPluginPackage";

	if ($install_dirs | length) == 0 {
		print "Could not find Unreal Engine installation on your system";
		exit 1;
	}

	let install_dir = if ($install_dirs | length) > 1 {
		$install_dirs | input list
	} else {
		$install_dirs | get 0
	};

	let engine_plugins_dir = [$install_dir, "Engine", "Plugins", "Marketplace"] | path join;
	let ecsact_net_plugin_dir = [$engine_plugins_dir, "EcsactNet"] | path join;
	mut ecsact_net_plugin_temp_dir = "";
	mut removed_ecsact_net = false;

	print $"using ($install_dir)";

	if ($ecsact_net_plugin_dir | path exists) {
		print $"(ansi yellow)EcsactNet plugin found in ($engine_plugins_dir)(ansi reset)";
		print $"(ansi yellow)EcsactNet plugin has to be moved before packaging Ecsact(ansi reset)";
		$removed_ecsact_net = match (input "Continue?" -d "Yes" | str downcase) {
			"yes" => true,
			"y" => true,
			"true" => true,
			"ya" => true,
			"sure" => true,
			_ => false,
		};
		if not $removed_ecsact_net {
			print "User aborted";
			exit 1;
		}
		$ecsact_net_plugin_temp_dir = mktemp -d --suffix "EcsactNetUnrealPluginMove";
		print $"(ansi yellow)Moving ($ecsact_net_plugin_dir) to ($ecsact_net_plugin_temp_dir) (ansi reset)";
		mv $ecsact_net_plugin_dir $ecsact_net_plugin_temp_dir;
	}

	let engine_dir = [$install_dir, 'Engine'] | path join;
	let uat = [$engine_dir, 'Build', 'BatchFiles', $"RunUAT.(ue-tool-extension)"] | path join;
	do { ^$uat BuildPlugin $"-Plugin=($plugin_descriptor)" $"-Package=($temp_package_dir)" }
	let uat_exit_code = $env.LAST_EXIT_CODE;
	
	if $uat_exit_code == 0 {
		print $"(ansi cyan)Creating archive ($dist_archive) from ($temp_package_dir)(ansi reset)";
		if $keep_binaries {
			tar -a -cf $dist_archive -C $temp_package_dir --exclude "Tools" '*';
		} else {
			tar -a -cf $dist_archive -C $temp_package_dir --exclude "Binaries" --exclude "Intermediate" --exclude "Tools" '*';
		}
		rm -rf $temp_package_dir;
	}

	if $removed_ecsact_net {
		print $"(ansi yellow)Bringing back ($ecsact_net_plugin_dir)(ansi reset)";
		mkdir $ecsact_net_plugin_dir;
		mv ([$ecsact_net_plugin_temp_dir, 'EcsactNet'] | path join) $engine_plugins_dir;
	}


	if $uat_exit_code == 0 {
		return {
			ue_install: $install_dir,
			plugin_name: $plugin_name,
			plugin_archive: $dist_archive,
		};
	}

	error make {msg: $"UAT failed with exit code ($uat_exit_code)"}
}

def main [--ue-install-dir: string] {
	package-plugin --ue-install-dir $ue_install_dir
}
