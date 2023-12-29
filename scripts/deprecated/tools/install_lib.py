# (1) Deprecated: after boost install

# Link installed libboost 1.81.0 to system path such that cachelib will use the same libboost version to compile libfolly
boost_system_include_dirpath = "{}/boost".format(boost_preferred_include_dirpath)
boost_system_version_filepath = "{}/version.hpp".format(boost_system_include_dirpath)
need_link_boost_system = False
target_boost_system_versionstr = "1_81"
LogUtil.prompt(filename, "check libboost version preferred by system...")
if not os.path.exists(boost_system_include_dirpath):
    need_link_boost_system = True
else:
    boost_system_checkversion_cmd = "cat {} | grep '#define BOOST_LIB_VERSION'".format(boost_system_version_filepath)
    boost_system_checkversion_subprocess = subprocess.run(boost_system_checkversion_cmd, shell=True, capture_output=True)
    if boost_system_checkversion_subprocess.returncode != 0:
        LogUtil.die(filename, "failed to get the version of libboost preferred by system (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(boost_system_checkversion_subprocess)))
    else:
        boost_system_checkversion_outputbytes = boost_system_checkversion_subprocess.stdout
        boost_system_checkversion_outputstr = boost_system_checkversion_outputbytes.decode("utf-8")
        if target_boost_system_versionstr not in boost_system_checkversion_outputstr:
            need_link_boost_system = True

if need_link_boost_system:
    backup_boost_system_include_dirpath = "{}_backup".format(boost_system_include_dirpath)
    if os.path.exists(boost_system_include_dirpath):
        LogUtil.prompt(filename, "backup original {} into {}...".format(boost_system_include_dirpath, backup_boost_system_include_dirpath))
        backup_original_boost_system_include_cmd = "sudo mv {} {}".format(boost_system_include_dirpath, backup_boost_system_include_dirpath)
        backup_original_boost_system_include_subprocess = subprocess.run(backup_original_boost_system_include_cmd, shell=True)
        if backup_original_boost_system_include_subprocess.returncode != 0:
            LogUtil.die(filename, "failed to backup original {} (errmsg: {})".format(boost_system_include_dirpath, SubprocessUtil.getSubprocessErrstr(backup_original_boost_system_include_subprocess)))
    
    boost_install_include_dirpath = "{}/include/boost".format(boost_install_dirpath)
    LogUtil.prompt(filename, "copy {} to {}...".format(boost_install_include_dirpath, boost_system_include_dirpath))
    copy_boost_system_include_cmd = "sudo cp -r {} {}".format(boost_install_include_dirpath, boost_system_include_dirpath)
    copy_boost_system_include_subprocess = subprocess.run(copy_boost_system_include_cmd, shell=True)
    if copy_boost_system_include_subprocess.returncode != 0:
        LogUtil.die(filename, "failed to copy {} to {} (errmsg: {})".format(boost_install_include_dirpath, boost_system_include_dirpath, SubprocessUtil.getSubprocessErrstr(copy_boost_system_include_subprocess)))
    
    backup_boost_system_lib_dirpath = "{}/boost_backup".format(boost_preferred_lib_dirpath)
    if not os.path.exists(backup_boost_system_lib_dirpath):
        LogUtil.prompt(filename, "create directory {}...".format(backup_boost_system_lib_dirpath))
        create_backup_boost_system_lib_dirpath_cmd = "sudo mkdir {}".format(backup_boost_system_lib_dirpath)
        create_backup_boost_system_lib_dirpath_subprocess = subprocess.run(create_backup_boost_system_lib_dirpath_cmd, shell=True)
        if create_backup_boost_system_lib_dirpath_subprocess.returncode != 0:
            LogUtil.die(filename, "failed to create {} (errmsg: {})".format(backup_boost_system_lib_dirpath, SubprocessUtil.getSubprocessErrstr(create_backup_boost_system_lib_dirpath_subprocess)))

    original_boost_system_libs = "{}/libboost_*".format(boost_preferred_lib_dirpath)
    LogUtil.prompt(filename, "backup original {} into {}...".format(original_boost_system_libs, backup_boost_system_lib_dirpath))
    backup_boost_system_lib_cmd = "sudo mv {} {}".format(original_boost_system_libs, backup_boost_system_lib_dirpath)
    backup_boost_system_lib_subprocess = subprocess.run(backup_boost_system_lib_cmd, shell=True)
    #if backup_boost_system_lib_subprocess.returncode != 0:
    #    LogUtil.die(filename, "failed to backup original {} into {} (errmsg: {})".format(original_boost_system_libs, backup_boost_system_lib_dirpath, SubprocessUtil.getSubprocessErrstr(backup_boost_system_lib_subprocess)))
    
    boost_install_lib_dirpath = "{}/lib".format(boost_install_dirpath)
    boost_install_libs_filepath = "{}/libboost_*".format(boost_install_lib_dirpath)
    LogUtil.prompt(filename, "copy {} to {}...".format(boost_install_libs_filepath, boost_preferred_lib_dirpath))
    copy_boost_system_lib_cmd = "sudo cp {} {}".format(boost_install_libs_filepath, boost_preferred_lib_dirpath)
    copy_boost_system_include_subprocess = subprocess.run(copy_boost_system_lib_cmd, shell=True)
    if copy_boost_system_include_subprocess.returncode != 0:
        LogUtil.die(filename, "failed to copy {} to {} (errmsg: {})".format(boost_install_libs_filepath, boost_preferred_lib_dirpath, SubprocessUtil.getSubprocessErrstr(copy_boost_system_include_subprocess)))

    LogUtil.prompt(filename, "update OS by ldconfig")
    update_ldconfig_cmd = "sudo ldconfig"
    update_ldconfig_subprocess = subprocess.run(update_ldconfig_cmd, shell=True)
    if update_ldconfig_subprocess.returncode != 0:
        LogUtil.die(filename, "failed to update OS by ldconfig (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(update_ldconfig_subprocess)))
else:
    LogUtil.dump(filename, "libboost preferred by system is already {}".format(target_boost_system_versionstr))

if is_clear_tarball:
    LogUtil.warn(filename, "clear {}".format(boost_download_filepath))
    boost_clear_cmd = "cd {} && rm {}".format(Common.lib_dirpath, boost_download_filepath)

    boost_clear_subprocess = subprocess.run(boost_clear_cmd, shell=True)
    if boost_clear_subprocess.returncode != 0:
        LogUtil.die(filename, "failed to clear {} (errmsg: {})".format(boost_download_filepath, SubprocessUtil.getSubprocessErrstr(boost_clear_subprocess)))

# (2) Before CacheLib's installFromRepo

# Install liburing-dev 2.3 for Ubuntu >= 22.04 (NO need for previous Ubuntu due to NO liburing and hence NOT used by libfolly in cachelib)
## Check if we need liburing-dev
cachelib_pre_install_tool = None
need_liburing = False
check_need_liburing_cmd = "dpkg -l | grep liburing-dev"
check_need_liburing_subprocess = SubprocessUtil.runCmd(check_need_liburing_cmd)
if check_need_liburing_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_need_liburing_subprocess) != "":
    need_liburing = True
if need_liburing: ## Check if we need to install liburing-dev 2.3 if need liburing
    need_upgrade_liburing, liburing_old_version = SubprocessUtil.checkVersion(Common.scriptname, "liburing-dev", "2.3", "dpkg -s liburing-dev | grep ^Version: | sed -n 's/^Version: \([0-9\.]\+\)-.*/\\1/p'")
    if need_upgrade_liburing:
        # Download liburing-dev 2.3
        liburing_decompress_dirpath = "{}/liburing".format(Common.lib_dirpath)
        liburing_download_filepath = "{}/liburing-dev_2.3-3_amd64.deb".format(liburing_decompress_dirpath)
        liburing_download_url = "http://archive.ubuntu.com/ubuntu/pool/main/libu/liburing/liburing-dev_2.3-3_amd64.deb"
        SubprocessUtil.downloadTarball(Common.scriptname, liburing_download_filepath, liburing_download_url)

        # Install liburing-dev 2.3 by dpkg
        liburing_install_filepath = "" # Always install
        SubprocessUtil.installDecompressedTarball(Common.scriptname, liburing_decompress_dirpath, liburing_install_filepath, "sudo dpkg -i liburing-dev_2.3-3_amd64.deb", time_consuming = True)