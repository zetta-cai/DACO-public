# Deprecated: python 3.7.5 only works for Ubuntu 20.04 -> we just need python3 (e.g., python 3.5.2 for Ubuntu 16.04 and python 3.6.9 for Ubuntu 18.04)

# (0) Install some pre-requisites

LogUtil.prompt(Common.scriptname, "install pre-requisites...")
install_prerequisite_cmd = "sudo apt-get -y install software-properties-common python3-pip"
install_prerequisite_subprocess = SubprocessUtil.runCmd(install_prerequisite_cmd)
if install_prerequisite_subprocess.returncode != 0:
    LogUtil.die(Common.scriptname, "failed to install pre-requisites (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(install_prerequisite_subprocess)))

# (1) Upgrade python3 if necessary

# For python3
python3_preferred_binpath = PathUtil.getPreferredDirpathForTarget(Common.scriptname, "python3") # e.g., /usr/local/bin
python3_installpath = os.path.dirname(python3_preferred_binpath) # e.g., /usr/local
python3_install_binpath = "{}/bin".format(python3_installpath) # e.g., /usr/local/bin

if is_upgrade_python3:
    python3_software_name = "python3"
    python3_target_version = "3.7.5"
    python3_checkversion_cmd = "python3 --version"
    need_upgrade_python3, python3_old_version = SubprocessUtil.checkVersion(Common.scriptname, python3_software_name, python3_target_version, python3_checkversion_cmd)

    if need_upgrade_python3:
        python3_old_canonical_filepath = SubprocessUtil.getCanonicalFilepath(Common.scriptname, python3_software_name, python3_old_version)
        need_preserve_old_python3 = SubprocessUtil.checkOldAlternative(Common.scriptname, python3_software_name, python3_old_canonical_filepath)

        if need_preserve_old_python3:
            SubprocessUtil.preserveOldAlternative(Common.scriptname, python3_software_name, python3_old_canonical_filepath, python3_preferred_binpath)
        else:
            LogUtil.dump(Common.scriptname, "old python3 has been preserved")

        python3_download_filepath = "{}/Python-3.7.5.tgz".format(Common.lib_dirpath)
        python3_download_url = "https://www.python.org/ftp/python/3.7.5/Python-3.7.5.tgz"
        SubprocessUtil.downloadTarball(Common.scriptname, python3_download_filepath, python3_download_url)

        python3_decompress_dirpath = "{}/Python-3.7.5".format(Common.lib_dirpath)
        python3_decompress_tool = "tar -xvf"
        SubprocessUtil.decompressTarball(Common.scriptname, python3_download_filepath, python3_decompress_dirpath, python3_decompress_tool)

        python3_install_filepath = "{}/python3.7".format(python3_install_binpath)
        python3_install_tool = "./configure --enable-optimizations --prefix={0} --exec_prefix={0} && sudo make altinstall".format(python3_installpath)
        SubprocessUtil.installDecompressedTarball(Common.scriptname, python3_decompress_dirpath, python3_install_filepath, python3_install_tool, time_consuming = True)

        SubprocessUtil.preserveNewAlternative(Common.scriptname, python3_software_name, python3_preferred_binpath, python3_install_filepath)

        if is_clear_tarball:
            SubprocessUtil.clearTarball(Common.scriptname, python3_download_filepath)
    print("")