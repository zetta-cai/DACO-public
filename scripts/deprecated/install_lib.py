# (1) Deprecated: after boost install

# Link installed libboost 1.81.0 to system path such that cachelib will use the same libboost version to compile libfolly
boost_system_include_dirpath = "{}/boost".format(boost_preferred_include_dirpath)
boost_system_version_filepath = "{}/version.hpp".format(boost_system_include_dirpath)
need_link_boost_system = False
target_boost_system_versionstr = "1_81"
prompt(filename, "check libboost version preferred by system...")
if not os.path.exists(boost_system_include_dirpath):
    need_link_boost_system = True
else:
    boost_system_checkversion_cmd = "cat {} | grep '#define BOOST_LIB_VERSION'".format(boost_system_version_filepath)
    boost_system_checkversion_subprocess = subprocess.run(boost_system_checkversion_cmd, shell=True, capture_output=True)
    if boost_system_checkversion_subprocess.returncode != 0:
        die(filename, "failed to get the version of libboost preferred by system")
    else:
        boost_system_checkversion_outputbytes = boost_system_checkversion_subprocess.stdout
        boost_system_checkversion_outputstr = boost_system_checkversion_outputbytes.decode("utf-8")
        if target_boost_system_versionstr not in boost_system_checkversion_outputstr:
            need_link_boost_system = True

if need_link_boost_system:
    backup_boost_system_include_dirpath = "{}_backup".format(boost_system_include_dirpath)
    if os.path.exists(boost_system_include_dirpath):
        prompt(filename, "backup original {} into {}...".format(boost_system_include_dirpath, backup_boost_system_include_dirpath))
        backup_original_boost_system_include_cmd = "sudo mv {} {}".format(boost_system_include_dirpath, backup_boost_system_include_dirpath)
        backup_original_boost_system_include_subprocess = subprocess.run(backup_original_boost_system_include_cmd, shell=True)
        if backup_original_boost_system_include_subprocess.returncode != 0:
            die(filename, "failed to backup original {}".format(boost_system_include_dirpath))
    
    boost_install_include_dirpath = "{}/include/boost".format(boost_install_dirpath)
    prompt(filename, "copy {} to {}...".format(boost_install_include_dirpath, boost_system_include_dirpath))
    copy_boost_system_include_cmd = "sudo cp -r {} {}".format(boost_install_include_dirpath, boost_system_include_dirpath)
    copy_boost_system_include_subprocess = subprocess.run(copy_boost_system_include_cmd, shell=True)
    if copy_boost_system_include_subprocess.returncode != 0:
        die(filename, "failed to copy {} to {}".format(boost_install_include_dirpath, boost_system_include_dirpath))
    
    backup_boost_system_lib_dirpath = "{}/boost_backup".format(boost_preferred_lib_dirpath)
    if not os.path.exists(backup_boost_system_lib_dirpath):
        prompt(filename, "create directory {}...".format(backup_boost_system_lib_dirpath))
        create_backup_boost_system_lib_dirpath_cmd = "sudo mkdir {}".format(backup_boost_system_lib_dirpath)
        create_backup_boost_system_lib_dirpath_subprocess = subprocess.run(create_backup_boost_system_lib_dirpath_cmd, shell=True)
        if create_backup_boost_system_lib_dirpath_subprocess.returncode != 0:
            die(filename, "failed to create {}".format(backup_boost_system_lib_dirpath))

    original_boost_system_libs = "{}/libboost_*".format(boost_preferred_lib_dirpath)
    prompt(filename, "backup original {} into {}...".format(original_boost_system_libs, backup_boost_system_lib_dirpath))
    backup_boost_system_lib_cmd = "sudo mv {} {}".format(original_boost_system_libs, backup_boost_system_lib_dirpath)
    backup_boost_system_lib_subprocess = subprocess.run(backup_boost_system_lib_cmd, shell=True)
    #if backup_boost_system_lib_subprocess.returncode != 0:
    #    die(filename, "failed to backup original {} into {}".format(original_boost_system_libs, backup_boost_system_lib_dirpath))
    
    boost_install_lib_dirpath = "{}/lib".format(boost_install_dirpath)
    boost_install_libs_filepath = "{}/libboost_*".format(boost_install_lib_dirpath)
    prompt(filename, "copy {} to {}...".format(boost_install_libs_filepath, boost_preferred_lib_dirpath))
    copy_boost_system_lib_cmd = "sudo cp {} {}".format(boost_install_libs_filepath, boost_preferred_lib_dirpath)
    copy_boost_system_include_subprocess = subprocess.run(copy_boost_system_lib_cmd, shell=True)
    if copy_boost_system_include_subprocess.returncode != 0:
        die(filename, "failed to copy {} to {}".format(boost_install_libs_filepath, boost_preferred_lib_dirpath))

    prompt(filename, "update OS by ldconfig")
    update_ldconfig_cmd = "sudo ldconfig"
    update_ldconfig_subprocess = subprocess.run(update_ldconfig_cmd, shell=True)
    if update_ldconfig_subprocess.returncode != 0:
        die(filename, "failed to update OS by ldconfig")
else:
    dump(filename, "libboost preferred by system is already {}".format(target_boost_system_versionstr))

if is_clear_tarball:
    warn(filename, "clear {}".format(boost_download_filepath))
    boost_clear_cmd = "cd {} && rm {}".format(lib_dirpath, boost_download_filepath)

    boost_clear_subprocess = subprocess.run(boost_clear_cmd, shell=True)
    if boost_clear_subprocess.returncode != 0:
        die(filename, "failed to clear {}".format(boost_download_filepath))