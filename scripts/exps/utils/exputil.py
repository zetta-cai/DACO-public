#!/usr/bin/env python3
# ExpUtil: utilities for evaluation (used by exp scripts)

from ...common import *

class ExpUtil:
    physical_machines_ = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")

    @classmethod
    def getRemoteCmd_(cls, remote_machine_idx, internal_cmd):
        remote_cmd = ""
        if remote_machine_idx == Common.cur_machine_idx:
            LogUtil.dieNoExit(Common.scriptname, "remote machine index {} should != current machine index {} in getRemoteCmd_".format(remote_machine_idx, Common.cur_machine_idx))
        else:
            remote_cmd = "ssh -i ~/.ssh/{} {}@{} \"cd {} && {}\"".format(Common.sshkey_name, Common.username, cls.physical_machines_[remote_machine_idx]["ipstr"], Common.proj_dirname, internal_cmd)
        return remote_cmd
    
    @classmethod
    def killComponenet_(cls, tmp_machine_idx, tmp_component):
        # Check PID(s) of the given component
        tmp_pidstr_list = []
        check_component_cmd = "ps -aux | grep {} | grep -v grep | awk '{{print $2}}'".format(tmp_component) # Doubling braces to escape them
        if tmp_machine_idx != Common.cur_machine_idx:
            check_component_cmd = cls.getRemoteCmd_(tmp_machine_idx, check_component_cmd)
        check_component_subprocess = SubprocessUtil.runCmd(check_component_cmd)
        if check_component_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_component_subprocess) != "": # PID exist
            tmp_pidstr_list = SubprocessUtil.getSubprocessOutputstr(check_component_subprocess).split()

        # Kill the given component if exist
        if len(tmp_pidstr_list) > 0:
            for tmp_pidstr in tmp_pidstr_list:
                kill_component_cmd = "kill -9 {}".format(tmp_pidstr)
                if tmp_machine_idx != Common.cur_machine_idx:
                    kill_component_cmd = cls.getRemoteCmd_(tmp_machine_idx, kill_component_cmd)
                kill_component_subprocess = SubprocessUtil.runCmd(kill_component_cmd)
                if kill_component_subprocess.returncode != 0:
                    LogUtil.dieNoExit(Common.scriptname, "failed to kill {} (errmsg: {})".format(tmp_component, SubprocessUtil.getSubprocessErrstr(kill_component_subprocess)))
        
        return