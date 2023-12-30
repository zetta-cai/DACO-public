#!/usr/bin/env python3
# CLIUtil: build CLI parameters for main programs including simulator, client, edge, cloud, and evaluator

from .logutil import *

class CLIUtil:
    # CLI base
    DEFAULT_CLIENTCNT = 1
    DEFAULT_EDGECNT = 1

    # Client CLI
    DEFAULT_IS_WARMUP_SPEEDUP = True
    DEFAULT_PERCLIENT_OPCNT = 1000000
    DEFAULT_PERCLIENT_WORKERCNT = 1

    # Cloud CLI
    DEFAULT_CLOUD_STORAGE = "hdd"

    # Dataset loader CLI
    DEFAULT_DATASET_LOADERCNT = 1
    DEFAULT_CLOUD_IDX = 0

    # Edge CLI
    DEFAULT_CACHE_NAME = "lru"
    DEFAULT_HASH_NAME = "mmh3"
    DEFAULT_PERCACHESERVER_WORKERCNT = 1
    DEFAULT_COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_MB = 1
    DEFAULT_COVERED_PEREDGE_SYNCED_VICTIMCNT = 3
    DEFAULT_COVERED_PEREDGE_MONITORED_VICTIMSETCNT = 3
    DEFAULT_COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_MB = 1
    DEFAULT_COVERED_POPULARITY_COLLECTION_CHANGE_RATIO = 0.1
    DEFAULT_COVERED_TOPK_EDGECNT = 1

    # Edgescale CLI
    DEFAULT_CAPACITY_MB = 1024

    # Evaluator CLI
    DEFAULT_WARMUP_REQCNT_SCALE = 5
    #DEFAULT_WARMUP_MAX_DURATION_SEC = 30
    DEFAULT_STRESSTEST_DURATION_SEC = 30

    # Propagation CLI
    DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_US = 1000
    DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_US = 10000
    DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_US = 100000

    # Workload CLI
    DEFAULT_KEYCNT = 10000
    DEFAULT_WORKLOAD_NAME = "facebook"

    def __init__(self, scriptname, clientcnt=DEFAULT_CLIENTCNT, edgecnt=DEFAULT_EDGECNT, is_warmup_speedup=DEFAULT_IS_WARMUP_SPEEDUP, perclient_opcnt=DEFAULT_PERCLIENT_OPCNT, perclient_workercnt=DEFAULT_PERCLIENT_WORKERCNT, cloud_storage=DEFAULT_CLOUD_STORAGE, dataset_loadercnt=DEFAULT_DATASET_LOADERCNT, cloud_idx=DEFAULT_CLOUD_IDX, cache_name=DEFAULT_CACHE_NAME, hash_name=DEFAULT_HASH_NAME, percacheserver_workercnt=DEFAULT_PERCACHESERVER_WORKERCNT, covered_local_uncached_max_mem_usage_mb=DEFAULT_COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_MB, covered_peredge_synced_victimcnt=DEFAULT_COVERED_PEREDGE_SYNCED_VICTIMCNT, covered_peredge_monitored_victimsetcnt=DEFAULT_COVERED_PEREDGE_MONITORED_VICTIMSETCNT, covered_popularity_aggregation_max_mem_usage_mb=DEFAULT_COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_MB, covered_popularity_collection_change_ratio=DEFAULT_COVERED_POPULARITY_COLLECTION_CHANGE_RATIO, covered_topk_edgecnt=DEFAULT_COVERED_TOPK_EDGECNT, capacity_mb=DEFAULT_CAPACITY_MB, warmup_reqcnt_scale=DEFAULT_WARMUP_REQCNT_SCALE, stresstest_duration_sec=DEFAULT_STRESSTEST_DURATION_SEC, propagation_latency_clientedge_us=DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_US, propagation_latency_crossedge_us=DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_US, propagation_latency_edgecloud_us=DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_US, keycnt=DEFAULT_KEYCNT, workload_name=DEFAULT_WORKLOAD_NAME):
        self.scriptname_ = scriptname

        # CLI base
        self.clientcnt_ = clientcnt
        self.edgecnt_ = edgecnt

        # Client CLI
        self.is_warmup_speedup_ = is_warmup_speedup
        self.perclient_opcnt_ = perclient_opcnt
        self.perclient_workercnt_ = perclient_workercnt

        # Cloud CLI
        self.cloud_storage_ = cloud_storage

        # Dataset loader CLI
        self.dataset_loadercnt_ = dataset_loadercnt
        self.cloud_idx_ = cloud_idx

        # Edge CLI
        self.cache_name_ = cache_name
        self.hash_name_ = hash_name
        self.percacheserver_workercnt_ = percacheserver_workercnt
        self.covered_local_uncached_max_mem_usage_mb_ = covered_local_uncached_max_mem_usage_mb
        self.covered_peredge_synced_victimcnt_ = covered_peredge_synced_victimcnt
        self.covered_peredge_monitored_victimsetcnt_ = covered_peredge_monitored_victimsetcnt
        self.covered_popularity_aggregation_max_mem_usage_mb_ = covered_popularity_aggregation_max_mem_usage_mb
        self.covered_popularity_collection_change_ratio_ = covered_popularity_collection_change_ratio
        self.covered_topk_edgecnt_ = covered_topk_edgecnt

        # Edgescale CLI
        self.capacity_mb_ = capacity_mb

        # Evaluator CLI
        self.warmup_reqcnt_scale_ = warmup_reqcnt_scale
        #self.warmup_max_duration_sec_ = warmup_max_duration_sec
        self.stresstest_duration_sec_ = stresstest_duration_sec

        # Propagation CLI
        self.propagation_latency_clientedge_us_ = propagation_latency_clientedge_us
        self.propagation_latency_crossedge_us_ = propagation_latency_crossedge_us
        self.propagation_latency_edgecloud_us_ = propagation_latency_edgecloud_us

        # Workload CLI
        self.keycnt_ = keycnt
        self.workload_name_ = workload_name
    
    def getClientCLIStr(self):
        client_cli_str = ""
        client_cli_str += self.getCLIBaseStr_()
        client_cli_str += self.getEdgescaleCLIStr_()
        client_cli_str += self.getPropagationCLIStr_()
        client_cli_str += self.getWorkloadCLIStr_()
        client_cli_str += self.getClientCLIStr_()
        
        # Strip the first blank space
        if client_cli_str != "":
            if client_cli_str[0] != " ":
                die(self.scriptname_, "the first char in client_cli_str should be blank space")
            client_cli_str = client_cli_str[1:] # client_cli_str[0] is " " here
        
        return client_cli_str
    
    def getEdgeCLIStr(self):
        edge_cli_str = ""
        edge_cli_str += self.getCLIBaseStr_()
        edge_cli_str += self.getEdgescaleCLIStr_()
        edge_cli_str += self.getPropagationCLIStr_()
        edge_cli_str += self.getEdgeCLIStr_()
        
        # Strip the first blank space
        if edge_cli_str != "":
            if edge_cli_str[0] != " ":
                die(self.scriptname_, "the first char in edge_cli_str should be blank space")
            edge_cli_str = edge_cli_str[1:]
        
        return edge_cli_str
    
    def getCloudCLIStr(self):
        cloud_cli_str = ""
        cloud_cli_str += self.getCLIBaseStr_()
        cloud_cli_str += self.getPropagationCLIStr_()
        cloud_cli_str += self.getWorkloadCLIStr_()
        cloud_cli_str += self.getCloudCLIStr_()

        # Strip the first blank space
        if cloud_cli_str != "":
            if cloud_cli_str[0] != " ":
                die(self.scriptname_, "the first char in cloud_cli_str should be blank space")
            cloud_cli_str = cloud_cli_str[1:]
        
        return cloud_cli_str

    def getEvaluatorCLIStr(self):
        evaluator_cli_str = ""
        evaluator_cli_str += self.getCLIBaseStr_()
        evaluator_cli_str += self.getEdgescaleCLIStr_()
        evaluator_cli_str += self.getPropagationCLIStr_()
        evaluator_cli_str += self.getWorkloadCLIStr_()
        evaluator_cli_str += self.getClientCLIStr_()
        evaluator_cli_str += self.getEdgeCLIStr_()
        evaluator_cli_str += self.getEvaluatorCLIStr_()

        # Strip the first blank space
        if evaluator_cli_str != "":
            if evaluator_cli_str[0] != " ":
                die(self.scriptname_, "the first char in evaluator_cli_str should be blank space")
            evaluator_cli_str = evaluator_cli_str[1:]
        
        return evaluator_cli_str

    def getSimulatorCLIStr(self):
        simulator_cli_str = ""
        simulator_cli_str += self.getCLIBaseStr_()
        simulator_cli_str += self.getEdgescaleCLIStr_()
        simulator_cli_str += self.getPropagationCLIStr_()
        simulator_cli_str += self.getWorkloadCLIStr_()
        simulator_cli_str += self.getClientCLIStr_()
        simulator_cli_str += self.getEdgeCLIStr_()
        evaluator_cli_str += self.getEvaluatorCLIStr_()
        simulator_cli_str += self.getCloudCLIStr_()

        # Strip the first blank space
        if simulator_cli_str != "":
            if simulator_cli_str[0] != " ":
                die(self.scriptname_, "the first char in simulator_cli_str should be blank space")
            simulator_cli_str = simulator_cli_str[1:]
        
        return simulator_cli_str

    def getDatasetLoaderCLIStr(self):
        dataset_loader_cli_str = ""
        dataset_loader_cli_str += self.getCLIBaseStr_()
        dataset_loader_cli_str += self.getPropagationCLIStr_()
        dataset_loader_cli_str += self.getWorkloadCLIStr_()
        dataset_loader_cli_str += self.getCloudCLIStr_()
        dataset_loader_cli_str += self.getDatasetLoaderCLIStr_()

        # Strip the first blank space
        if dataset_loader_cli_str != "":
            if dataset_loader_cli_str[0] != " ":
                die(self.scriptname_, "the first char in dataset_loader_cli_str should be blank space")
            dataset_loader_cli_str = dataset_loader_cli_str[1:]
        
        return dataset_loader_cli_str

    # Private functions
    
    def getCLIBaseStr_(self):
        cli_base_str = ""
        if self.clientcnt_ != self.DEFAULT_CLIENTCNT:
            cli_base_str += " --clientcnt " + str(self.clientcnt_)
        if self.edgecnt_ != self.DEFAULT_EDGECNT:
            cli_base_str += " --edgecnt " + str(self.edgecnt_)
        return cli_base_str
    
    def getClientCLIStr_(self):
        client_cli_str = ""
        if self.is_warmup_speedup_ != self.DEFAULT_IS_WARMUP_SPEEDUP:
            if self.is_warmup_speedup_ is True:
                die(self.scriptname_, "default is_warmup_speedup should be True")
            client_cli_str += " --disable_warmup_speedup" # self.is_warmup_speedup_ is False here
        if self.perclient_opcnt_ != self.DEFAULT_PERCLIENT_OPCNT:
            client_cli_str += " --perclient_opcnt " + str(self.perclient_opcnt_)
        if self.perclient_workercnt_ != self.DEFAULT_PERCLIENT_WORKERCNT:
            client_cli_str += " --perclient_workercnt " + str(self.perclient_workercnt_)
        return client_cli_str
    
    def getCloudCLIStr_(self):
        cloud_cli_str = ""
        if self.cloud_storage_ != self.DEFAULT_CLOUD_STORAGE:
            cloud_cli_str += " --cloud_storage " + str(self.cloud_storage_)
        return cloud_cli_str
    
    def getDatasetLoaderCLIStr_(self):
        dataset_loader_cli_str = ""
        if self.dataset_loadercnt_ != self.DEFAULT_DATASET_LOADERCNT:
            dataset_loader_cli_str += " --dataset_loadercnt " + str(self.dataset_loadercnt_)
        if self.cloud_idx_ != self.DEFAULT_CLOUD_IDX:
            dataset_loader_cli_str += " --cloud_idx " + str(self.cloud_idx_)
        return dataset_loader_cli_str

    def getEdgeCLIStr_(self):
        edge_cli_str = ""
        if self.cache_name_ != self.DEFAULT_CACHE_NAME:
            edge_cli_str += " --cache_name " + str(self.cache_name_)
        if self.hash_name_ != self.DEFAULT_HASH_NAME:
            edge_cli_str += " --hash_name " + str(self.hash_name_)
        if self.percacheserver_workercnt_ != self.DEFAULT_PERCACHESERVER_WORKERCNT:
            edge_cli_str += " --percacheserver_workercnt " + str(self.percacheserver_workercnt_)
        if self.covered_local_uncached_max_mem_usage_mb_ != self.DEFAULT_COVERED_LOCAL_UNCACHED_MAX_MEM_USAGE_MB:
            edge_cli_str += " --covered_local_uncached_max_mem_usage_mb " + str(self.covered_local_uncached_max_mem_usage_mb_)
        if self.covered_peredge_synced_victimcnt_ != self.DEFAULT_COVERED_PEREDGE_SYNCED_VICTIMCNT:
            edge_cli_str += " --covered_peredge_synced_victimcnt " + str(self.covered_peredge_synced_victimcnt_)
        if self.covered_peredge_monitored_victimsetcnt_ != self.DEFAULT_COVERED_PEREDGE_MONITORED_VICTIMSETCNT:
            edge_cli_str += " --covered_peredge_monitored_victimsetcnt " + str(self.covered_peredge_monitored_victimsetcnt_)
        if self.covered_popularity_aggregation_max_mem_usage_mb_ != self.DEFAULT_COVERED_POPULARITY_AGGREGATION_MAX_MEM_USAGE_MB:
            edge_cli_str += " --covered_popularity_aggregation_max_mem_usage_mb " + str(self.covered_popularity_aggregation_max_mem_usage_mb_)
        if self.covered_popularity_collection_change_ratio_ != self.DEFAULT_COVERED_POPULARITY_COLLECTION_CHANGE_RATIO:
            edge_cli_str += " --covered_popularity_collection_change_ratio " + str(self.covered_popularity_collection_change_ratio_)
        if self.covered_topk_edgecnt_ != self.DEFAULT_COVERED_TOPK_EDGECNT:
            edge_cli_str += " --covered_topk_edgecnt " + str(self.covered_topk_edgecnt_)
        return edge_cli_str
    
    def getEdgescaleCLIStr_(self):
        edgescale_cli_str = ""
        if self.capacity_mb_ != self.DEFAULT_CAPACITY_MB:
            edgescale_cli_str += " --capacity_mb " + str(self.capacity_mb_)
        return edgescale_cli_str

    def getEvaluatorCLIStr_(self):
        evaluator_cli_str = ""
        if self.warmup_reqcnt_scale_ != self.DEFAULT_WARMUP_REQCNT_SCALE:
            evaluator_cli_str += " --warmup_reqcnt_scale " + str(self.warmup_reqcnt_scale_)
        #if self.warmup_max_duration_sec_ != self.DEFAULT_WARMUP_MAX_DURATION_SEC:
        #    evaluator_cli_str += " --warmup_max_duration_sec " + str(self.warmup_max_duration_sec_)
        if self.stresstest_duration_sec_ != self.DEFAULT_STRESSTEST_DURATION_SEC:
            evaluator_cli_str += " --stresstest_duration_sec " + str(self.stresstest_duration_sec_)
        return evaluator_cli_str
    
    def getPropagationCLIStr_(self):
        propagation_cli_str = ""
        if self.propagation_latency_clientedge_us_ != self.DEFAULT_PROPAGATION_LATENCY_CLIENTEDGE_US:
            propagation_cli_str += " --propagation_latency_clientedge_us " + str(self.propagation_latency_clientedge_us_)
        if self.propagation_latency_crossedge_us_ != self.DEFAULT_PROPAGATION_LATENCY_CROSSEDGE_US:
            propagation_cli_str += " --propagation_latency_crossedge_us " + str(self.propagation_latency_crossedge_us_)
        if self.propagation_latency_edgecloud_us_ != self.DEFAULT_PROPAGATION_LATENCY_EDGECLOUD_US:
            propagation_cli_str += " --propagation_latency_edgecloud_us " + str(self.propagation_latency_edgecloud_us_)
        return propagation_cli_str
    
    def getWorkloadCLIStr_(self):
        workload_cli_str = ""
        if self.keycnt_ != self.DEFAULT_KEYCNT:
            workload_cli_str += " --keycnt " + str(self.keycnt_)
        if self.workload_name_ != self.DEFAULT_WORKLOAD_NAME:
            workload_cli_str += " --workload_name " + str(self.workload_name_)
        return workload_cli_str