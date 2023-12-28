#!/usr/bin/env python3
# cliutil: build CLI parameters for main programs including simulator, client, edge, cloud, and evaluator

class CLIUtil:
    def __init__(self, clientcnt=1, edgecnt=1, is_warmup_speedup=True, perclient_opcnt=1000000, perclient_workercnt=1, cloud_storage="hdd", dataset_loadercnt=1, cloud_idx=0, cache_name="lru", hash_name="mmh3", percacheserver_workercnt=1, covered_local_uncached_max_mem_usage_mb=1, covered_peredge_synced_victimcnt=3, covered_peredge_monitored_victimsetcnt=3, covered_popularity_aggregation_max_mem_usage_mb=1, covered_popularity_collection_change_ratio=0.1, covered_topk_edgecnt=1, capacity_mb=1024, warmup_reqcnt_scale=5, warmup_max_duration_sec=30, stresstest_duration_sec=30, propagation_latency_clientedge_us=1000, propagation_latency_crossedge_us=10000, propagation_latency_edgecloud_us=100000, keycnt=10000, workload_name="facebook"):
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