#!/usr/bin/env python3
# Simulator: for evaluation on single-node prototype (used by exp scripts)

import time

from ...common import *
from .exputil import *

class Simulator:
    def __init__(self, **kwargs):
        # For launched componenets
        self.is_successful_finish_ = False

        self.cliutil_instance_ = CLIUtil(Common.scriptname, **kwargs)
    
    def run(self):
        # NOTE: client/edge/cloud/evaluator machine idx MUST be current machine idx, which will also be checked by src/common/config.c after launching simulator
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

        # (1) Launch simulator
        ## Get launch simulator command
        simulator_logfile = "tmp_simulator.out"
        launch_simulator_cmd = "nohup ./simulator {} >{} 2>&1 &".format(self.cliutil_instance_.getSimulatorCLIStr(), simulator_logfile)
        ## Execute command
        launch_simulator_subprocess = SubprocessUtil.runCmd(launch_simulator_cmd)
        if launch_simulator_subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch simulator (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_simulator_subprocess)))
        
        # (2) Periodically check whether simulator finishes benchmark
        LogUtil.prompt(Common.scriptname, "wait for simulator to finish benchmark...")
        ## Get check simulator finish benchmark command
        check_simulator_finish_benchmark_cmd = "cat {} | grep '{}'".format(simulator_logfile, Common.EVALUATOR_FINISH_BENCHMARK_SYMBOL)
        while True:
            ## Periodically check
            time.sleep(5)
            ## Check existence of simulator finish benchmark symbol
            check_simulator_finish_benchmark_subprocess = SubprocessUtil.runCmd(check_simulator_finish_benchmark_cmd)
            if check_simulator_finish_benchmark_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_simulator_finish_benchmark_subprocess) != "": # Symbol exist
                LogUtil.emphasize(Common.scriptname, "simulator has finished benchmark")
                break
        
        # (3) Kill simulator
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
        # Kill launched simulator
        ExpUtil.killComponenet_(Common.cur_machine_idx, "./simulator")

        if not self.is_successful_finish_:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch simulator")
            exit(1)
        else:
            LogUtil.emphasize(Common.scriptname, "cleanup simulator successfully")