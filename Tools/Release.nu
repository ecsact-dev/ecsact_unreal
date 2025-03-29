use Package.nu package-plugin

const ECSACT_SDK_VERSION = "0.9.0"

def fetch-third-party [] {
	let ecsact_sdk_dir = "Source/ThirdParty/EcsactSDK" | path expand;
	rm -rf $ecsact_sdk_dir;
	mkdir $ecsact_sdk_dir;
	let ecsact_sdk_zip = $"https://github.com/ecsact-dev/ecsact_sdk/releases/download/($ECSACT_SDK_VERSION)/ecsact_sdk_($ECSACT_SDK_VERSION)_windows_x64.zip";
	http get --raw $ecsact_sdk_zip | tar -x -C $ecsact_sdk_dir;

	# always use our version of EcsactUnrealCodegen incase there's a mismatch on release
	cp "Dist/EcsactUnrealCodegen-Win64.exe" ([$ecsact_sdk_dir, "bin", "EcsactUnrealCodegen.exe"] | path join);
}

def main [version: string, --dry] {
	if $dry {
		print "dry run - release will not be created";
	}
	let plugin_dir = $env.FILE_PWD | path join '..' | path expand;
	let dist_dir = [$plugin_dir, 'Dist'] | path join;
	rm -rf $dist_dir;
	mkdir $dist_dir;
	let ecsact_unreal_codegen_dir = $env.FILE_PWD | path join 'EcsactUnrealCodegen' | path expand;

	cd $plugin_dir;
	let plugin_descriptor_filename = (ls *.uplugin).0.name;
	let plugin_name = $plugin_descriptor_filename | split row ".uplugin" | get 0;

	mut plugin_descriptor = open $plugin_descriptor_filename | from json;
	$plugin_descriptor.Version += 1;
	$plugin_descriptor.VersionName = $version;
	$plugin_descriptor | to json -t 1 | save $plugin_descriptor_filename -f;

	cd $ecsact_unreal_codegen_dir;
	^bazel run //:CopyDist;
	# TODO: the unreal packaging tools read the bazel symlinks. We need to either disable them or get unreal packaging tools to ignore them
	^bazel clean;
	cd $plugin_dir;
	fetch-third-party;
	package-plugin;

	ls $dist_dir;
	
	if not $dry {
		git add $plugin_descriptor_filename;
		git commit -m $"chore: update version ($version)";
		git push;
		gh release create $version --generate-notes --latest -t $version Dist/*;
	} else {
		print "dry run - release not created";
	}
}
