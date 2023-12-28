#!/usr/bin/env python3
# Prototype: for evaluation on multi-machine prototype (used by exp scripts)

from ..common import *

class Prototype:
    # NOTE: MUST be the same as simulator.c and evaluator.c
    EVALUATOR_FINISH_INITIALIZATION_SYMBOL = "Evaluator initialized"
    EVALUATOR_FINISH_BENCHMARK_SYMBOL = "Evaluator done"

    def __init__(self, **kwargs):
        # For launched componenets
        self.is_cleanup_ = False
        self.is_successful_finish_ = False
        self.permachine_launched_components_ = {}

        self.cliutil_instance_ = CLIUtil(Common.scriptname, **kwargs)

    def run(self):
        physical_machines = JsonUtil.getValueForKeystr(Common.scriptname, "physical_machines")

        # (1) Launch evaluator
        ## Get launch evaluator command
        evaluator_machine_idx = JsonUtil.getValueForKeystr(Common.scriptname, "evaluator_machine_index")
        launch_evaluator_cmd = ""
        if evaluator_machine_idx == Common.cur_machine_idx:
            launch_evaluator_cmd = "nohup ./evaluator {} >tmp_evaluator.out 2>&1 &".format(cliutil_instance.getEvaluatorCLIStr())
        else:
            launch_evaluator_cmd = "ssh -i ~/.ssh/{} {}@{} \"nohup ./evaluator {} >tmp_evaluator.out 2>&1 &\"".format(Common.sshkey_name, Common.username, physical_machines[evaluator_machine_idx]["ipstr"], cliutil_instance.getEvaluatorCLIStr())
        ## Execute command and update launched components
        launch_evaluator_subprocess = SubprocessUtil.runCmd(launch_evaluator_cmd)
        if evaluator_machine_idx not in self.permachine_launched_components_:
            self.permachine_launched_components_[evaluator_machine_idx] = []
        self.permachine_launched_components_[evaluator_machine_idx].append("./evaluator")
        if launch_evaluator_subprocess.returncode != 0:
            LogUtil.dieNoExit(Common.scriptname, "failed to launch evaluator")
            is_cleanup = True
        ## Cleanup if necessary
        self.tryToCleanup_()

        # (2) Verify that evaluator finishes initialization
        ## Wait for evaluator initialization
        sleep(0.5)
        ## Verify existence of evaluator finish initialization symbol
        verify_evaluator_initialization_cmd = ""
        if evaluator_machine_idx == Common.cur_machine_idx:
            verify_evaluator_initialization_cmd = "cat tmp_evaluator.out | grep \"{}\"".format(self.EVALUATOR_FINISH_INITIALIZATION_SYMBOL)
        else:
            verify_evaluator_initialization_cmd = "ssh -i ~/.ssh/{} {}@{} \"cat tmp_evaluator.out | grep \\\"{}\\\"\"".format(Common.sshkey_name, Common.username, physical_machines[evaluator_machine_idx]["ipstr"], self.EVALUATOR_FINISH_INITIALIZATION_SYMBOL)
        verify_evaluator_initialization_subprocess = SubprocessUtil.runCmd(verify_evaluator_initialization_cmd)
        if verify_evaluator_initialization_subprocess.returncode != 0: # Error or symbol NOT exist
            if SubprocessUtil.getSubprocessErrstr(verify_evaluator_initialization_subprocess) != "": # Error
                LogUtil.dieNoExit(Common.scriptname, "failed to verify evaluator initialization; error: {}".format(SubprocessUtil.getSubprocessErrstr(verify_evaluator_initialization_subprocess)))
            else: # Symbol NOT exist
                LogUtil.dieNoExit(Common.scriptname, "evaluator has NOT finished initialization")
            is_cleanup = True
        else:
            if SubprocessUtil.getSubprocessOutputstr(verify_evaluator_initialization_subprocess) == "": # Symbol NOT exist
                LogUtil.dieNoExit(Common.scriptname, "evaluator has NOT finished initialization")
                is_cleanup = True
        ## Cleanup if necessary
        self.tryToCleanup_()

        # (3) Launch cloud
        ## TODO: END HERE
    
    def tryToCleanup_(self):
        if self.is_cleanup_:
            for tmp_machine_idx in self.permachine_launched_components_:
                for tmp_component in self.permachine_launched_components_[machine_idx]:
                    kill_component_cmd = ""
                    if tmp_machine_idx == Common.cur_machine_idx:
                        kill_component_cmd = "ps -aux | grep {} | grep -v grep | awk '{print $2}'".format(tmp_component)
                    else:
                        kill_component_cmd = "ssh -i ~/.ssh/{} {}@{} \"ps -aux | grep {} | grep -v grep | awk '{print $2}'\"".format(Common.sshkey_name, Common.username, physical_machines[tmp_machine_idx]["ipstr"], tmp_component)
                    kill_component_subprocess = SubprocessUtil.runCmd(kill_component_cmd)
                    if kill_component_subprocess.returncode != 0 and not self.is_successful_finish_:
                        LogUtil.dieNoExit(Common.scriptname, "failed to kill {}; error: {}".format(tmp_component, SubprocessUtil.getSubprocessErrstr(kill_component_subprocess)))
            if not is_successful_finish_:
                LogUtil.dieNoExit(Common.scriptname, "failed to launch prototype")
                exit(1)
            else:
                LogUtil.emphasize(Common.scriptname, "cleanup prototype successfully")


# clientcnt = 3
# edgecnt = 3
# keycnt = 100000
# capacity_mb = 1000
# cache_name = "covered"



