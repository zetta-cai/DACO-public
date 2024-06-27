#!/usr/bin/env python3
# ExpUtil: utilities for evaluation (used by exp scripts)

from ...common import *

class ExpUtil:
    physical_machines_ = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")
    cloud_machine_idx_ = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")

    @classmethod
    def getRemoteCmd(cls, remote_machine_idx, internal_cmd, with_background = False, specify_sshkey = True):
        remote_cmd = ""
        if remote_machine_idx == Common.cur_machine_idx:
            LogUtil.dieNoExit(Common.scriptname, "remote machine index {} should != current machine index {} in getRemoteCmd".format(remote_machine_idx, Common.cur_machine_idx))
        else:
            ssh_cmd = ""
            if specify_sshkey:
                ssh_cmd = "ssh -i {}".format(Common.sshkey_filepath)
            else:
                ssh_cmd = "ssh"
            if with_background:
                # NOTE: use -f to move SSH connection into background in target machine after launching the background command, such that current machine will NOT get stuck
                ssh_cmd = "{} -f {}@{}".format(ssh_cmd, Common.username, cls.physical_machines_[remote_machine_idx]["public_ipstr"])
            else:
                ssh_cmd = "{} {}@{}".format(ssh_cmd, Common.username, cls.physical_machines_[remote_machine_idx]["public_ipstr"])
            remote_cmd = "{} \"source /home/{}/.bashrc_non_interactive && {}\"".format(ssh_cmd, Common.username, internal_cmd)
        return remote_cmd

    @classmethod
    def launchComponent(cls, tmp_machine_idx, tmp_component, tmp_component_clistr, tmp_component_logfile):
        # Get launch component command
        launch_component_cmd = "cd {} && nohup {} {} >{} 2>&1 &".format(Common.proj_dirname, tmp_component, tmp_component_clistr, tmp_component_logfile)
        if tmp_machine_idx != Common.cur_machine_idx:
            # NOTE: set with_background = True to avoid getting stuck by background command
            launch_component_cmd = cls.getRemoteCmd(tmp_machine_idx, launch_component_cmd, with_background = True)
        
        # Execute command to launch component
        # NOTE: set is_capture_output = False to avoid getting stuck by background command
        launch_component_subprocess = SubprocessUtil.runCmd(launch_component_cmd, is_capture_output = False)

        return launch_component_subprocess
    
    @classmethod
    def killComponenet(cls, tmp_machine_idx, tmp_component):
        # Check PID(s) of the given component
        tmp_pidstr_list = []
        
        check_component_cmd = "ps -aux | grep -F {} | grep -v grep | grep {}".format(tmp_component, Common.username)
        if tmp_machine_idx != Common.cur_machine_idx:
            # NOTE: use double braces to escape braces from python string format processing
            # NOTE: use \$ to escape $ from ssh command, otherwise $2 will be interpreted as a variable by bash due to using double quotes in getRemoteCmd()
            check_component_cmd = "{} | awk '{{print \$2}}'".format(check_component_cmd)
            check_component_cmd = cls.getRemoteCmd(tmp_machine_idx, check_component_cmd)
        else:
            # NOTE: use double braces to escape braces from python string format processing
            check_component_cmd = "{} | awk '{{print $2}}'".format(check_component_cmd)
        check_component_subprocess = SubprocessUtil.runCmd(check_component_cmd)
        if check_component_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_component_subprocess) != "": # PID exist
            tmp_pidstr_list = SubprocessUtil.getSubprocessOutputstr(check_component_subprocess).split()

        # Kill the given component if exist
        if len(tmp_pidstr_list) > 0:
            for tmp_pidstr in tmp_pidstr_list:
                # Check if tmp_pidstr is numberic
                if not tmp_pidstr.isnumeric():
                    LogUtil.die(Common.scriptname, "pidstr {} is not numeric for component {}; outputstr of command {} is {}".format(tmp_pidstr, tmp_component, check_component_cmd, SubprocessUtil.getSubprocessOutputstr(check_component_subprocess)))

                kill_component_cmd = "kill -9 {}".format(tmp_pidstr)
                if tmp_machine_idx != Common.cur_machine_idx:
                    kill_component_cmd = cls.getRemoteCmd(tmp_machine_idx, kill_component_cmd)
                kill_component_subprocess = SubprocessUtil.runCmd(kill_component_cmd)
                if kill_component_subprocess.returncode != 0:
                    LogUtil.dieNoExit(Common.scriptname, "failed to kill {} (errmsg: {})".format(tmp_component, SubprocessUtil.getSubprocessErrstr(kill_component_subprocess)))
        
        return
    
    @classmethod
    def tryToCopyFromCurrentMachineToCloud(cls, given_filepath):
        # Copy given file to cloud machine if not exist
        if cls.cloud_machine_idx_ == Common.cur_machine_idx: # NOTE: clients and cloud may co-locate in the same physical machine
            # NOTE: NO need to check and copy, as current physical machine MUST have the given file
            continue
        else: # Remote cloud machine
            # Check if cloud machine has the given file
            # NOTE: cloud machine may already have the given file (the given file has been copied before or generated before, e.g., cloud machine is one of client machines yet not the first client to generate the given file)
            LogUtil.prompt(Common.scriptname, "check if the given file {} exists in cloud machine...".format(given_filepath))
            check_cloud_given_filepath_remote_cmd = ExpUtil.getRemoteCmd(cloud_machine_idx, "ls {}".format(given_filepath))
            need_copy_given_file = False
            check_cloud_given_filepath_subprocess = SubprocessUtil.runCmd(check_cloud_given_filepath_remote_cmd)
            if check_cloud_given_filepath_subprocess.returncode != 0: # given file not found in cloud
                need_copy_given_file = True
            # elif SubprocessUtil.getSubprocessOutputstr(check_cloud_given_filepath_subprocess) == "": # cloud given file not found (OBSOLETE: existing empty directory could return empty string)
            #     need_copy_given_file = True
            else: # given file is found in cloud
                need_copy_given_file = False

            # If cloud machine does NOT have the given file, copy it to cloud machine
            if need_copy_given_file:
                # Mkdir for the given filepath if not exist
                given_dirpath = os.path.dirname(given_filepath)
                LogUtil.prompt(Common.scriptname, "create directory {} if not exist in cloud machine...".format(given_dirpath))
                try_to_create_given_dirpath_remote_cmd = ExpUtil.getRemoteCmd(cloud_machine_idx, "mkdir -p {}".format(given_dirpath))
                try_to_create_given_dirpath_subprocess = SubprocessUtil.runCmd(try_to_create_given_dirpath_remote_cmd)
                if try_to_create_given_dirpath_subprocess.returncode != 0:
                    LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(try_to_create_given_dirpath_subprocess))

                # Copy the given file to cloud machine
                LogUtil.prompt(Common.scriptname, "copy given file {} to cloud machine...".format(given_filepath))
                cloud_machine_public_ip = physical_machines[cloud_machine_idx]["public_ipstr"]
                copy_given_file_remote_cmd = "scp -i {0} {1} {2}@{3}:{1}".format(Common.sshkey_filepath, given_filepath, Common.username, cloud_machine_public_ip)
                copy_given_file_subprocess = SubprocessUtil.runCmd(copy_given_file_remote_cmd, is_capture_output=False) # Copy given file may be time-consuming
                if copy_given_file_subprocess.returncode != 0:
                    LogUtil.die(Common.scriptname, SubprocessUtil.getSubprocessErrstr(copy_given_file_subprocess))
        return