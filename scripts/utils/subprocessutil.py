#!/usr/bin/env python3

import subprocess

# Subprocess-related variables and functions

def runCmd(cmdstr):
    print("[shell] {}".format(cmdstr))
    tmp_subprocess = subprocess.run(cmdstr, shell=True, capture_output=True)
    return tmp_subprocess

def getSubprocessErrstr(tmp_subprocess):
    tmp_subprocess_errstr = ""
    if tmp_subprocess.stderr != None:
        tmp_subprocess_errbytes = tmp_subprocess.stderr
        tmp_subprocess_errstr = tmp_subprocess_errbytes.decode("utf-8")
    return tmp_subprocess_errstr

def getSubprocessOutputstr(tmp_subprocess):
    tmp_subprocess_outputstr = ""
    if tmp_subprocess.stdout != None:
        tmp_subprocess_outputbytes = tmp_subprocess.stdout
        tmp_subprocess_outputstr = tmp_subprocess_outputbytes.decode("utf-8")
    return tmp_subprocess_outputstr

def cloneRepo(clone_dirpath, repo_name, repo_url):
    if not os.path.exists(clone_dirpath):
        tmp_parent_dirpath = os.path.dirname(clone_dirpath)
        tmp_repo_dirname = os.path.basename(clone_dirpath)

        prompt(filename, "clone {} into {}...".format(repo_name, clone_dirpath))
        clone_cmd = "cd {} && git clone {} {}".format(tmp_parent_dirpath, repo_url, tmp_repo_dirname)

        clone_subprocess = runCmd(clone_cmd)
        if clone_subprocess.returncode != 0:
            clone_errstr = getSubprocessErrstr(clone_subprocess)
            die(filename, "failed to clone {} into {}; error: {}".format(repo_name, clone_dirpath, clone_errstr))
    else:
        dump(filename, "{} exists ({} has been cloned)".format(cclone_dirpath, repo_name))