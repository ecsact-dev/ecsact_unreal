def get-ue-install-dirs [] {
	let eng_reg_version_keys = ^reg.exe query 'HKLM\SOFTWARE\EpicGames\Unreal Engine' | str trim | lines;
	$eng_reg_version_keys | each {|key|
		^reg.exe query $key /v 'InstalledDirectory' | str trim | lines | get 1 | str trim | split column '    ' key type value | get value
	} | flatten
}

def get-ue-os [] {
	match (sys host | get name) {
		"Windows" => "Win64",
		_ => {
			print $"unhandled host (sys host)"
			exit 1
		}
	}
}

def main [] {
	let install_dirs = get-ue-install-dirs;
	let plugin_dir = $env.FILE_PWD | path join '..' | path expand;
	let dist_dir = [$plugin_dir, 'Dist'] | path join;
	mkdir $dist_dir;
	let dist_archive = [$plugin_dir, 'Dist', $"EcsactUnreal-(get-ue-os).zip"] | path join;
	cd $plugin_dir;
	let plugin_descriptor = [$plugin_dir, (ls *.uplugin).0.name] | path join;
	let temp_package_dir = mktemp -d --suffix 'EcsactUnrealPluginPackage';

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


	let uat = [$engine_dir, 'Build', 'BatchFiles', 'RunUAT.bat'] | path join;
	^$uat BuildPlugin $"-Plugin=($plugin_descriptor)" $"-Package=($temp_package_dir)";

	tar -a -cf $dist_archive -C $temp_package_dir '*';
	rm -rf $temp_package_dir;
}
