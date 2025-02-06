use ./Package.nu package-plugin

def main [--ue-install-dir: string] {
	package-plugin --ue-install-dir $ue_install_dir
}
