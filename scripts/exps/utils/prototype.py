#!/usr/bin/env python3
# Prototype: for evaluation on multi-machine prototype (used by exp scripts)

import time

from ...common import *
from .cliutil import *
from .exputil import *

class Prototype:
    def __init__(self, **kwargs):
        # For launched componenets
        self.is_successful_finish_ = False
        self.permachine_launched_components_ = {}

        self.cliutil_instance_ = CLIUtil(**kwargs)

    def run(self):
        physical_machines = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")

        # (1) Launch evaluator
        ## Launch evaluator in background
        evaluator_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "evaluator_machine_index")
        evaluator_component = "./evaluator"
        evaluator_logfile = "tmp_evaluator.out"
        launch_evaluator_subprocess = ExpUtil.launchComponent(evaluator_machine_idx, evaluator_component, self.cliutil_instance_.getEvaluatorCLIStr(), evaluator_logfile)
        ## Update launched components
        if evaluator_machine_idx not in self.permachine_launched_components_:
            self.permachine_launched_components_[evaluator_machine_idx] = []
        self.permachine_launched_components_[evaluator_machine_idx].append(evaluator_component)
        if launch_evaluator_subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch evaluator (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_evaluator_subprocess)))

        # (2) Verify that evaluator finishes initialization
        ## Wait for evaluator initialization
        time.sleep(0.5)
        ## Get verify evaluator finish initialization command
        verify_evaluator_initialization_cmd =  "cd {} && cat {} | grep '{}'".format(Common.proj_dirname, evaluator_logfile, Common.EVALUATOR_FINISH_INITIALIZATION_SYMBOL)
        if evaluator_machine_idx != Common.cur_machine_idx:
            verify_evaluator_initialization_cmd = ExpUtil.getRemoteCmd(evaluator_machine_idx, verify_evaluator_initialization_cmd)
        ## Verify existence of evaluator finish initialization symbol
        verify_evaluator_initialization_subprocess = SubprocessUtil.runCmd(verify_evaluator_initialization_cmd)
        if verify_evaluator_initialization_subprocess.returncode != 0: # Error or symbol NOT exist
            if SubprocessUtil.getSubprocessErrstr(verify_evaluator_initialization_subprocess) != "": # Error
                self.dieWithCleanup_("failed to verify evaluator initialization (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(verify_evaluator_initialization_subprocess)))
            else: # Symbol NOT exist
                self.dieWithCleanup_("evaluator has NOT finished initialization (please check tmp_evaluator.out in corresponding machine)")
        else:
            if SubprocessUtil.getSubprocessOutputstr(verify_evaluator_initialization_subprocess) == "": # Symbol NOT exist
                self.dieWithCleanup_("evaluator has NOT finished initialization (please check tmp_evaluator.out in corresponding machine)")

        # (3) Launch cloud
        ## Launch cloud in background
        cloud_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "cloud_machine_index")
        cloud_component = "./cloud"
        cloud_logfile = "tmp_cloud.out"
        launch_cloud_subprocess = ExpUtil.launchComponent(cloud_machine_idx, cloud_component, self.cliutil_instance_.getCloudCLIStr(), cloud_logfile)
        ## Update launched components
        if cloud_machine_idx not in self.permachine_launched_components_:
            self.permachine_launched_components_[cloud_machine_idx] = []
        self.permachine_launched_components_[cloud_machine_idx].append("./cloud")
        if launch_cloud_subprocess.returncode != 0:
            self.dieWithCleanup_("failed to launch cloud (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_cloud_subprocess)))

        # (4) Launch edges
        ## For each edge machine
        edge_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "edge_machine_indexes")
        edge_component = "./edge"
        edge_logfile = "tmp_edge.out"
        if len(edge_machine_idxes) != len(set(edge_machine_idxes)):
            self.dieWithCleanup_("duplicate edge machine indexes")
        for tmp_edge_machine_idx in edge_machine_idxes:
            ## Launch edge in background
            launch_edge_subprocess = ExpUtil.launchComponent(tmp_edge_machine_idx, edge_component, self.cliutil_instance_.getEdgeCLIStr(), edge_logfile)
            ## Update launched components
            if tmp_edge_machine_idx not in self.permachine_launched_components_:
                self.permachine_launched_components_[tmp_edge_machine_idx] = []
            self.permachine_launched_components_[tmp_edge_machine_idx].append("./edge")
            if launch_edge_subprocess.returncode != 0:
                self.dieWithCleanup_("failed to launch edge (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_edge_subprocess)))
        
        # (5) Launch clients
        ## Wait for cloud/edges to notify evaluator on finish initialization
        time.sleep(0.5)
        ## For each client machine
        client_machine_idxes = JsonUtil.getValueForKeystr(Common.scriptname, "client_machine_indexes")
        client_component = "./client"
        client_logfile = "tmp_client.out"
        if len(client_machine_idxes) != len(set(client_machine_idxes)):
            self.dieWithCleanup_("duplicate client machine indexes")
        for tmp_client_machine_idx in client_machine_idxes:
            ## Launch client in background
            launch_client_subprocess = ExpUtil.launchComponent(tmp_client_machine_idx, client_component, self.cliutil_instance_.getClientCLIStr(), client_logfile)
            ## Update launched components
            if tmp_client_machine_idx not in self.permachine_launched_components_:
                self.permachine_launched_components_[tmp_client_machine_idx] = []
            self.permachine_launched_components_[tmp_client_machine_idx].append("./client")
            if launch_client_subprocess.returncode != 0:
                self.dieWithCleanup_("failed to launch client (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(launch_client_subprocess)))
            
        # (6) Periodically check whether evaluator finishes benchmark
        LogUtil.prompt(Common.scriptname, "wait for prototype to finish benchmark...")
        ## Get check evaluator finish benchmark command
        check_evaluator_finish_benchmark_cmd = "Common.proj_dirname && cat {} | grep '{}'".format(evaluator_logfile, Common.EVALUATOR_FINISH_BENCHMARK_SYMBOL)
        if evaluator_machine_idx != Common.cur_machine_idx:
            check_evaluator_finish_benchmark_cmd = ExpUtil.getRemoteCmd(evaluator_machine_idx, check_evaluator_finish_benchmark_cmd)
        while True:
            ## Periodically check
            time.sleep(5)
            ## Check existence of evaluator finish benchmark symbol
            check_evaluator_finish_benchmark_subprocess = SubprocessUtil.runCmd(check_evaluator_finish_benchmark_cmd)
            if check_evaluator_finish_benchmark_subprocess.returncode == 0 and SubprocessUtil.getSubprocessOutputstr(check_evaluator_finish_benchmark_subprocess) != "": # Symbol exist
                LogUtil.emphasize(Common.scriptname, "evaluator has finished benchmark")
                break
        
        # (7) Kill all launched components
        ## Wait for all launched components which may be blocked by UDP sockets before timeout
        LogUtil.prompt(Common.scriptname, "wait for all launched components to finish...")
        time.sleep(5)
        ## Cleanup all launched componenets
        self.is_successful_finish_ = True
        self.cleanup_()
    
    def dieWithCleanup_(self, msg):
        LogUtil.dieNoExit(Common.scriptname, msg)
        self.cleanup_()
    
    def cleanup_(self):
        # Kill all launched components
        for tmp_machine_idx in self.permachine_launched_components_:
            for tmp_component in self.permachine_launched_components_[tmp_machine_idx]:
                ExpUtil.killComponenet(tmp_machine_idx, tmp_component)

        if not self.is_successful_finish_:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch prototype")
            exit(1)
        else:
            LogUtil.emphasize(Common.scriptname, "cleanup prototype successfully")