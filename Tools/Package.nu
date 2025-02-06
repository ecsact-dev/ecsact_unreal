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

export def package-plugin [--ue-install-dir: string] {
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

	print $"using ($install_dir)";

	let engine_dir = [$install_dir, 'Engine'] | path join;
	let uat = [$engine_dir, 'Build', 'BatchFiles', $"RunUAT.(ue-tool-extension)"] | path join;
	^$uat BuildPlugin $"-Plugin=($plugin_descriptor)" $"-Package=($temp_package_dir)";

	tar -a -cf $dist_archive -C $temp_package_dir '*';
	rm -rf $temp_package_dir;

	return {
		ue_install: $install_dir,
		plugin_name: $plugin_name,
		plugin_archive: $dist_archive,
	};
}

def main [--ue-install-dir: string] {
	package-plugin --ue-install-dir $ue_install_dir
}
