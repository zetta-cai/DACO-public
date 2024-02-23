#!/usr/bin/env python3
# TracePreprocessor: for preprocessing phase in each client machine (used by exp scripts)

import time

from ...common import *
from .cliutil import *
from .exputil import *

class TracePreprocessor:
    def __init__(self, trace_preprocessor_logfile = None, **kwargs):
        # For launched componenets
        self.is_successful_finish_ = False

        self.cliutil_instance_ = CLIUtil(**kwargs)
        self.trace_preprocessor_logfile_ = "tmp_trace_preprocessor.out"
        if trace_preprocessor_logfile is not None:
            self.trace_preprocessor_logfile_ = trace_preprocessor_logfile
    
    def run(self):
        # NOTE: current machine idx MUST be a client machine idx for preprocessing phase
        client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
        if Common.cur_machine_idx not in client_machine_idxes:
            self.dieWithCleanup_("please run trace_preprocessor.py in each client machine!")

        # (1) Launch trace preprocessor
        ## Get launch trace preprocessor command
        launch_trace_preprocessor_cmd = "nohup ./trace_preprocessor {} >{} 2>&1 &".format(self.cliutil_instance_.getTracePreprocessorCLIStr(), self.trace_preprocessor_logfile_)
        ## Execute command
        launch_trace_preprocessor__subprocess = SubprocessUtil.runCmd(launch_trace_preprocessor_cmd)
        if launch_trace_preprocessor__subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch trace preprocessor (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_trace_preprocessor__subprocess)))
        
        # (2) Periodically check whether trace preprocessor finishes benchmark
        LogUtil.prompt(Common.scriptname, "wait for trace preprocessor to finish preprocessing phase...")
        ## Get check trace preprocessor finish preprocessing command
        check_trace_preprocessor_finish_preprocessing_cmd = "cat {} | grep '{}'".format(self.trace_preprocessor_logfile_, Common.TRACE_PREPROCESSOR_FINISH_SYMBOL)
        while True:
            ## Periodically check
            time.sleep(5)
            ## Check existence of trace preprocessor finish preprocessing symbol
            check_trace_preprocessor_finish_preprocessing_subprocess = SubprocessUtil.runCmd(check_trace_preprocessor_finish_preprocessing_cmd)
            if check_trace_preprocessor_finish_preprocessing_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_trace_preprocessor_finish_preprocessing_subprocess) != "": # Symbol exist
                LogUtil.emphasize(Common.scriptname, "trace preprocessor has finished preprocessing")
                break
        
        # (3) Kill trace preprocessor
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
        # Kill launched trace preprocessor
        ExpUtil.killComponenet(Common.cur_machine_idx, "./trace_preprocessor")

        if not self.is_successful_finish_:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch trace preprocessor")
            exit(1)
        else:
            LogUtil.emphasize(Common.scriptname, "cleanup trace preprocessor successfully")