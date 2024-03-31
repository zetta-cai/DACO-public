#!/usr/bin/env python3
# ExpUtil: utilities for evaluation (used by exp scripts)

from ...common import *

class ExpUtil:
    physical_machines_ = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")

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