#!/usr/bin/env python3
# DatasetLoader: for loading phase in cloud machine (used by exp scripts)

import time

from ...common import *
from .cliutil import *
from .exputil import *

class DatasetLoader:
    def __init__(self, dataset_loader_logfile = None, **kwargs):
        # For launched componenets
        self.is_successful_finish_ = False

        self.cliutil_instance_ = CLIUtil(**kwargs)
        self.dataset_loader_logfile_ = "tmp_dataset_loader.out"
        if dataset_loader_logfile is not None:
            self.dataset_loader_logfile_ = dataset_loader_logfile
    
    def run(self):
        # NOTE: current machine idx MUST be cloud machine idx for loading phase
        cloud_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")
        if cloud_machine_idx != Common.cur_machine_idx:
            self.dieWithCleanup_("cloud machine idx MUST be current machine idx for dataset loader")

        # (1) Launch dataset loader
        ## Get launch dataset loader command
        launch_dataset_loader_cmd = "nohup ./dataset_loader {} >{} 2>&1 &".format(self.cliutil_instance_.getDatasetLoaderCLIStr(), self.dataset_loader_logfile_)
        ## Execute command
        launch_dataset_loader_subprocess = SubprocessUtil.runCmd(launch_dataset_loader_cmd)
        if launch_dataset_loader_subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch dataset loader (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_dataset_loader_subprocess)))
        
        # (2) Periodically check whether dataset loader finishes benchmark
        LogUtil.prompt(Common.scriptname, "wait for dataset loader to finish loading phase...")
        ## Get check dataset loader finish loading command
        check_dataset_loader_finish_loading_cmd = "cat {} | grep '{}'".format(self.dataset_loader_logfile_, Common.DATASET_LOADER_FINISH_LOADING_SYMBOL)
        while True:
            ## Periodically check
            time.sleep(5)
            ## Check existence of dataset loader finish loading symbol
            check_dataset_loader_finish_loading_subprocess = SubprocessUtil.runCmd(check_dataset_loader_finish_loading_cmd)
            if check_dataset_loader_finish_loading_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_dataset_loader_finish_loading_subprocess) != "": # Symbol exist
                LogUtil.emphasize(Common.scriptname, "dataset loader has finished loading")
                break
        
        # (3) Kill dataset loader
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
        # Kill launched dataset loader
        ExpUtil.killComponenet(Common.cur_machine_idx, "./dataset_loader")

        if not self.is_successful_finish_:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch dataset loader")
            exit(1)
        else:
            LogUtil.emphasize(Common.scriptname, "cleanup dataset loader successfully")