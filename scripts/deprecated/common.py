# (1) Deprecated: after list_immediate_files_and_directories()

def getPreferredDirpathForTarget(infix_name, target_name, env_pathstr):
    if env_pathstr == None:
        print("ERROR: env_pathstr for {} is None!",format(target_name))
        sys.exit(1)
    else:
        print("env_pathstr for {}: {}".format(target_name, env_pathstr))

    preferred_dirpath = ""
    env_paths = env_pathstr.split(":")
    for i in range(len(env_paths)):
        tmp_env_path = env_paths[i].strip()

        # Judge whether the file or directory target_name is under tmp_env_path, if tmp_env_path exists and is a directory
        if "/usr/{}".format(infix_name) in tmp_env_path or "/usr/local/{}".format(infix_name) in tmp_env_path:
            tmp_sub_files_or_dirs = PathUtil.list_immediate_files_and_directories(tmp_env_path)
            for tmp_sub_file_or_dir in tmp_sub_files_or_dirs:
                if target_name in tmp_sub_file_or_dir:
                    preferred_dirpath = tmp_env_path.strip()
                    break
    
    if preferred_dirpath == "":
        print("ERROR: /usr/{0}/{1} or /usr/local/{0}/{1} are also NOT found in {2}!".format(infix_name, target_name, env_pathstr))
        sys.exit(1)
    
    return preferred_dirpath

# (2) Deprecated: after getPreferredDirpathForTarget()

# def getDefaultIncludeAndLibPathstr():
#     get_gpp_include_pathstr_cmd = "echo | g++ -E -Wp,-v -x c++ - 2>&1 | grep -v '#' | grep -v '^$' | grep -v 'search starts here:' | grep -v 'End of search list.' | grep -v 'ignoring'"
#     get_gpp_include_pathstr_subprocess = subprocess.run(get_gpp_include_pathstr_cmd, shell=True, capture_output=True)
#     if get_gpp_include_pathstr_subprocess.returncode != 0:
#         print("ERROR: get_gpp_include_pathstr_subprocess returncode is not 0!")
#         sys.exit(1)
#     else:
#         get_gpp_include_pathstr_outputbytes = get_gpp_include_pathstr_subprocess.stdout
#         get_gpp_include_pathstr_outputstr = get_gpp_include_pathstr_outputbytes.decode("utf-8")

#         gpp_include_pathstr_elements = get_gpp_include_pathstr_outputstr.split("\n")
#         for i in range(len(gpp_include_pathstr_elements)):
#             gpp_include_pathstr_elements[i] = gpp_include_pathstr_elements[i].strip()
#         gpp_include_pathstr = ":".join(gpp_include_pathstr_elements)
    
#     get_gpp_lib_pathstr_cmd = "echo | g++ -E -v -x c++ - 2>&1 | grep 'LIBRARY_PATH'"
#     get_gpp_lib_pathstr_subprocess = subprocess.run(get_gpp_lib_pathstr_cmd, shell=True, capture_output=True)
#     if get_gpp_lib_pathstr_subprocess.returncode != 0:
#         print("ERROR: get_gpp_lib_pathstr_subprocess returncode is not 0!")
#         sys.exit(1)
#     else:
#         get_gpp_lib_pathstr_outputbytes = get_gpp_lib_pathstr_subprocess.stdout
#         get_gpp_lib_pathstr_outputstr = get_gpp_lib_pathstr_outputbytes.decode("utf-8")

#         gpp_lib_pathstr = get_gpp_lib_pathstr_outputstr.split("=")[1]

#     return gpp_include_pathstr, gpp_lib_pathstr

# (3) Deprecated: after for cmake

# For libboost (set as the default search path of find_package for cmake)
#gpp_include_pathstr, gpp_lib_pathstr = getDefaultIncludeAndLibPathstr()
#boost_preferred_include_dirpath = PathUtil.getPreferredDirpathForTarget("include", "boost", gpp_include_pathstr)
#boost_preferred_lib_dirpath = PathUtil.getPreferredDirpathForTarget("lib", "boost", gpp_lib_pathstr)
boost_preferred_include_dirpath = "/usr/local/include"
boost_preferred_lib_dirpath = "/usr/local/lib"

#print("{} {}".format(boost_preferred_include_dirpath, boost_preferred_lib_dirpath))

# (4) OBSOLETE due to too close backend storage in Singapore

# (4.1) scripts/common.py

# Initialized single-trip time (RTT / 2)
# NOTE: used for the first round (for round 0)
#alicloud_avg_clientedge_latency_us = 750
#alicloud_avg_crossedge_latency_us = 10650
#alicloud_avg_edgecloud_latency_us = 30000
# NOTE: used for the second round (for round 1)
#alicloud_avg_clientedge_latency_us = 742
#alicloud_avg_crossedge_latency_us = 12140
#alicloud_avg_edgecloud_latency_us = 29985
# NOTE: used for the third round (for round 3)
#alicloud_avg_clientedge_latency_us = 755
#alicloud_avg_crossedge_latency_us = 10745
#alicloud_avg_edgecloud_latency_us = 30100

# (4.2) docs/readme/ailcloud.md

# - Test time: 9:58AM, Mar. 31, 2024 (use for round0)
#     - Backend public IP: 8.219.4.220
#     - Average client-cache: 1.5 ms
#     - Average cross-cache: 21.3 ms
#     - Average cache-cloud: 60 ms

# | One End | Another End | Ping RTT (ms) |
# | --- | --- | --- |
# | client0 | cache 0 | 1.8 |
# | client1 | cache 1 | 0.25 |
# | client2 | cache 2 | 0.75 |
# | client3 | cache 3 | 3.2 |
# | cache0 | cache1 | 8.7 |
# | cache0 | cache2 | 27 |
# | cache0 | cache3 | 30.7 |
# | cache1 | cache2 | 28 |
# | cache1 | cache3 | 25.6 |
# | cache2 | cache3 | 7.8 |
# | cache0 | backend | 67.5 |
# | cache1 | backend | 66.5 |
# | cache2 | backend | 50.1 |
# | cache3 | backend | 55.9 |

# - Test time: 14:00PM, Mar. 31, 2024 (use for round1)
#     - Backend public IP: 8.219.4.220
#     - Average client-cache: 1.485 ms
#     - Average cross-cache: 24.28 ms
#     - Average cache-cloud: 59.97 ms

# | One End | Another End | Ping RTT (ms) |
# | --- | --- | --- |
# | client0 | cache 0 | 1.67 |
# | client1 | cache 1 | 0.27 |
# | client2 | cache 2 | 0.8 |
# | client3 | cache 3 | 3.2 |
# | cache0 | cache1 | 8.75 |
# | cache0 | cache2 | 27 |
# | cache0 | cache3 | 30.8 |
# | cache1 | cache2 | 28 |
# | cache1 | cache3 | 25.6 |
# | cache2 | cache3 | 25.5 |
# | cache0 | backend | 67.5 |
# | cache1 | backend | 66.5 |
# | cache2 | backend | 50.1 |
# | cache3 | backend | 55.8 |

# - Test time: 20:25PM, Mar. 31, 2024 (use for round2)
#     - Backend public IP: 8.219.4.220
#     - Average client-cache: 1.51 ms
#     - Average cross-cache: 21.49 ms
#     - Average cache-cloud: 60.2 ms

# | One End | Another End | Ping RTT (ms) |
# | --- | --- | --- |
# | client0 | cache 0 | 2.03 |
# | client1 | cache 1 | 0.28 |
# | client2 | cache 2 | 0.78 |
# | client3 | cache 3 | 2.96 |
# | cache0 | cache1 | 8.74 |
# | cache0 | cache2 | 26.9 |
# | cache0 | cache3 | 30.7 |
# | cache1 | cache2 | 29.5 |
# | cache1 | cache3 | 25.8 |
# | cache2 | cache3 | 7.3 |
# | cache0 | backend | 67.7 |
# | cache1 | backend | 66.5 |
# | cache2 | backend | 50.2 |
# | cache3 | backend | 56.4 |

# (4.3) docs/exp.md

# ## (4) Real-network Analysis

# ### (4.1) Real-network Analysis on Per-edge Memory Capacity

# - Round 0.

# | Method | Per-cache-node Memory (GiB) | Global Hit Ratio (%) (Local + Cooperative) | Average Latency (ms) | Throughput (OPS) | Per-request Average Bandwidth Cost (MiB/pkt) (cross-edge/edge-cloud) | Per-request Msgcnt (cross-edge/edge-cloud) |
# | --- | --- | --- | --- | --- | --- | --- |
# | GDSF+ | 1 | 63.2857 (52.7143 + 10.5714) | 86.118 | 46.6667 | 0.00242654/0.02608 | 1.83/0.734286 |
# | GDSF+ | 2 | 76.489 (63.7409 + 12.7482) | 62.881 | 63.8 | 0.00446331/0.0204255 | 1.47649/0.470219 |
# | GDSF+ | 4 | 87.484 (76.8886 + 10.5954) | 38.642 | 104.133 | 0.00492611/0.0140696 | 0.973111/0.25032 |
# | GDSF+ | 8 | 96.2947 (89.9915 + 6.30324) | 17.183 | 234.8 | 0.0047397/0.00683243 | 0.456275/0.0741056 |
# | LHD+ | 1 | 62.0629 (48.0769 + 13.986) | 104.869 | 38.1333 | 0.00281109/0.0260813 | 4.37238/0.758741 |
# | LHD+ | 2 | 73.2143 (59.6561 + 13.5582) | 79.424 | 50.4 | 0.00380491/0.0229565 | 3.15873/0.535714 |
# | LHD+ | 4 | 84.6396 (71.6216 + 13.018) | 54.494 | 74 | 0.00424788/0.0173612 | 2.02162/0.307207 |
# | LHD+ | 8 | 95.298 (85.7084 + 9.58961) | 24.907 | 161.633 | 0.00561202/0.00727344 | 0.96721/0.09404 |
# | COVERED | 1 | 68.1769 (50.5933 + 17.5836) | 64.736 | 61.8 | 0.00395428/0.024416 | 1.13376/0.636462 |
# | COVERED | 2 | 80.2538 (61.8557 + 18.3981) | 47.738 | 84.0667 | 0.00576469/0.018979 | 1.00872/0.394925 |
# | COVERED | 4 | 91.5149 (75.1678 + 16.3471) | 28.854 | 139.067 | 0.00791944/0.0115877 | 0.750719/0.169703 |
# | COVERED | 8 | 98.0446 (89.3062 + 8.7384) | 13.407 | 301.733 | 0.00695856/0.00491359 | 0.411622/0.0391074 |

# - Round 1.

# | Method | Per-cache-node Memory (GiB) | Global Hit Ratio (%) (Local + Cooperative) | Average Latency (ms) | Throughput (OPS) | Per-request Average Bandwidth Cost (MiB/pkt) (cross-edge/edge-cloud) | Per-request Msgcnt (cross-edge/edge-cloud) |
# | --- | --- | --- | --- | --- | --- | --- |
# | GDSF+ | 1 | 62.8338 (52.2997 + 10.5341) | 89.23 | 44.9333 | 0.00245268/0.0266945 | 1.83828/0.743323 |
# | GDSF+ | 2 | 76.4297 (63.4955 + 12.9343) | 64.179 | 62.3667 | 0.00453244/0.0205916 | 1.47728/0.471406 |
# | GDSF+ | 4 | 87.4385 (77.3368 + 10.1017) | 39.506 | 101.633 | 0.00469201/0.0141937 | 0.943916/0.25123 |
# | GDSF+ | 8 | 96.322 (89.795 + 6.52698) | 18.37 | 221.133 | 0.00479251/0.00676088 | 0.467591/0.0735604 |
# | LHD+ | 1 | 62.15 (49.1418 + 13.0081) | 108.317 | 36.9 | 0.00266965/0.0262326 | 4.49142/0.757001 |
# | LHD+ | 2 | 72.8421 (60 + 12.8421) | 84.397 | 47.5 | 0.0032114/0.0234304 | 3.17754/0.543158 |
# | LHD+ | 4 | 84.9403 (72.2681 + 12.6722) | 55.277 | 72.6 | 0.00410432/0.0176241 | 2.03673/0.301194 |
# | LHD+ | 8 | 95.2834 (85.7823 + 9.50113) | 27.242 | 147 | 0.00589143/0.00717879 | 0.97551/0.0943311 |
# | COVERED | 1 | 67.5511 (51.1885 + 16.3626) | 66.537 | 60.3 | 0.00357778/0.0247741 | 1.10061/0.648977 |
# | COVERED | 2 | 79.9267 (62.5814 + 17.3453) | 48.997 | 81.8667 | 0.0055077/0.0190784 | 0.963762/0.401466 |
# | COVERED | 4 | 90.8818 (75.8016 + 15.0802) | 30.118 | 133.067 | 0.00737548/0.012287 | 0.723196/0.182365 |
# | COVERED | 8 | 97.8506 (89.3924 + 8.45823) | 14.177 | 286.9 | 0.00689924/0.0051448 | 0.398861/0.0429883 |

# - Round 2.

# | Method | Per-cache-node Memory (GiB) | Global Hit Ratio (%) (Local + Cooperative) | Average Latency (ms) | Throughput (OPS) | Per-request Average Bandwidth Cost (MiB/pkt) (cross-edge/edge-cloud) | Per-request Msgcnt (cross-edge/edge-cloud) |
# | --- | --- | --- | --- | --- | --- | --- |
# | GDSF+ | 1 | 63.0435 (52.3188 + 10.7246) | 87.245 | 46 | 0.00247937/0.0263174 | 1.83768/0.73913 |
# | GDSF+ | 2 | 76.4798 (63.4995 + 12.9803) | 62.502 | 64.2 | 0.00447034/0.0204004 | 1.48806/0.470405 |
# | GDSF+ | 4 | 87.5553 (77.2584 + 10.2969) | 38.112 | 105.533 | 0.00495034/0.0139688 | 0.948831/0.248895 |
# | GDSF+ | 8 | 96.3558 (89.8422 + 6.51363) | 17.371 | 232.333 | 0.00503168/0.00680556 | 0.463702/0.0728838 |
# | LHD+ | 1 | 61.5865 (49.287 + 12.2995) | 106.937 | 37.4 | 0.00248237/0.0266098 | 4.45098/0.768271 |
# | LHD+ | 2 | 73.0951 (59.6763 + 13.4187) | 81.233 | 49.4333 | 0.00348048/0.0234436 | 3.25287/0.538098 |
# | LHD+ | 4 | 84.9333 (72.0889 + 12.8444) | 53.608 | 75 | 0.0044913/0.0172607 | 2.00267/0.301333 |
# | LHD+ | 8 | 95.0934 (85.2801 + 9.81329) | 26.284 | 153.533 | 0.0054018/0.0075182 | 0.988276/0.0981329 |
# | COVERED | 1 | 68.5471 (50.8249 + 17.7222) | 64.126 | 62.6333 | 0.00404869/0.0242369 | 1.14582/0.629058 |
# | COVERED | 2 | 79.9367 (61.8916 + 18.0451) | 47.595 | 84.2333 | 0.00569496/0.0190783 | 0.991294/0.401266 |
# | COVERED | 4 | 91.3288 (75.0611 + 16.2677) | 29.501 | 136.467 | 0.00787537/0.0119761 | 0.762091/0.173913 |
# | COVERED | 8 | 98.1581 (89.3503 + 8.80777) | 13.529 | 298.6 | 0.0073157/0.00463204 | 0.414155/0.0368386 |

# ### (4.2) Real-network Analysis on Different Workloads

# - Round 0.

# | Method | Workload | Global Hit Ratio (%) (Local + Cooperative) | Average Latency (ms) | Throughput (OPS) | Per-request Average Bandwidth Cost (MiB/pkt) (cross-edge/edge-cloud) | Per-request Msgcnt (cross-edge/edge-cloud) |
# | --- | --- | --- | --- | --- | --- | --- |
# | GDSF+ | Facebook | 84.755 (74.0109 + 10.7441) | 43.79 | 91.8333 | 0.00451602/0.0157187 | 1.07005/0.3049 |
# | GDSF+ | WikiImage | 94.1502 (83.0953 + 11.0548) | 27.685 | 144.733 | 0.00534893/0.0330107 | 0.827729/0.116997 |
# | GDSF+ | WikiText | 69.2112 (34.8601 + 34.3511) | 101.851 | 39.3 | 0.0102675/0.013494 | 2.87871/0.615776 |
# | LHD+ | Facebook | 82.5639 (69.4499 + 13.1139) | 59.172 | 67.8667 | 0.00458425/0.0182018 | 2.28585/0.348723 |
# | LHD+ | WikiImage | 76.3948 (48.2833 + 28.1116) | 85.53 | 46.6 | 0.00946913/0.0307285 | 3.13448/0.473534 |
# | LHD+ | WikiText | 59.5843 (22.4018 + 37.1824) | 138.371 | 28.8667 | 0.0126863/0.0136206 | 4.17552/0.808314 |
# | COVERED | Facebook | 89.2623 (72.2951 + 16.9672) | 32.918 | 122 | 0.00740285/0.0132252 | 0.817486/0.214754 |
# | COVERED | WikiImage | 90.9128 (81.2386 + 9.67416) | 24.358 | 164.7 | 0.00319333/0.0318233 | 0.506982/0.181745 |
# | COVERED | WikiText | 69.8032 (37.3309 + 32.4723) | 74.489 | 54.2 | 0.00778771/0.0143787 | 1.6556/0.603936 |

# - Round 1.

# | Method | Workload | Global Hit Ratio (%) (Local + Cooperative) | Average Latency (ms) | Throughput (OPS) | Per-request Average Bandwidth Cost (MiB/pkt) (cross-edge/edge-cloud) | Per-request Msgcnt (cross-edge/edge-cloud) |
# | --- | --- | --- | --- | --- | --- | --- |
# | GDSF+ | Facebook | 84.9715 (74.0417 + 10.9298) | 45.685 | 87.8333 | 0.00481581/0.0156383 | 1.07476/0.301328 |
# | GDSF+ | WikiImage | 92.1471 (82.3115 + 9.83564) | 31.367 | 127.767 | 0.00454533/0.0380829 | 0.824941/0.157057 |
# | GDSF+ | WikiText | 67.2429 (33.394 + 33.849) | 109.351 | 36.6333 | 0.0114023/0.0128043 | 2.9172/0.655141 |
# | LHD+ | Facebook | 82.789 (68.844 + 13.945) | 62.367 | 64.3 | 0.00452501/0.018797 | 2.31415/0.34422 |
# | LHD+ | WikiImage | 85.6851 (50.1704 + 35.5147) | 81.907 | 48.9 | 0.0128971/0.0258568 | 3.25426/0.286299 |
# | LHD+ | WikiText | 59.7826 (20.7729 + 39.0097) | 144.799 | 27.6 | 0.0136091/0.0135871 | 4.36715/0.804348 |
# | COVERED | Facebook | 88.3898 (72.7966 + 15.5932) | 34.158 | 118 | 0.00667674/0.0137672 | 0.770339/0.232203 |
# | COVERED | WikiImage | 88.6849 (82.0773 + 6.60764) | 26.049 | 154.367 | 0.00224093/0.0343802 | 0.419132/0.226301 |
# | COVERED | WikiText | 67.2447 (36.9942 + 30.2505) | 79.147 | 51.9 | 0.00680618/0.0157972 | 1.57418/0.655106 |