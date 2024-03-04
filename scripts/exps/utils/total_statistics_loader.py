#!/usr/bin/env python3
# TotalStatisticsLoader: reload statistics of previously-run exps in evaluator machine (used by exp scripts)

import time

from ...common import *
from .cliutil import *
from .exputil import *

class TotalStatisticsLoader:
    def __init__(self, total_statistics_loader_logfile = None, **kwargs):
        # For launched componenets
        self.is_successful_finish_ = False

        self.cliutil_instance_ = CLIUtil(**kwargs)
        self.total_statistics_loader_logfile_ = "tmp_total_statistics_loader.out"
        if total_statistics_loader_logfile is not None:
            self.total_statistics_loader_logfile_ = total_statistics_loader_logfile
    
    def run(self):
        # NOTE: current machine idx MUST be evaluator machine idx for statistics reloading
        evaluator_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "evaluator_machine_index")
        if evaluator_machine_idx != Common.cur_machine_idx:
            self.dieWithCleanup_("evaluator machine idx MUST be current machine idx for total statistics loader")

        # (1) Launch total statistics loader
        ## Get launch total statistics loader command
        launch_total_statistics_loader_cmd = "nohup ./total_statistics_loader {} >{} 2>&1 &".format(self.cliutil_instance_.getTotalStatisticsLoaderCLIStr(), self.total_statistics_loader_logfile_)
        ## Execute command
        launch_total_statistics_loader_subprocess = SubprocessUtil.runCmd(launch_total_statistics_loader_cmd)
        if launch_total_statistics_loader_subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch total statistics loader (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_total_statistics_loader_subprocess)))
        
        # (2) Periodically check whether total statistics loader finishes benchmark
        LogUtil.prompt(Common.scriptname, "wait for total statistics loader to finish reloading phase...")
        ## Get check total statistics loader finish reloading command
        check_total_statistics_loader_finish_reloading_cmd = "cat {} | grep '{}'".format(self.total_statistics_loader_logfile_, Common.TOTAL_STATISTICS_LOADER_FINISH_RELOADING_SYMBOL)
        while True:
            ## Periodically check
            time.sleep(1)
            ## Check existence of total statistics loader finish reloading symbol
            check_total_statistics_loader_finish_reloading_subprocess = SubprocessUtil.runCmd(check_total_statistics_loader_finish_reloading_cmd)
            if check_total_statistics_loader_finish_reloading_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_total_statistics_loader_finish_reloading_subprocess) != "": # Symbol exist
                LogUtil.emphasize(Common.scriptname, "total statistics loader has finished reloading")
                break
        
        # (3) Kill total statistics loader
        ## Wait for all launched threads which may be blocked by UDP sockets before timeout
        LogUtil.prompt(Common.scriptname, "wait for all launched threads to finish...")
        time.sleep(1)
        ## Cleanup all launched threads
        self.is_successful_finish_ = True
        self.cleanup_()
    
    def dieWithCleanup_(self, msg):
        LogUtil.dieNoExit(Common.scriptname, msg)
        self.cleanup_()
    
    def cleanup_(self):
        # Kill launched total statistics loader
        ExpUtil.killComponenet(Common.cur_machine_idx, "./total_statistics_loader")

        if not self.is_successful_finish_:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch total statistics loader")
            exit(1)
        else:
            LogUtil.emphasize(Common.scriptname, "cleanup total statistics loader successfully")