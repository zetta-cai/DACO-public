#!/usr/bin/env python3
# CLIUtil: build CLI parameters for main programs including simulator, client, edge, cloud, evaluator, dataset_loader, and total_statistics_loader

from ...common import *

class CLIUtil:
    # NOTE: used by C++ program cliutil -> MUST be the same as src/cliutil.c
    CLIENT_PREFIX_STRING = "Client:"
    EDGE_PREFIX_STRING = "Edge:"
    CLOUD_PREFIX_STRING = "Cloud:"
    EVALUATOR_PREFIX_STRING = "Evaluator:"
    SIMULATOR_PREFIX_STRING = "Simulator:"
    DATASET_LOADER_PREFIX_STRING = "DatasetLoader:"
    TRACE_PREPROCESSOR_PREFIX_STRING = "TracePreprocessor:"

    @staticmethod
    def getCliStrAfterPrefix_(tmp_outputstr_line, prefix_string):
        prefix_string_startidx = tmp_outputstr_line.find(prefix_string)
        if prefix_string_startidx >= 0:
            prefix_string_endidx = prefix_string_startidx + len(prefix_string)

            # Get substring of [prefix_string_endidx, prefix_string_endidx)
            return tmp_outputstr_line[prefix_string_endidx:].strip()
        else:
            return ""

    def __init__(self, **kwargs):
        # Build CLI string for cliutil program
        cliutil_cli_str = ""
        for k, v in kwargs.items():
            cliutil_cli_str += " --{} {}".format(k, v)
        cliutil_cli_str.strip() # Strip the first blank space

        # Run cliutil program
        run_cliutil_cmd = "./cliutil {}".format(cliutil_cli_str)
        run_cliutil_subprocess = SubprocessUtil.runCmd(run_cliutil_cmd)
        if run_cliutil_subprocess.returncode != 0:
            LogUtil.die(Common.scriptname, "failed to run cliutil (errmsg: {})".format(SubprocessUtil.getSubprocessErrstr(run_cliutil_subprocess)))
        run_cliutil_outputstr = SubprocessUtil.getSubprocessOutputstr(run_cliutil_subprocess)

        # Get CLI string for different components
        self.client_cli_str_ = ""
        self.edge_cli_str_ = ""
        self.cloud_cli_str_ = ""
        self.evaluator_cli_str_ = ""
        self.simulator_cli_str_ = ""
        self.dataset_loader_cli_str_ = ""
        self.trace_preprocessor_cli_str_ = ""
        for tmp_outputstr_line in run_cliutil_outputstr.splitlines():
            # For client
            if self.client_cli_str_ == "":
                self.client_cli_str_ = CLIUtil.getCliStrAfterPrefix_(tmp_outputstr_line, CLIUtil.CLIENT_PREFIX_STRING)
                if self.client_cli_str_ != "":
                    continue
            # For edge
            if self.edge_cli_str_ == "":
                self.edge_cli_str_ = CLIUtil.getCliStrAfterPrefix_(tmp_outputstr_line, CLIUtil.EDGE_PREFIX_STRING)
                if self.edge_cli_str_ != "":
                    continue
            # For cloud
            if self.cloud_cli_str_ == "":
                self.cloud_cli_str_ = CLIUtil.getCliStrAfterPrefix_(tmp_outputstr_line, CLIUtil.CLOUD_PREFIX_STRING)
                if self.cloud_cli_str_ != "":
                    continue
            # For evaluator
            if self.evaluator_cli_str_ == "":
                self.evaluator_cli_str_ = CLIUtil.getCliStrAfterPrefix_(tmp_outputstr_line, CLIUtil.EVALUATOR_PREFIX_STRING)
                if self.evaluator_cli_str_ != "":
                    continue
            # For simulator
            if self.simulator_cli_str_ == "":
                self.simulator_cli_str_ = CLIUtil.getCliStrAfterPrefix_(tmp_outputstr_line, CLIUtil.SIMULATOR_PREFIX_STRING)
                if self.simulator_cli_str_ != "":
                    continue
            # For dataset_loader
            if self.dataset_loader_cli_str_ == "":
                self.dataset_loader_cli_str_ = CLIUtil.getCliStrAfterPrefix_(tmp_outputstr_line, CLIUtil.DATASET_LOADER_PREFIX_STRING)
                if self.dataset_loader_cli_str_ != "":
                    continue
            # For trace_preprocessor
            if self.trace_preprocessor_cli_str_ == "":
                self.trace_preprocessor_cli_str_ = CLIUtil.getCliStrAfterPrefix_(tmp_outputstr_line, CLIUtil.TRACE_PREPROCESS)
                if self.trace_preprocessor_cli_str_ != "":
                    continue
    
    def getClientCLIStr(self):
        return self.client_cli_str_
    
    def getEdgeCLIStr(self):
        return self.edge_cli_str_
    
    def getCloudCLIStr(self):
        return self.cloud_cli_str_

    def getEvaluatorCLIStr(self):
        return self.evaluator_cli_str_

    def getSimulatorCLIStr(self):
        return self.simulator_cli_str_

    def getDatasetLoaderCLIStr(self):
        return self.dataset_loader_cli_str_

    def getTotalStatisticsLoaderCLIStr(self):
        return self.evaluator_cli_str_
    
    def getTracePreprocessorCLIStr(self):
        return self.trace_preprocessor_cli_str_