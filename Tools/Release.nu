use Package.nu package-plugin

def main [version: string] {
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
	cd $plugin_dir;
	package-plugin;

	ls $dist_dir;
	
	git add $plugin_descriptor_filename;
	git commit -m $"chore: update version ($version)";
	git push;
	gh release create $version --generate-notes --latest -t $version Dist/*;
}
