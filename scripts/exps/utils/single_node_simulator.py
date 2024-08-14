#!/usr/bin/env python3
# SingleNodeSimulator: for evaluation on single-node simulator (used by exp scripts)

import time

from ...common import *
from .cliutil import *
from .exputil import *

class SingleNodeSimulator:
    def __init__(self, single_node_logfile = None, **kwargs):
        # For launched componenets
        self.is_successful_finish_ = False

        self.cliutil_instance_ = CLIUtil(**kwargs)
        self.single_node_logfile_ = "tmp_single_node_simulator.out"
        if single_node_logfile is not None:
            self.single_node_logfile_ = single_node_logfile
    
    def run(self):
        # NOTE: NO need to check client/edge/cloud/evaluator machine idx in the current machine idx, as we do NOT launch multiple components simultaneously and ONLY focus on hit ratios

        # (1) Launch single-node simulator
        ## Get launch single-node simulator command
        launch_single_node_cmd = "nohup ./single_node_simulator {} >{} 2>&1 &".format(self.cliutil_instance_.getSingleNodeCLIStr(), self.single_node_logfile_)
        ## Execute command
        launch_single_node_subprocess = SubprocessUtil.runCmd(launch_single_node_cmd)
        if launch_single_node_subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch single-node simulator (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_single_node_subprocess)))
        
        # (2) Periodically check whether single-node simulator finishes benchmark
        LogUtil.prompt(Common.scriptname, "wait for single-node simulator to finish benchmark...")
        ## Get check single-node simulator finish benchmark command
        check_single_node_finish_benchmark_cmd = "cat {} | grep '{}'".format(self.single_node_logfile_, Common.EVALUATOR_FINISH_BENCHMARK_SYMBOL)
        while True:
            ## Periodically check
            time.sleep(5)
            ## Check existence of single-node simulator finish benchmark symbol
            check_single_node_finish_benchmark_subprocess = SubprocessUtil.runCmd(check_single_node_finish_benchmark_cmd)
            if check_single_node_finish_benchmark_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_single_node_finish_benchmark_subprocess) != "": # Symbol exist
                LogUtil.emphasize(Common.scriptname, "single-node simulator has finished benchmark")
                break
        
        # (3) Kill single-node simulator
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
        # Kill launched single-node simulator
        ExpUtil.killComponenet(Common.cur_machine_idx, "./single_node_simulator")

        if not self.is_successful_finish_:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch single-node simulator")
            exit(1)
        else:
            LogUtil.emphasize(Common.scriptname, "cleanup single-node simulator successfully")