#!/usr/bin/env python3
# SingleNodePrototype: for evaluation on single-node prototype (used by exp scripts)

import time

from ...common import *
from .cliutil import *
from .exputil import *

class SingleNodePrototype:
    def __init__(self, single_node_logfile = None, **kwargs):
        # For launched componenets
        self.is_successful_finish_ = False

        self.cliutil_instance_ = CLIUtil(**kwargs)
        self.single_node_logfile_ = "tmp_single_node_protoype.out"
        if single_node_logfile is not None:
            self.single_node_logfile_ = single_node_logfile
    
    def run(self):
        # NOTE: client/edge/cloud/evaluator machine idx MUST be current machine idx, which will also be checked by src/common/config.c after launching single-node prototype
        evaluator_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "evaluator_machine_index")
        if evaluator_machine_idx != Common.cur_machine_idx:
            self.dieWithCleanup_("evaluator machine idx MUST be current machine idx for single-node simulation")
        cloud_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")
        if cloud_machine_idx != Common.cur_machine_idx:
            self.dieWithCleanup_("cloud machine idx MUST be current machine idx for single-node simulation")
        edge_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "edge_machine_indexes")
        if edge_machine_idxes != [Common.cur_machine_idx]:
            self.dieWithCleanup_("edge machine idxes MUST be current machine idx for single-node simulation")
        client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
        if client_machine_idxes != [Common.cur_machine_idx]:
            self.dieWithCleanup_("client machine idxes MUST be current machine idx for single-node simulation")

        # (1) Launch single-node prototype
        ## Get launch single-node prototype command
        launch_single_node_cmd = "nohup ./single_node_prototype {} >{} 2>&1 &".format(self.cliutil_instance_.getSingleNodeCLIStr(), self.single_node_logfile_)
        ## Execute command
        launch_single_node_subprocess = SubprocessUtil.runCmd(launch_single_node_cmd)
        if launch_single_node_subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch single-node prototype (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_single_node_subprocess)))
        
        # (2) Periodically check whether single-node prototype finishes benchmark
        LogUtil.prompt(Common.scriptname, "wait for single-node prototype to finish benchmark...")
        ## Get check single-node prototype finish benchmark command
        check_single_node_finish_benchmark_cmd = "cat {} | grep '{}'".format(self.single_node_logfile_, Common.EVALUATOR_FINISH_BENCHMARK_SYMBOL)
        while True:
            ## Periodically check
            time.sleep(5)
            ## Check existence of single-node prototype finish benchmark symbol
            check_single_node_finish_benchmark_subprocess = SubprocessUtil.runCmd(check_single_node_finish_benchmark_cmd)
            if check_single_node_finish_benchmark_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_single_node_finish_benchmark_subprocess) != "": # Symbol exist
                LogUtil.emphasize(Common.scriptname, "single-node prototype has finished benchmark")
                break
        
        # (3) Kill single-node prototype
        ## Wait for all launched threads which may be blocked by UDP sockets before timeout
        LogUtil.prompt(Common.scriptname, "wait for all launched threads to finish...")
        time.sleep(5)
        ## Cleanup all launched threads
        self.is_successful_finish_ = True
        self.cleanup_()
    
    def dieWithCleanup_(self, msg):
        LogUtil.dieNoExit(Common.scriptname, msg)
        self.cleanup_()
    
    def cleanup_(self):
        # Kill launched single-node prototype
        ExpUtil.killComponenet(Common.cur_machine_idx, "./single_node_prototype")

        if not self.is_successful_finish_:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch single-node prototype")
            exit(1)
        else:
            LogUtil.emphasize(Common.scriptname, "cleanup single-node prototype successfully")