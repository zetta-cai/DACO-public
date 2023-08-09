# (1) Deprecated: after list_immediate_files_and_directories()

def getPreferredDirpathForTarget(infix_name, target_name, env_pathstr):
    if env_pathstr == None:
        print("ERROR: env_pathstr for {} is None!",format(target_name))
        sys.exit(1)
    else:
        print("env_pathstr for {}: {}".format(target_name, env_pathstr))

    preferred_dirpath = ""
    env_paths = env_pathstr.split(":")
    for i in range(len(env_paths)):
        tmp_env_path = env_paths[i].strip()

        # Judge whether the file or directory target_name is under tmp_env_path, if tmp_env_path exists and is a directory
        if "/usr/{}".format(infix_name) in tmp_env_path or "/usr/local/{}".format(infix_name) in tmp_env_path:
            tmp_sub_files_or_dirs = list_immediate_files_and_directories(tmp_env_path)
            for tmp_sub_file_or_dir in tmp_sub_files_or_dirs:
                if target_name in tmp_sub_file_or_dir:
                    preferred_dirpath = tmp_env_path.strip()
                    break
    
    if preferred_dirpath == "":
        print("ERROR: /usr/{0}/{1} or /usr/local/{0}/{1} are also NOT found in {2}!".format(infix_name, target_name, env_pathstr))
        sys.exit(1)
    
    return preferred_dirpath

# (2) Deprecated: after getPreferredDirpathForTarget()

# def getDefaultIncludeAndLibPathstr():
#     get_gpp_include_pathstr_cmd = "echo | g++ -E -Wp,-v -x c++ - 2>&1 | grep -v '#' | grep -v '^$' | grep -v 'search starts here:' | grep -v 'End of search list.' | grep -v 'ignoring'"
#     get_gpp_include_pathstr_subprocess = subprocess.run(get_gpp_include_pathstr_cmd, shell=True, capture_output=True)
#     if get_gpp_include_pathstr_subprocess.returncode != 0:
#         print("ERROR: get_gpp_include_pathstr_subprocess returncode is not 0!")
#         sys.exit(1)
#     else:
#         get_gpp_include_pathstr_outputbytes = get_gpp_include_pathstr_subprocess.stdout
#         get_gpp_include_pathstr_outputstr = get_gpp_include_pathstr_outputbytes.decode("utf-8")

#         gpp_include_pathstr_elements = get_gpp_include_pathstr_outputstr.split("\n")
#         for i in range(len(gpp_include_pathstr_elements)):
#             gpp_include_pathstr_elements[i] = gpp_include_pathstr_elements[i].strip()
#         gpp_include_pathstr = ":".join(gpp_include_pathstr_elements)
    
#     get_gpp_lib_pathstr_cmd = "echo | g++ -E -v -x c++ - 2>&1 | grep 'LIBRARY_PATH'"
#     get_gpp_lib_pathstr_subprocess = subprocess.run(get_gpp_lib_pathstr_cmd, shell=True, capture_output=True)
#     if get_gpp_lib_pathstr_subprocess.returncode != 0:
#         print("ERROR: get_gpp_lib_pathstr_subprocess returncode is not 0!")
#         sys.exit(1)
#     else:
#         get_gpp_lib_pathstr_outputbytes = get_gpp_lib_pathstr_subprocess.stdout
#         get_gpp_lib_pathstr_outputstr = get_gpp_lib_pathstr_outputbytes.decode("utf-8")

#         gpp_lib_pathstr = get_gpp_lib_pathstr_outputstr.split("=")[1]

#     return gpp_include_pathstr, gpp_lib_pathstr

# (3) Deprecated: after for cmake

# For libboost (set as the default search path of find_package for cmake)
#gpp_include_pathstr, gpp_lib_pathstr = getDefaultIncludeAndLibPathstr()
#boost_preferred_include_dirpath = getPreferredDirpathForTarget("include", "boost", gpp_include_pathstr)
#boost_preferred_lib_dirpath = getPreferredDirpathForTarget("lib", "boost", gpp_lib_pathstr)
boost_preferred_include_dirpath = "/usr/local/include"
boost_preferred_lib_dirpath = "/usr/local/lib"

#print("{} {}".format(boost_preferred_include_dirpath, boost_preferred_lib_dirpath))