use std/log;

# unreal requires us to have a copyright notice on all our source files
# to make this process a little easier this script simply makes sure the first few lines of each file contans said copyright

# NOTE: the line wrapping needs to be identical to how clang-format would line wrap this
let copyright = [
	"// Copyright (c) 2025 Seaube Software CORP. <https://seaube.com>",
	"//",
	"// This file is part of the Ecsact Unreal plugin.",
	"// Distributed under the MIT License. (See accompanying file LICENSE or view",
	"// online at <https://github.com/ecsact-dev/ecsact_unreal/blob/main/LICENSE>)",
	"",
];

def main [--write] {
	let clang_format = which clang-format;
	if ($clang_format | length) == 0 {
		print "Must have clang-format in PATH to use this script";
		exit 1
	}
	let clang_format = $clang_format | get 0 | get path;

	# move to root
	cd ([$env.FILE_PWD, ".."] | path join);

	glob "Source/**/*.{cpp,h,cs}"| each {|file|
		let contents = open $file --raw | lines;
		let contents_header = $contents | slice 0..(($copyright | length) - 1);
		if $contents_header != $copyright {
			if $write {
				log warning $"($file | path relative-to $env.PWD) is missing copyright header";
				let contents = $contents | prepend $copyright;
				$contents | save $file --raw -f;
				log info $"($file | path relative-to $env.PWD) copyright header was added - please review";
				^$clang_format $file -i;
			} else {
				log error $"($file | path relative-to $env.PWD) is missing copyright header";
			}
		}
	}
}
