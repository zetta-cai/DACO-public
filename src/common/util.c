#include "common/util.h"

#include <assert.h>
#include <chrono> // system_clock
#include <errno.h> // ENOENT, errno
#include <fcntl.h> // open, close
#include <math.h> // isnan and isinf
#include <sstream> // ostringstream
#include <cmath> // pow

#include <boost/filesystem.hpp>
#include <boost/stacktrace.hpp>
#include <boost/system.hpp>

#include "common/config.h"

namespace covered
{
    // (1) For CLI parameters

    // Main class name
    const std::string Util::SIMULATOR_MAIN_NAME("simulator");
    //const std::string Util::STATISTICS_AGGREGATOR_MAIN_NAME("statistics_aggregator");
    const std::string Util::TOTAL_STATISTICS_LOADER_MAIN_NAME("total_statistics_loader");
    const std::string Util::DATASET_LOADER_MAIN_NAME("dataset_loader");
    const std::string Util::CLIENT_MAIN_NAME("client");
    const std::string Util::EDGE_MAIN_NAME("edge");
    const std::string Util::CLOUD_MAIN_NAME("cloud");
    const std::string Util::EVALUATOR_MAIN_NAME("evaluator");
    const std::string Util::CLIUTIL_MAIN_NAME("cliutil");
    const std::string Util::TRACE_PREPROCESSOR_MAIN_NAME("trace_preprocessor");

    // Workload name
    const std::string Util::AKAMAI_WEB_WORKLOAD_NAME("akamaiweb"); // TRAGEN-generated Akamai's web CDN trace (geo-distributed)
    const std::string Util::AKAMAI_VIDEO_WORKLOAD_NAME("akamaivideo"); // TRAGEN-generated Akamai's video CDN trace (geo-distributed)
    const std::string Util::FACEBOOK_WORKLOAD_NAME("facebook"); // Original Facebook CDN workload generator
    const std::string Util::ZIPF_FACEBOOK_WORKLOAD_NAME("zipf_facebook"); // Zipfian Facebook CDN workload generator (based on Zipfian power-law distribution)
    const std::string Util::ZETA_WIKIPEDIA_IMAGE_WORKLOAD_NAME("zeta_wikiimage"); // Zipfian Wikipedia image workload generator (based on Zeta distribution)
    const std::string Util::ZETA_WIKIPEDIA_TEXT_WORKLOAD_NAME("zeta_wikitext"); // Zipfian Wikipedia text workload generator (based on Zeta distribution)
    const std::string Util::ZETA_TENCENT_PHOTO1_WORKLOAD_NAME("zeta_tencentphoto1"); // Zipfian Tencent photo1 workload generator (based on Zeta distribution)
    const std::string Util::ZETA_TENCENT_PHOTO2_WORKLOAD_NAME("zeta_tencentphoto2"); // Zipfian Tencent photo2 workload generator (based on Zeta distribution)
    const std::string Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME("wikiimage"); // (OBSOLETE) Replayed wikipedia image traces (single-node)
    const std::string Util::WIKIPEDIA_TEXT_WORKLOAD_NAME("wikitext"); // (OBSOLETE) Replayed wikipedian text traces (single-node)
    const std::string Util::FBPHOTO_WORKLOAD_NAME("fbphoto"); // (OBSOLETE) Zipfian Facebook photo caching workload generator (based on Zeta distribution)

    // Cloud storage
    const std::string Util::HDD_NAME = "hdd";

    // Cache name
    // NOTE: update Util::isSingleNodeCache() accordingly if necessary
    const std::string Util::ARC_CACHE_NAME("arc");
    const std::string Util::BESTGUESS_CACHE_NAME("bestguess"); // cooperative caching
    const std::string Util::CACHELIB_CACHE_NAME("cachelib");
    const std::string Util::FIFO_CACHE_NAME("fifo");
    const std::string Util::FROZENHOT_CACHE_NAME("frozenhot");
    const std::string Util::GLCACHE_CACHE_NAME("glcache");
    const std::string Util::GDSF_CACHE_NAME("gdsf"); // Greedy dual
    const std::string Util::GDSIZE_CACHE_NAME("gdsize"); // Greedy dual
    const std::string Util::LFUDA_CACHE_NAME("lfuda"); // Greedy dual
    const std::string Util::LRUK_CACHE_NAME("lruk"); // Greedy dual
    const std::string Util::LFU_CACHE_NAME("lfu");
    const std::string Util::LHD_CACHE_NAME("lhd");
    const std::string Util::LRB_CACHE_NAME("lrb");
    const std::string Util::LRU_CACHE_NAME("lru");
    const std::string Util::S3FIFO_CACHE_NAME("s3fifo");
    const std::string Util::SEGCACHE_CACHE_NAME("segcache");
    const std::string Util::SIEVE_CACHE_NAME("sieve");
    const std::string Util::SLRU_CACHE_NAME("slru");
    const std::string Util::WTINYLFU_CACHE_NAME("wtinylfu");
    const std::string Util::COVERED_CACHE_NAME("covered");
    
    // Shark-extended cache names
    const std::string Util::SHARK_EXTENDED_ARC_CACHE_NAME("shark+arc");
    const std::string Util::SHARK_EXTENDED_CACHELIB_CACHE_NAME("shark+cachelib");
    const std::string Util::SHARK_EXTENDED_FIFO_CACHE_NAME("shark+fifo");
    const std::string Util::SHARK_EXTENDED_FROZENHOT_CACHE_NAME("shark+frozenhot");
    const std::string Util::SHARK_EXTENDED_GLCACHE_CACHE_NAME("shark+glcache");
    const std::string Util::SHARK_EXTENDED_GDSF_CACHE_NAME("shark+gdsf");
    const std::string Util::SHARK_EXTENDED_GDSIZE_CACHE_NAME("shark+gdsize");
    const std::string Util::SHARK_EXTENDED_LFUDA_CACHE_NAME("shark+lfuda");
    const std::string Util::SHARK_EXTENDED_LRUK_CACHE_NAME("shark+lruk");
    const std::string Util::SHARK_EXTENDED_LFU_CACHE_NAME("shark+lfu");
    const std::string Util::SHARK_EXTENDED_LHD_CACHE_NAME("shark+lhd");
    const std::string Util::SHARK_EXTENDED_LRB_CACHE_NAME("shark+lrb");
    const std::string Util::SHARK_EXTENDED_LRU_CACHE_NAME("shark"); // shark+lru = shark
    const std::string Util::SHARK_EXTENDED_S3FIFO_CACHE_NAME("shark+s3fifo");
    const std::string Util::SHARK_EXTENDED_SEGCACHE_CACHE_NAME("shark+segcache");
    const std::string Util::SHARK_EXTENDED_SIEVE_CACHE_NAME("shark+sieve");
    const std::string Util::SHARK_EXTENDED_SLRU_CACHE_NAME("shark+slru");
    const std::string Util::SHARK_EXTENDED_WTINYLFU_CACHE_NAME("shark+wtinylfu");

    // MagNet-extended cache names
    const std::string Util::MAGNET_EXTENDED_ARC_CACHE_NAME("magnet+arc");
    const std::string Util::MAGNET_EXTENDED_CACHELIB_CACHE_NAME("magnet+cachelib");
    const std::string Util::MAGNET_EXTENDED_FIFO_CACHE_NAME("magnet+fifo");
    const std::string Util::MAGNET_EXTENDED_FROZENHOT_CACHE_NAME("magnet+frozenhot");
    const std::string Util::MAGNET_EXTENDED_GLCACHE_CACHE_NAME("magnet+glcache");
    const std::string Util::MAGNET_EXTENDED_GDSF_CACHE_NAME("magnet+gdsf");
    const std::string Util::MAGNET_EXTENDED_GDSIZE_CACHE_NAME("magnet+gdsize");
    const std::string Util::MAGNET_EXTENDED_LFUDA_CACHE_NAME("magnet+lfuda");
    const std::string Util::MAGNET_EXTENDED_LRUK_CACHE_NAME("magnet+lruk");
    const std::string Util::MAGNET_EXTENDED_LFU_CACHE_NAME("magnet+lfu");
    const std::string Util::MAGNET_EXTENDED_LHD_CACHE_NAME("magnet+lhd");
    const std::string Util::MAGNET_EXTENDED_LRB_CACHE_NAME("magnet+lrb");
    const std::string Util::MAGNET_EXTENDED_LRU_CACHE_NAME("magnet"); // magnet+lru = magnet
    const std::string Util::MAGNET_EXTENDED_S3FIFO_CACHE_NAME("magnet+s3fifo");
    const std::string Util::MAGNET_EXTENDED_SEGCACHE_CACHE_NAME("magnet+segcache");
    const std::string Util::MAGNET_EXTENDED_SIEVE_CACHE_NAME("magnet+sieve");
    const std::string Util::MAGNET_EXTENDED_SLRU_CACHE_NAME("magnet+slru");
    const std::string Util::MAGNET_EXTENDED_WTINYLFU_CACHE_NAME("magnet+wtinylfu");

    // Hash name
    const std::string Util::MMH3_HASH_NAME("mmh3");

    // Real-network options
    const std::string Util::REALNET_NO_OPTION_NAME("disable");
    const std::string Util::REALNET_DUMP_OPTION_NAME("dump");
    const std::string Util::REALNET_LOAD_OPTION_NAME("load");

    // (2) For utility functions

    // Type conversion/checking
    const int64_t Util::MAX_UINT16 = 65536;
    const int64_t Util::MAX_UINT32 = 4294967296;
    const double Util::DOUBLE_IOTA = 0.1;

    // Network
    const std::string Util::ANY_IPSTR("0.0.0.0");
    const uint32_t Util::UDP_MAX_PKT_PAYLOAD = 65507; // 65535(ipmax) - 20(iphdr) - 8(udphdr)
    const uint32_t Util::UDP_FRAGHDR_SIZE = 5 * sizeof(uint32_t) + sizeof(uint16_t); // 4(fragment_idx) + 4(fragment_cnt) + 4(msg_payload_size) + 4(msg_seqnum) + 4(source_ip) + 2(source_port)
    const uint32_t Util::UDP_MAX_FRAG_PAYLOAD = Util::UDP_MAX_PKT_PAYLOAD - Util::UDP_FRAGHDR_SIZE;
    const uint16_t Util::UDP_MIN_PORT = 1024; // UDP port has to be >= 1024 (0-1023 are reserved for well-known usage; 1024-49151 are registered ports; 49152-65535 are custom ports)

    // Atomicity
    std::memory_order Util::LOAD_CONCURRENCY_ORDER = std::memory_order_acquire;
    std::memory_order Util::STORE_CONCURRENCY_ORDER = std::memory_order_release;
    std::memory_order Util::RMW_CONCURRENCY_ORDER = std::memory_order_acq_rel;

    // Workflow control
    const unsigned int Util::SLEEP_INTERVAL_US = MS2US(100); // 100ms

    // Workload generation
    const uint32_t Util::DATASET_KVPAIR_GENERATION_SEED = 0;
    //const uint32_t Util::WORKLOAD_KVPAIR_GENERATION_SEED = 1;
    const uint32_t Util::DATASET_KVPAIR_SAMPLE_SEED = 0;

    // Time measurement
    const int Util::START_YEAR = 1900;
    const long Util::NANOSECONDS_PERSECOND = 1000000000L;
    const uint32_t Util::SECOND_PRECISION = 6;

    // For charset
    const std::string Util::CHARSET("0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz");
    std::mt19937_64 Util::string_randgen_(0);
    std::uniform_int_distribution<uint32_t> Util::string_randdist_(0, CHARSET.size() - 1); // Range of [0, CHARSET.size() - 1]

    // I/O
    const int64_t Util::MAX_MMAP_BLOCK_SIZE = GB2B(1); // 1GB
    const int Util::LINE_SEP_CHAR = '\n';
    const int Util::TSV_SEP_CHAR = '\t';

    // Propagation latency simulation
    const std::string Util::PROPAGATION_SIMULATION_UNIFORM_DISTNAME("uniform");
    const std::string Util::PROPAGATION_SIMULATION_CONSTANT_DISTNAME("constant");
    const uint32_t Util::PROPAGATION_SIMULATION_CLIENT_SEED_BASE = 0;
    const uint32_t Util::PROPAGATION_SIMULATION_CLIENT_SEED_FACTOR = 10;
    const uint32_t Util::PROPAGATION_SIMULATION_EDGE_SEED_BASE = 10000;
    const uint32_t Util::PROPAGATION_SIMULATION_EDGE_SEED_FACTOR = 10;
    const uint32_t Util::PROPAGATION_SIMULATION_CLOUD_SEED_BASE = 20000;
    const uint32_t Util::PROPAGATION_SIMULATION_CLOUD_SEED_FACTOR = 10;

    const std::string Util::kClassName("Util");

    std::mutex Util::msgdump_lock_;

    // (0) Cache names

    bool Util::isSingleNodeCache(const std::string cache_name)
    {
        if (cache_name == ARC_CACHE_NAME || cache_name == CACHELIB_CACHE_NAME || cache_name == FIFO_CACHE_NAME || cache_name == FROZENHOT_CACHE_NAME || cache_name == GLCACHE_CACHE_NAME || cache_name == GDSF_CACHE_NAME || cache_name == GDSIZE_CACHE_NAME || cache_name == LFUDA_CACHE_NAME || cache_name == LRUK_CACHE_NAME || cache_name == LFU_CACHE_NAME || cache_name == LHD_CACHE_NAME || cache_name == LRB_CACHE_NAME || cache_name == LRU_CACHE_NAME || cache_name == S3FIFO_CACHE_NAME || cache_name == SEGCACHE_CACHE_NAME || cache_name == SIEVE_CACHE_NAME || cache_name == SLRU_CACHE_NAME || cache_name == WTINYLFU_CACHE_NAME)
        {
            return true;
        }

        return false;
    }

    bool Util::isCooperativeCache(const std::string cache_name)
    {
        // NOTE: Shark and MagNet are in Shark-like and MagNet-like methods
        if (cache_name == BESTGUESS_CACHE_NAME || cache_name == COVERED_CACHE_NAME)
        {
            return true;
        }

        return false;
    }

    bool Util::isSharkLikeCache(const std::string cache_name)
    {
        if (cache_name == SHARK_EXTENDED_ARC_CACHE_NAME || cache_name == SHARK_EXTENDED_CACHELIB_CACHE_NAME || cache_name == SHARK_EXTENDED_FIFO_CACHE_NAME || cache_name == SHARK_EXTENDED_FROZENHOT_CACHE_NAME || cache_name == SHARK_EXTENDED_GLCACHE_CACHE_NAME || cache_name == SHARK_EXTENDED_GDSF_CACHE_NAME || cache_name == SHARK_EXTENDED_GDSIZE_CACHE_NAME || cache_name == SHARK_EXTENDED_LFUDA_CACHE_NAME || cache_name == SHARK_EXTENDED_LRUK_CACHE_NAME || cache_name == SHARK_EXTENDED_LFU_CACHE_NAME || cache_name == SHARK_EXTENDED_LHD_CACHE_NAME || cache_name == SHARK_EXTENDED_LRB_CACHE_NAME || cache_name == SHARK_EXTENDED_LRU_CACHE_NAME || cache_name == SHARK_EXTENDED_S3FIFO_CACHE_NAME || cache_name == SHARK_EXTENDED_SEGCACHE_CACHE_NAME || cache_name == SHARK_EXTENDED_SIEVE_CACHE_NAME || cache_name == SHARK_EXTENDED_SLRU_CACHE_NAME || cache_name == SHARK_EXTENDED_WTINYLFU_CACHE_NAME)
        {
            return true;
        }

        return false;
    }

    bool Util::isMagnetLikeCache(const std::string cache_name)
    {
        if (cache_name == MAGNET_EXTENDED_ARC_CACHE_NAME || cache_name == MAGNET_EXTENDED_CACHELIB_CACHE_NAME || cache_name == MAGNET_EXTENDED_FIFO_CACHE_NAME || cache_name == MAGNET_EXTENDED_FROZENHOT_CACHE_NAME || cache_name == MAGNET_EXTENDED_GLCACHE_CACHE_NAME || cache_name == MAGNET_EXTENDED_GDSF_CACHE_NAME || cache_name == MAGNET_EXTENDED_GDSIZE_CACHE_NAME || cache_name == MAGNET_EXTENDED_LFUDA_CACHE_NAME || cache_name == MAGNET_EXTENDED_LRUK_CACHE_NAME || cache_name == MAGNET_EXTENDED_LFU_CACHE_NAME || cache_name == MAGNET_EXTENDED_LHD_CACHE_NAME || cache_name == MAGNET_EXTENDED_LRB_CACHE_NAME || cache_name == MAGNET_EXTENDED_LRU_CACHE_NAME || cache_name == MAGNET_EXTENDED_S3FIFO_CACHE_NAME || cache_name == MAGNET_EXTENDED_SEGCACHE_CACHE_NAME || cache_name == MAGNET_EXTENDED_SIEVE_CACHE_NAME || cache_name == MAGNET_EXTENDED_SLRU_CACHE_NAME || cache_name == MAGNET_EXTENDED_WTINYLFU_CACHE_NAME)
        {
            return true;
        }

        return false;
    }

    // OBSOLETE: single-node does NOT have geo-graphical information and cannot be simply partitioned for geo-distributed access
    // Replay single-node trace files by sampling and partitioning for geo-distributed caching
    bool Util::isReplayedWorkload(const std::string workload_name)
    {
        if (workload_name == WIKIPEDIA_IMAGE_WORKLOAD_NAME || workload_name == WIKIPEDIA_TEXT_WORKLOAD_NAME)
        {
            return true;
        }

        return false;
    }

    // OBSOLETE: single-node does NOT have geo-graphical information and cannot be simply partitioned for geo-distributed access
    // Replay single-node trace files by sampling and partitioning for geo-distributed caching
    std::string Util::getReplayedWorkloadHintstr()
    {
        std::ostringstream oss;
        oss << "replayed workloads (" << WIKIPEDIA_IMAGE_WORKLOAD_NAME << " and " << WIKIPEDIA_TEXT_WORKLOAD_NAME << ")";
        return oss.str();
    }

    // (1) I/O

    // (1.1) stdout/stderr I/O

    void Util::dumpNormalMsg(const std::string& class_name, const std::string& normal_message)
    {
        std::string cur_timestr = getCurrentTimestr();
        msgdump_lock_.lock();
        std::cout << cur_timestr << " <" << class_name << "> " << normal_message << std::endl;
        msgdump_lock_.unlock();
        return;
    }

    void Util::dumpInfoMsg(const std::string& class_name, const std::string& info_message)
    {
        if (Config::isInfo())
        {
            std::string cur_timestr = getCurrentTimestr();
            msgdump_lock_.lock();
            // \033 means ESC character, 1 means bold, 34 means blue foreground, 0 means reset, and m is end character
            std::cout << "\033[1;34m" << cur_timestr << " [INFO] <" << class_name << "> " << info_message << std::endl << "\033[0m";
            msgdump_lock_.unlock();
        }
        return;
    }

    void Util::dumpDebugMsg(const std::string& class_name, const std::string& debug_message)
    {
        if (Config::isDebug())
        {
            std::string cur_timestr = getCurrentTimestr();
            msgdump_lock_.lock();
            // \033 means ESC character, 1 means bold, 32 means green foreground, 0 means reset, and m is end character
            std::cout << "\033[1;32m" << cur_timestr << " [DEBUG] <" << class_name << "> " << debug_message << std::endl << "\033[0m";
            msgdump_lock_.unlock();
        }
        return;
    }

    void Util::dumpWarnMsg(const std::string& class_name, const std::string& warn_message)
    {
        std::string cur_timestr = getCurrentTimestr();
        msgdump_lock_.lock();
        // \033 means ESC character, 1 means bold, 33 means yellow foreground, 0 means reset, and m is end character
        std::cout << "\033[1;33m" << cur_timestr << " [WARN] <" << class_name << "> " << warn_message << std::endl << "\033[0m";
        msgdump_lock_.unlock();
        return;
    }

    void Util::dumpErrorMsg(const std::string& class_name, const std::string& error_message)
    {
        std::string cur_timestr = getCurrentTimestr();
        msgdump_lock_.lock();
        // \033 means ESC character, 1 means bold, 31 means red foreground, 0 means reset, and m is end character
        std::cerr << "\033[1;31m" << cur_timestr << " [ERROR] <" << class_name << "> " << error_message << std::endl << "\033[0m" << boost::stacktrace::stacktrace();
        msgdump_lock_.unlock();
        return;
    }

    void Util::dumpVariablesForDebug(const std::string& class_name, const uint32_t count, ...)
    {
        std::va_list args;
        va_start(args, count);

        std::ostringstream oss;
        for (uint32_t i = 0; i < count; i++)
        {
            const char* tmpstr = va_arg(args, const char*);
            oss << std::string(tmpstr) << " ";
        }

        dumpDebugMsg(class_name, oss.str());
        return;
    }

    // (1.2) File I/O

    bool Util::isFileExist(const std::string& filepath, const bool& is_silent)
    {
        return isPathExist_(filepath, true, is_silent);
    }

    bool Util::isDirectoryExist(const std::string& dirpath, const bool& is_silent)
    {
        return isPathExist_(dirpath, false, is_silent);
    }

    void Util::createDirectory(const std::string& dirpath)
    {
        assert(!isDirectoryExist(dirpath, true));
        //bool result = boost::filesystem::create_directory(dirpath);
        bool result = boost::filesystem::create_directories(dirpath); // Create directory path recursively
        if (!result)
        {
            std::ostringstream oss;
            oss << "fail to create directory " << dirpath;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    std::string Util::getParentDirpath(const std::string& filepath)
    {
        boost::filesystem::path boost_filepath(filepath);
        boost::filesystem::path boost_dirpath = boost_filepath.parent_path();
        return boost_dirpath.string();
    }

    std::string Util::getFilenameFromFilepath(const std::string& filepath)
    {
        boost::filesystem::path boost_filepath(filepath);
        boost::filesystem::path boost_filename = boost_filepath.filename();
        return boost_filename.string();
    }

    bool Util::isPathExist_(const std::string& path, const bool& is_file, const bool& is_silent)
    {
        // Get boost::file_status
        boost::filesystem::path boost_path(path);
        boost::system::error_code boost_errcode;
        boost::filesystem::file_status boost_pathstatus = boost::filesystem::status(boost_path, boost_errcode);

        // An error occurs
        bool is_error = bool(boost_errcode); // boost_errcode.m_val != 0
        if (is_error && boost_errcode.value() != ENOENT)
        {
            std::ostringstream oss;
            oss << "failed to check status of file " << path << " (error code: " << boost_errcode.value() << "; error msg: " << boost_errcode.message() << ") !";
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        // File not exist
        if (!boost::filesystem::exists(boost_pathstatus))
        {
            if (!is_silent)
            {
                std::ostringstream oss;
                oss << path << " does not exist!";
                dumpWarnMsg(kClassName, oss.str());
            }
            return false;
        }

        // Judge file or directory
        if (is_file) // Should be a regular file
        {
            if (boost::filesystem::is_directory(boost_pathstatus))
            {
                std::ostringstream oss;
                oss << path << " is a directory!";
                dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }
        else // Should be a directory
        {
            if (boost::filesystem::is_regular_file(boost_pathstatus))
            {
                std::ostringstream oss;
                oss << path << " is a regular file!";
                dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        return true;
    }

    std::fstream* Util::openFile(const std::string& filepath, std::ios_base::openmode mode)
    {
        std::fstream* fs_ptr = new std::fstream(filepath.c_str(), mode);
        assert(fs_ptr != NULL);
        if (!(*fs_ptr))
        {
            std::ostringstream oss;
            oss << "fail to open statistics file " << filepath;
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return fs_ptr; // Release outside Util
    }

    int Util::openFile(const std::string& filepath, const int& flags)
    {
        int tmp_fd = open(filepath.c_str(), flags);
        if (tmp_fd < 0)
        {
            std::ostringstream oss;
            oss << "failed to open the file " << filepath << " (errno: " << errno << ") !";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return tmp_fd; // Close outside Util
    }

    void Util::closeFile(const int& fd)
    {
        int result = close(fd);
        if (result < 0)
        {
            std::ostringstream oss;
            oss << "failed to close the file descriptor " << fd << " (errno: " << errno << ") !";
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    // (2) Time measurement

    struct timespec Util::getCurrentTimespec()
    {
        struct timespec current_timespec;
        clock_gettime(CLOCK_REALTIME, &current_timespec);
        return current_timespec;
    }

    uint64_t Util::getCurrentTimeUs()
    {
        struct timespec current_timespec = getCurrentTimespec();
        double current_time_us = SEC2US(current_timespec.tv_sec) + double(current_timespec.tv_nsec) / double(1000.0);
        return static_cast<uint64_t>(current_time_us);
    }

    std::string Util::getCurrentTimestr()
    {
        // Calculate reuiqred data
        struct timespec current_timespec = getCurrentTimespec();
        struct tm* current_datetime = localtime(&(current_timespec.tv_sec));
        int year = current_datetime->tm_year + START_YEAR;
        int month = current_datetime->tm_mon + 1;
        int day = current_datetime->tm_mday;
        int hour = current_datetime->tm_hour;
        int minute = current_datetime->tm_min;
        double second = static_cast<double>(current_datetime->tm_sec);
        double nanosecond = static_cast<double>(current_timespec.tv_nsec) / static_cast<double>(NANOSECONDS_PERSECOND);
        if (nanosecond >= (double(1.0) / pow(10, static_cast<double>(SECOND_PRECISION))))
        {
            second += nanosecond;
        }

        // Dump time string in a fixed format (keep the first two digits after decimal point for second)
        std::ostringstream oss;
        std::streamsize default_precision = oss.precision();
        oss << year << "." << month << "." << day << " " << hour << ":" << minute << ":";
        oss.setf(std::ios_base::fixed);
        oss.precision(SECOND_PRECISION);
        oss << second;
        oss.unsetf(std::ios_base::fixed);
        oss.precision(default_precision);
        return oss.str();
    }

    double Util::getDeltaTimeUs(const struct timespec& current_timespec, const struct timespec& previous_timespec)
    {
        struct timespec delta_timespec;
        delta_timespec.tv_sec = current_timespec.tv_sec - previous_timespec.tv_sec;
		delta_timespec.tv_nsec = current_timespec.tv_nsec - previous_timespec.tv_nsec;
		if (delta_timespec.tv_nsec < 0)
        {
			delta_timespec.tv_sec--;
			delta_timespec.tv_nsec += NANOSECONDS_PERSECOND;
		}

        double delta_time = SEC2US(delta_timespec.tv_sec) + double(delta_timespec.tv_nsec) / double(1000.0);
        return delta_time;
    }

    // (3) Type conversion/checking

    uint16_t Util::toUint16(const int64_t& val)
    {
        if (val >= 0 && val < MAX_UINT16)
        {
            uint16_t result = static_cast<uint16_t>(val);
            return result;
        }
        else
        {
            std::ostringstream oss;
            oss << "cannot convert " << val << " (>= " << MAX_UINT16 << ") to uint16_t!";
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

    uint32_t Util::toUint32(const int64_t& val)
    {
        if (val >= 0 && val < MAX_UINT32)
        {
            uint32_t result = static_cast<uint32_t>(val);
            return result;
        }
        else
        {
            std::ostringstream oss;
            oss << "cannot convert " << val << " (>= " << MAX_UINT32 << ") to uint32_t!";
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

    uint64_t Util::uint64Add(const uint64_t& a, const uint64_t& b)
    {
        uint64_t results = a + b;
        if (results < a || results < b)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << a << " + " << b;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return results;
    }
    
    uint64_t Util::uint64Minus(const uint64_t& a, const uint64_t& b)
    {
        uint64_t results = a - b;
        if (results > a)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << a << " - " << b;
            oss << std::endl << boost::stacktrace::stacktrace();

            dumpWarnMsg(kClassName, oss.str());
            results = 0;
            
            // dumpErrorMsg(kClassName, oss.str());
            // exit(1);
        }
        return results;
    }

    void Util::uint64AddForAtomic(std::atomic<uint64_t>& a, const uint64_t& b)
    {
        uint64_t original_a = a.fetch_add(b, RMW_CONCURRENCY_ORDER);
        //uint64_t results = a.load(LOAD_CONCURRENCY_ORDER); // OBSOLETE: the value of the atomic integer a may be changed by other threads
        uint64_t results = original_a + b;
        if (results < original_a || results < b)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << original_a << " + " << b;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }
    
    void Util::uint64MinusForAtomic(std::atomic<uint64_t>& a, const uint64_t& b)
    {
        uint64_t original_a = a.fetch_sub(b, RMW_CONCURRENCY_ORDER);
        //uint64_t results = a.load(LOAD_CONCURRENCY_ORDER); // OBSOLETE: the value of the atomic integer a may be changed by other threads
        uint64_t results = original_a - b;
        if (results > original_a)
        {
            std::ostringstream oss;
            oss << "uint64_t overflow of " << original_a << " - " << b;
            oss << std::endl << boost::stacktrace::stacktrace();

            dumpWarnMsg(kClassName, oss.str());
            a.store(0, STORE_CONCURRENCY_ORDER);
            
            // dumpErrorMsg(kClassName, oss.str());
            // exit(1);
        }
        return;
    }

    Popularity Util::popularityDivide(const Popularity& a, const Popularity& b)
    {
        assertPopularity_(a);
        assertPopularity_(b);

        Popularity c = a / b;

        std::ostringstream oss;
        oss << a << " / " << b;
        assertPopularity_(c, oss.str());

        return c;
    }

    Popularity Util::popularityNonegMinus(const Popularity& a, const Popularity& b)
    {
        assertPopularity_(a);
        assertPopularity_(b);

        Popularity c = a - b;

        std::ostringstream oss;
        oss << "min(" << a << " - " << b << ", 0)";
        assertPopularity_(c, oss.str());

        if (c < 0)
        {
            c = 0.0;
        }

        assert(c >= 0);
        return c;
    }

    Popularity Util::popularityAbsMinus(const Popularity& a, const Popularity& b)
    {
        assertPopularity_(a);
        assertPopularity_(b);

        Popularity c = 0.0;
        if (a >= b)
        {
            c = a - b;
        }
        else
        {
            c = b - a;
        }

        std::ostringstream oss;
        oss << "|" << a << " - " << b << "|";
        assertPopularity_(c, oss.str());

        assert(c >= 0);
        return c;
    }

    Popularity Util::popularityMultiply(const Popularity& a, const Popularity& b)
    {
        assertPopularity_(a);
        assertPopularity_(b);

        Popularity c = a * b;

        std::ostringstream oss;
        oss << a << " * " << b;
        assertPopularity_(c, oss.str());

        return c;
    }

    Popularity Util::popularityAdd(const Popularity& a, const Popularity& b)
    {
        assertPopularity_(a);
        assertPopularity_(b);

        Popularity c = a + b;

        std::ostringstream oss;
        oss << a << " + " << b;
        assertPopularity_(c, oss.str());

        return c;
    }

    void Util::assertPopularity_(const Popularity& a, const std::string& opstr)
    {
        if (std::isnan(a) || std::isinf(a))
        {
            std::ostringstream oss;
            oss << "invalid popularity " << a;
            if (opstr != "")
            {
                oss << " got by " << opstr;
            }
            Util::dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
        return;
    }

    bool Util::isApproxLargerEqual(const double& a, const double& b)
    {
        if (a > b)
        {
            return true;
        }
        else if (Util::isApproxEqual(a, b))
        {
            return true;
        }
        return false;
    }

    bool Util::isApproxLargerEqual(const float& a, const float& b)
    {
        return isApproxLargerEqual(static_cast<double>(a), static_cast<double>(b));
    }

    bool Util::isApproxEqual(const double& a, const double& b)
    {
        if (a >= b)
        {
            if (a - b <= DOUBLE_IOTA)
            {
                return true;
            }
        }
        else
        {
            if (b - a <= DOUBLE_IOTA)
            {
                return true;
            }
        }
        return false;
    }

    bool Util::isApproxEqual(const float& a, const float& b)
    {
        return isApproxEqual(static_cast<double>(a), static_cast<double>(b));
    }

    std::string Util::toString(void* pointer)
    {
        return std::to_string(reinterpret_cast<intptr_t>(pointer));
    }

    std::string Util::toString(const bool& boolean)
    {
        return boolean?"true":"false";
    }

    std::string Util::toString(const uint32_t& val)
    {
        return std::to_string(val);
    }

    // (4) Client-edge-cloud scenario

    // (4.1) Client

    uint16_t Util::getClientRecvmsgPort(const uint32_t& client_idx, const uint32_t& clientcnt)
    {
        int64_t client_recvmsg_startport = static_cast<int64_t>(Config::getClientRecvmsgStartport());
        const uint16_t client_recvmsg_port = getNodePort_(client_recvmsg_startport, client_idx, clientcnt, Config::getClientMachineCnt());
        Config::portVerification(static_cast<uint16_t>(client_recvmsg_startport), client_recvmsg_port);

        return client_recvmsg_port;
    }

    uint32_t Util::getClosestEdgeIdx(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt)
    {
        assert(edgecnt > 0);
        uint32_t peredge_clientcnt = clientcnt / edgecnt;
        assert(peredge_clientcnt > 0);
        uint32_t closest_edge_idx = client_idx / peredge_clientcnt;
        if (closest_edge_idx >= edgecnt)
        {
            closest_edge_idx = edgecnt - 1; // Assign tail clients to the last edge node
        }
        return closest_edge_idx;
    }

    std::string Util::getClosestEdgeIpstr(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt)
    {
        const bool is_private_client_ipstr = true; // NOTE: client communicates with the closest edge via public IP address
        const bool is_launch_edge = false; // Just connect the closest edge node by the logical client edge node instead of launching the closest edge node
        uint32_t closest_edge_idx = getClosestEdgeIdx(client_idx, clientcnt, edgecnt);
        return Config::getEdgeIpstr(closest_edge_idx, edgecnt, is_private_client_ipstr, is_launch_edge);
    }

    uint16_t Util::getClosestEdgeCacheServerRecvreqPort(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& edgecnt, uint32_t& closest_edge_idx)
    {
        closest_edge_idx = getClosestEdgeIdx(client_idx, clientcnt, edgecnt);
        return getEdgeCacheServerRecvreqPort(closest_edge_idx, edgecnt);
    }

    uint32_t Util::getGlobalClientWorkerIdx(const uint32_t& client_idx, const uint32_t& local_client_worker_idx, const uint32_t& perclient_workercnt)
    {
        uint32_t global_client_worker_idx = client_idx * perclient_workercnt + local_client_worker_idx;
        return global_client_worker_idx;
    }

    /*void Util::parseGlobalClientWorkerIdx(const uint32_t& global_client_worker_idx, const uint32_t& perclient_workercnt, uint32_t& client_idx, uint32_t& local_client_worker_idx)
    {
        client_idx = global_client_worker_idx / perclient_workercnt;
        local_client_worker_idx = global_client_worker_idx - client_idx * perclient_workercnt;
        assert(local_client_worker_idx < perclient_workercnt);
        return;
    }*/

    uint16_t Util::getClientWorkerRecvrspPort(const uint32_t& client_idx, const uint32_t& clientcnt, const uint32_t& local_client_worker_idx, const uint32_t& perclient_workercnt)
    {
        assert(local_client_worker_idx < perclient_workercnt);

        // Get client recvrsp port if 1 worker per client
        int64_t client_worker_recvrsp_startport = static_cast<int64_t>(Config::getClientWorkerRecvrspStartport());
        int64_t client_recvrsp_port = static_cast<int64_t>(getNodePort_(client_worker_recvrsp_startport, client_idx, clientcnt, Config::getClientMachineCnt()));
        assert(client_recvrsp_port >= client_worker_recvrsp_startport);

        // Get client worker recvrsp port
        const uint16_t client_worker_recvrsp_port = Util::toUint16(client_worker_recvrsp_startport + (client_recvrsp_port - client_worker_recvrsp_startport) * static_cast<int64_t>(perclient_workercnt) + static_cast<int64_t>(local_client_worker_idx));
        Config::portVerification(static_cast<uint16_t>(client_worker_recvrsp_startport), client_worker_recvrsp_port);
        
        return client_worker_recvrsp_port;
    }

    // (4.2) Edge

    uint16_t Util::getEdgeRecvmsgPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_recvmsg_startport = static_cast<int64_t>(Config::getEdgeRecvmsgStartport());
        const uint16_t edge_recvmsg_port = getNodePort_(edge_recvmsg_startport, edge_idx, edgecnt, Config::getEdgeMachineCnt());
        Config::portVerification(static_cast<uint16_t>(edge_recvmsg_startport), edge_recvmsg_port);

        return edge_recvmsg_port;
    }

    uint16_t Util::getEdgeBeaconServerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_beacon_server_recvreq_startport = static_cast<int64_t>(Config::getEdgeBeaconServerRecvreqStartport());
        const uint16_t edge_beacon_server_recvreq_port = getNodePort_(edge_beacon_server_recvreq_startport, edge_idx, edgecnt, Config::getEdgeMachineCnt());
        Config::portVerification(static_cast<uint16_t>(edge_beacon_server_recvreq_startport), edge_beacon_server_recvreq_port);

        return edge_beacon_server_recvreq_port;
    }

    uint16_t Util::getEdgeBeaconServerRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_beacon_server_recvrsp_startport = static_cast<int64_t>(Config::getEdgeBeaconServerRecvrspStartport());
        const uint16_t edge_beacon_server_recvrsp_port = getNodePort_(edge_beacon_server_recvrsp_startport, edge_idx, edgecnt, Config::getEdgeMachineCnt());
        Config::portVerification(static_cast<uint16_t>(edge_beacon_server_recvrsp_startport), edge_beacon_server_recvrsp_port);

        return edge_beacon_server_recvrsp_port;
    }

    uint16_t Util::getEdgeCacheServerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_cache_server_recvreq_startport = static_cast<int64_t>(Config::getEdgeCacheServerRecvreqStartport());
        const uint16_t edge_cache_server_recvreq_port = getNodePort_(edge_cache_server_recvreq_startport, edge_idx, edgecnt, Config::getEdgeMachineCnt());
        Config::portVerification(static_cast<uint16_t>(edge_cache_server_recvreq_startport), edge_cache_server_recvreq_port);

        return edge_cache_server_recvreq_port;
    }

    uint16_t Util::getEdgeCacheServerPlacementProcessorRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt)
    {
        int64_t edge_cache_server_placement_processor_recvrsp_startport = static_cast<int64_t>(Config::getEdgeCacheServerPlacementProcessorRecvrspStartport());
        const uint16_t edge_cache_server_placement_processor_recvrsp_port = getNodePort_(edge_cache_server_placement_processor_recvrsp_startport, edge_idx, edgecnt, Config::getEdgeMachineCnt());
        Config::portVerification(static_cast<uint16_t>(edge_cache_server_placement_processor_recvrsp_startport), edge_cache_server_placement_processor_recvrsp_port);

        return edge_cache_server_placement_processor_recvrsp_port;
    }

    NetworkAddr Util::getEdgeCacheServerWorkerRecvreqAddrFromRecvrspAddr(const NetworkAddr& edge_cache_server_worker_recvrsp_addr)
    {
        std::string edge_ipstr = edge_cache_server_worker_recvrsp_addr.getIpstr();

        const uint16_t edge_cache_server_worker_recvrsp_port = edge_cache_server_worker_recvrsp_addr.getPort();
        const uint16_t edge_cache_server_worker_recvrsp_startport = Config::getEdgeCacheServerWorkerRecvrspStartport();
        assert(edge_cache_server_worker_recvrsp_port >= edge_cache_server_worker_recvrsp_startport);

        const uint16_t edge_cache_server_worker_recvreq_startport = Config::getEdgeCacheServerWorkerRecvreqStartport();
        const uint16_t edge_cache_server_worker_recvreq_port = edge_cache_server_worker_recvrsp_port - edge_cache_server_worker_recvrsp_startport + edge_cache_server_worker_recvreq_startport;
        Config::portVerification(edge_cache_server_worker_recvreq_startport, edge_cache_server_worker_recvreq_port);

        NetworkAddr edge_cache_server_worker_recvreq_addr(edge_ipstr, edge_cache_server_worker_recvreq_port);

        return edge_cache_server_worker_recvreq_addr;
    }

    uint32_t Util::getEdgeIdxFromCacheServerWorkerRecvreqAddr(const NetworkAddr& edge_cache_server_worker_recvreq_addr, const bool& is_private_ipstr, const uint32_t& edgecnt)
    {
        std::string edge_ipstr = edge_cache_server_worker_recvreq_addr.getIpstr();
        uint32_t local_edge_machine_idx = Config::getEdgeLocalMachineIdxByIpstr(edge_ipstr, is_private_ipstr);
        uint32_t edge_machine_cnt = Config::getEdgeMachineCnt();
        assert(local_edge_machine_idx < edge_machine_cnt);

        const uint16_t edge_cache_server_worker_recvreq_port = edge_cache_server_worker_recvreq_addr.getPort();
        const uint16_t edge_cache_server_worker_recvreq_startport = Config::getEdgeCacheServerWorkerRecvreqStartport();
        assert(edge_cache_server_worker_recvreq_port >= edge_cache_server_worker_recvreq_startport);
        uint32_t local_edge_idx = edge_cache_server_worker_recvreq_port - edge_cache_server_worker_recvreq_startport;

        uint32_t permachine_edgecnt = edgecnt / edge_machine_cnt;
        assert(permachine_edgecnt > 0);
        if (local_edge_machine_idx < edge_machine_cnt - 1)
        {
            assert(local_edge_idx < permachine_edgecnt);
        }
        uint32_t global_edge_idx = local_edge_machine_idx * permachine_edgecnt + local_edge_idx;

        return global_edge_idx;
    }

    uint16_t Util::getEdgeCacheServerWorkerRecvreqPort(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& local_cache_server_worker_idx, const uint32_t& percacheserver_workercnt)
    {
        int64_t edge_cache_server_worker_recvreq_startport = static_cast<int64_t>(Config::getEdgeCacheServerWorkerRecvreqStartport());
        int64_t edge_cache_server_recvreq_port = static_cast<int64_t>(getNodePort_(edge_cache_server_worker_recvreq_startport, edge_idx, edgecnt, Config::getEdgeMachineCnt()));

        // Get cache server worker recvreq port
        const uint16_t cache_server_worker_recvreq_port = Util::toUint16(edge_cache_server_worker_recvreq_startport + (edge_cache_server_recvreq_port - edge_cache_server_worker_recvreq_startport) * static_cast<int64_t>(percacheserver_workercnt) + static_cast<int64_t>(local_cache_server_worker_idx));
        Config::portVerification(static_cast<uint16_t>(edge_cache_server_worker_recvreq_startport), cache_server_worker_recvreq_port);

        return cache_server_worker_recvreq_port;
    }

    uint16_t Util::getEdgeCacheServerWorkerRecvrspPort(const uint32_t& edge_idx, const uint32_t& edgecnt, const uint32_t& local_cache_server_worker_idx, const uint32_t& percacheserver_workercnt)
    {
        int64_t edge_cache_server_worker_recvrsp_startport = static_cast<int64_t>(Config::getEdgeCacheServerWorkerRecvrspStartport());
        int64_t edge_cache_server_recvrsp_port = static_cast<int64_t>(getNodePort_(edge_cache_server_worker_recvrsp_startport, edge_idx, edgecnt, Config::getEdgeMachineCnt()));

        // Get cache server worker recvreq port
        const uint16_t cache_server_worker_recvrsp_port = Util::toUint16(edge_cache_server_worker_recvrsp_startport + (edge_cache_server_recvrsp_port - edge_cache_server_worker_recvrsp_startport) * static_cast<int64_t>(percacheserver_workercnt) + static_cast<int64_t>(local_cache_server_worker_idx));
        Config::portVerification(static_cast<uint16_t>(edge_cache_server_worker_recvrsp_startport), cache_server_worker_recvrsp_port);

        return cache_server_worker_recvrsp_port;
    }

    // (4.3) Cloud

    uint16_t Util::getCloudRecvmsgPort(const uint32_t& cloud_idx)
    {
        // TODO: only support 1 cloud node now
        assert(cloud_idx == 0);

        int64_t cloud_recvmsg_startport = static_cast<int64_t>(Config::getCloudRecvmsgStartport());
        const uint16_t cloud_recvmsg_port = getNodePort_(cloud_recvmsg_startport, cloud_idx, 1, 1);
        Config::portVerification(static_cast<uint16_t>(cloud_recvmsg_startport), cloud_recvmsg_port);

        return cloud_recvmsg_port;
    }

    uint16_t Util::getCloudRecvreqPort(const uint32_t& cloud_idx)
    {
        // TODO: only support 1 cloud node now
        assert(cloud_idx == 0);

        int64_t cloud_recvreq_startport = static_cast<int64_t>(Config::getCloudRecvreqStartport());
        const uint16_t cloud_recvreq_port = getNodePort_(cloud_recvreq_startport, cloud_idx, 1, 1);
        Config::portVerification(static_cast<uint16_t>(cloud_recvreq_startport), cloud_recvreq_port);

        return cloud_recvreq_port;
    }

    uint16_t Util::getNodePort_(const int64_t& start_port, const uint32_t& node_idx, const uint32_t& nodecnt, const uint32_t& machine_cnt)
    {
        int64_t node_port = 0;

        assert(machine_cnt > 0);
        uint32_t permachine_nodecnt = nodecnt / machine_cnt;
        assert(permachine_nodecnt > 0);
        uint32_t machine_idx = node_idx / permachine_nodecnt;

        uint32_t local_node_idx = 0;
        if (machine_idx < machine_cnt)
        {
            local_node_idx = node_idx - machine_idx * permachine_nodecnt;
            assert(local_node_idx < permachine_nodecnt);
        }
        else
        {
            local_node_idx = node_idx - (machine_cnt - 1) * permachine_nodecnt;
            assert(local_node_idx >= permachine_nodecnt);
        }

        node_port = start_port + local_node_idx;
        
        return Util::toUint16(node_port);
    }

    // (5) Network

    uint32_t Util::getFragmentCnt(const uint32_t& msg_payload_size)
    {
        // Split message payload into multiple fragment payloads
        uint32_t max_fragment_payload = Util::UDP_MAX_FRAG_PAYLOAD;
        uint32_t fragment_cnt = 0;
        if (msg_payload_size == 0)
        {
            fragment_cnt = 1;
        }
        else
        {
            assert(max_fragment_payload > 0);
            fragment_cnt = (msg_payload_size - 1) / max_fragment_payload + 1;
        }
        return fragment_cnt;
    }

    uint32_t Util::getFragmentOffset(const uint32_t& fragment_idx)
    {
        return fragment_idx * UDP_MAX_FRAG_PAYLOAD;
    }

    uint32_t Util::getFragmentPayloadSize(const uint32_t& fragment_idx, const uint32_t& msg_payload_size)
    {
        uint32_t fragment_payload_size = UDP_MAX_FRAG_PAYLOAD;
        uint32_t fragment_cnt = getFragmentCnt(msg_payload_size);
        if (fragment_idx == fragment_cnt - 1)
        {
            fragment_payload_size = msg_payload_size -  (fragment_cnt - 1) * UDP_MAX_FRAG_PAYLOAD;
        }
        assert(fragment_payload_size >= 0);
        assert(fragment_payload_size <= UDP_MAX_FRAG_PAYLOAD);
        return fragment_payload_size;
    }

    std::string Util::getAckedStatusStr(const std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>& acked_flags)
    {
        std::ostringstream oss;
        for (std::unordered_map<NetworkAddr, std::pair<bool, std::string>, NetworkAddrHasher>::const_iterator iter = acked_flags.begin(); iter != acked_flags.end(); iter++)
        {
            oss << iter->second.second << " (" << Util::toString(iter->second.first) << ") ";
        }
        return oss.str();
    }

    std::string Util::getAckedStatusStr(const std::unordered_map<NetworkAddr, std::pair<bool, uint32_t>, NetworkAddrHasher>& acked_flags, const std::string& rolestr)
    {
        std::ostringstream oss;
        for (std::unordered_map<NetworkAddr, std::pair<bool, uint32_t>, NetworkAddrHasher>::const_iterator iter = acked_flags.begin(); iter != acked_flags.end(); iter++)
        {
            oss << rolestr << " " << std::to_string(iter->second.second) << " (" << Util::toString(iter->second.first) << ") ";
        }
        return oss.str();
    }

    std::string Util::getAckedStatusStr(const std::unordered_map<uint32_t, bool>& acked_flags, const std::string& rolestr)
    {
        std::ostringstream oss;
        for (std::unordered_map<uint32_t, bool>::const_iterator iter = acked_flags.begin(); iter != acked_flags.end(); iter++)
        {
            oss << rolestr << " " << iter->first << " (" << Util::toString(iter->second) << ") ";
        }
        return oss.str();
    }

    std::string Util::getAckedStatusStr(const std::unordered_map<Key, std::pair<bool, uint32_t>, KeyHasher>& acked_flags, const std::string& rolestr)
    {
        std::ostringstream oss;
        for (std::unordered_map<Key, std::pair<bool, uint32_t>, KeyHasher>::const_iterator iter = acked_flags.begin(); iter != acked_flags.end(); iter++)
        {
            oss << rolestr << " " << std::to_string(iter->second.second) << " (" << Util::toString(iter->second.first) << ") ";
        }
        return oss.str();
    }

    // Propagation latency simulation

    uint32_t Util::getPropagationSimulationRandomSeedForClient(const uint32_t& node_idx, const uint32_t& local_propagation_simulator_idx)
    {
        return getPropagationSimulationRandomSeed_(node_idx, local_propagation_simulator_idx, PROPAGATION_SIMULATION_CLIENT_SEED_BASE, PROPAGATION_SIMULATION_CLIENT_SEED_FACTOR);
    }

    uint32_t Util::getPropagationSimulationRandomSeedForEdge(const uint32_t& node_idx, const uint32_t& local_propagation_simulator_idx)
    {
        return getPropagationSimulationRandomSeed_(node_idx, local_propagation_simulator_idx, PROPAGATION_SIMULATION_EDGE_SEED_BASE, PROPAGATION_SIMULATION_EDGE_SEED_FACTOR);
    }

    uint32_t Util::getPropagationSimulationRandomSeedForCloud(const uint32_t& node_idx, const uint32_t& local_propagation_simulator_idx)
    {
        return getPropagationSimulationRandomSeed_(node_idx, local_propagation_simulator_idx, PROPAGATION_SIMULATION_CLOUD_SEED_BASE, PROPAGATION_SIMULATION_CLOUD_SEED_FACTOR);
    }

    uint32_t Util::getPropagationSimulationRandomSeed_(const uint32_t& node_idx, const uint32_t& local_propagation_simulator_idx, const uint32_t& seed_base, const uint32_t& seed_factor)
    {
        uint32_t random_seed = seed_base + node_idx * seed_factor + local_propagation_simulator_idx;
        return random_seed;
    }

    // (6) Intermediate files

    std::string Util::getSampledDatasetFilepath(const std::string& workload_name)
    {
        const uint32_t tmp_trace_sample_opcnt = Config::getTraceSampleOpcnt(workload_name);

        // NOTE: MUST be the same as dataset filepath in scripts/tools/preprocess_traces.py
        const std::string tmp_dirpath = Config::getTraceDirpath();
        const std::string tmp_dataset_filepath = tmp_dirpath + "/" + workload_name + ".dataset." + std::to_string(tmp_trace_sample_opcnt); // E.g., data/wikitext.dataset.1000000
        return tmp_dataset_filepath;
    }

    std::string Util::getSampledWorkloadFilepath(const std::string& workload_name)
    {
        const uint32_t tmp_trace_sample_opcnt = Config::getTraceSampleOpcnt(workload_name);

        // NOTE: MUST be the same as worload filepath in scripts/tools/preprocess_traces.py
        const std::string tmp_dirpath = Config::getTraceDirpath();
        const std::string tmp_workload_filepath = tmp_dirpath + "/" + workload_name + ".workload." + std::to_string(tmp_trace_sample_opcnt); // E.g., data/wikitext.workload.1000000
        return tmp_workload_filepath;
    }

    std::string Util::getEvaluatorStatisticsDirpath(EvaluatorCLI* evaluator_cli_ptr)
    {
        assert(evaluator_cli_ptr != NULL);

        std::ostringstream oss;
        oss << Config::getOutputDirpath() << "/statistics/" << getInfixForEvaluatorStatisticsFilepath_(evaluator_cli_ptr);
        std::string client_statistics_dirpath = oss.str();
        return client_statistics_dirpath;
    }

    /*std::string Util::getClientStatisticsFilepath(const uint32_t& client_idx)
    {
        std::ostringstream oss;
        oss << getStatisticsDirpath() << "/client" << client_idx << "_statistics.out";
        std::string client_statistics_filepath = oss.str();
        return client_statistics_filepath;
    }*/

    std::string Util::getEvaluatorStatisticsFilepath(EvaluatorCLI* evaluator_cli_ptr)
    {
        assert(evaluator_cli_ptr != NULL);

        std::ostringstream oss;
        oss << getEvaluatorStatisticsDirpath(evaluator_cli_ptr) << "/evaluator_statistics.out";
        std::string evaluator_statistics_filepath = oss.str();
        return evaluator_statistics_filepath;
    }

    std::string Util::getCloudRocksdbBasedirForWorkload(const uint32_t& keycnt, const std::string& workload_name)
    {
        assert(keycnt > 0);

        // Example: /tmp/key100000_facebook
        std::ostringstream oss;
        oss << Config::getCloudRocksdbBasedir() << "/key" << keycnt << "_" << workload_name;

        return oss.str();
    }

    std::string Util::getCloudRocksdbDirpath(const uint32_t& keycnt, const std::string& workload_name, const uint32_t& cloud_idx)
    {
        // TODO: only support 1 cloud node now!
        assert(cloud_idx == 0);

        // NOTE: MUST be consistent with scripts/exps/load_dataset.py
        std::ostringstream oss;
        oss << getCloudRocksdbBasedirForWorkload(keycnt, workload_name) << "/cloud" << cloud_idx << ".db";
        std::string cloud_rocksdb_dirpath = oss.str();
        return cloud_rocksdb_dirpath;
    }

    std::string Util::getEdgeSnapshotDirpath()
    {
        std::ostringstream oss;
        oss << Config::getOutputDirpath() << "/snapshot";
        std::string edge_snapshot_dirpath = oss.str();
        return edge_snapshot_dirpath;
    }
    
    std::string Util::getEdgeSnapshotFilepath(const std::string& realnet_expname, const uint32_t& edgeidx)
    {
        assert(realnet_expname != "");

        std::ostringstream oss;
        oss << getEdgeSnapshotDirpath() << "/" << realnet_expname << "_edge" << edgeidx << ".snapshot";
        std::string edge_snapshot_filepath = oss.str();
        return edge_snapshot_filepath;
    }

    std::string Util::getInfixForEvaluatorStatisticsFilepath_(EvaluatorCLI* evaluator_cli_ptr)
    {
        assert(evaluator_cli_ptr != NULL);

        const std::string cache_name = evaluator_cli_ptr->getCacheName();
        const uint64_t capacity_bytes = evaluator_cli_ptr->getCapacityBytes();
        const uint32_t clientcnt = evaluator_cli_ptr->getClientcnt();
        const bool is_warmup_speedup = evaluator_cli_ptr->isWarmupSpeedup();
        const uint32_t edgecnt = evaluator_cli_ptr->getEdgecnt();
        const std::string hash_name = evaluator_cli_ptr->getHashName();
        const uint32_t keycnt = evaluator_cli_ptr->getKeycnt();
        const uint32_t perclient_opcnt = evaluator_cli_ptr->getPerclientOpcnt();
        const uint32_t percacheserver_workercnt = evaluator_cli_ptr->getPercacheserverWorkercnt();
        const uint32_t perclient_workercnt = evaluator_cli_ptr->getPerclientWorkercnt();
        // NOTE: using avg latencies is sufficient to distinguish different WAN settings
        const CLILatencyInfo cli_latency_info = evaluator_cli_ptr->getCLILatencyInfo();
        const uint32_t propagation_latency_clientedge_avg_us = cli_latency_info.getPropagationLatencyClientedgeAvgUs();
        const uint32_t propagation_latency_crossedge_avg_us = cli_latency_info.getPropagationLatencyCrossedgeAvgUs();
        const uint32_t propagation_latency_edgecloud_avg_us = cli_latency_info.getPropagationLatencyEdgecloudAvgUs();
        const uint32_t warmup_reqcnt_scale = evaluator_cli_ptr->getWarmupReqcntScale();
        const uint32_t warmup_max_duration_sec = evaluator_cli_ptr->getWarmupMaxDurationSec();
        const uint32_t stresstest_duration_sec = evaluator_cli_ptr->getStresstestDurationSec();
        const std::string workload_name = evaluator_cli_ptr->getWorkloadName();
        
        // ONLY used by COVERED
        uint64_t local_uncached_capacitymb = B2MB(evaluator_cli_ptr->getCoveredLocalUncachedMaxMemUsageBytes());
        const uint32_t covered_peredge_synced_victimcnt = evaluator_cli_ptr->getCoveredPeredgeSyncedVictimcnt();
        const uint32_t covered_peredge_monitored_victimsetcnt = evaluator_cli_ptr->getCoveredPeredgeMonitoredVictimsetcnt();
        uint64_t popularity_aggregation_capacitymb = B2MB(evaluator_cli_ptr->getCoveredPopularityAggregationMaxMemUsageBytes());
        double popularity_collection_change_ratio = evaluator_cli_ptr->getCoveredPopularityCollectionChangeRatio();
        uint32_t topk_edgecnt = evaluator_cli_ptr->getCoveredTopkEdgecnt();

        std::ostringstream oss;
        // NOTE: NOT consider variables which do NOT affect evaluation results
        // (i) Config variables from JSON, e.g., is_debug and is_track_event
        // (ii) Config variables from CLI, e.g., main_class_name and --config_filepath
        // (iii) Unchanged CLI parameters, e.g., --cloud_storage
        // Example (tuned params/rarely changed params/covered params):
        // Tunable params: covered_capacitymb1000_clientcnt1_edgecnt1_keycnt1000000_propagationus100010000100000_stresstestdurationsec10_facebook
        // Rarely changed params: /warmupspeedup1_hashnamemmh3_perclientopcnt1000000_percacheserverworkercnt1_perclientworkercnt1_warmupscale10_warmupmaxdurationsec0
        // 
        oss << cache_name << "_capacitymb" << B2MB(capacity_bytes) << "_clientcnt" << clientcnt << "_edgecnt" << edgecnt << "_keycnt" << keycnt << "_propagationus" << propagation_latency_clientedge_avg_us << propagation_latency_crossedge_avg_us << propagation_latency_edgecloud_avg_us << "_stresstestdurationsec" << stresstest_duration_sec << "_" << workload_name;
        oss << "/warmupspeedup" << (is_warmup_speedup?"1":"0")  << "_hashname" << hash_name << "_perclientopcnt" << perclient_opcnt << "_percacheserverworkercnt" << percacheserver_workercnt << "_perclientworkercnt" << perclient_workercnt << "_warmupscale" << warmup_reqcnt_scale << "_warmupmaxdurationsec" << warmup_max_duration_sec;
        // // (OBSOLETE due to too long filename) Example: covered_capacitymb1000_clientcnt1_warmupspeedup1_edgecnt1_hashnamemmh3_keycnt1000000_perclientopcnt1000000_percacheserverworkercnt1_perclientworkercnt1_propagationus100010000100000_warmupscale5_warmupmaxdurationsec0_stresstestdurationsec10_facebook
        // oss << cache_name << "_capacitymb" << B2MB(capacity_bytes) << "_clientcnt" << clientcnt << "_warmupspeedup" << (is_warmup_speedup?"1":"0") << "_edgecnt" << edgecnt << "_hashname" << hash_name << "_keycnt" << keycnt << "_perclientopcnt" << perclient_opcnt << "_percacheserverworkercnt" << percacheserver_workercnt << "_perclientworkercnt" << perclient_workercnt << "_propagationus" << propagation_latency_clientedge_avg_us << propagation_latency_crossedge_avg_us << propagation_latency_edgecloud_avg_us << "_warmupscale" << warmup_reqcnt_scale << "_warmupmaxdurationsec" << warmup_max_duration_sec << "_stresstestdurationsec" << stresstest_duration_sec << "_" << workload_name;
        if (cache_name == "covered")
        {
            // Example: XXX/localuncachedcapacitymb1_peredgesyncedvictimcnt3_peredgemonitoredsetcnt3_popaggregationcapacitymb1_popcollectchangeratio0.0_topkedgecnt3
            oss << "/localuncachedcapacitymb" << local_uncached_capacitymb << "_peredgesyncedvictimcnt" << covered_peredge_synced_victimcnt << "peredgemonitoredsetcnt" << covered_peredge_monitored_victimsetcnt << "_popaggregationcapacitymb" << popularity_aggregation_capacitymb << "_popcollectchangeratio" << popularity_collection_change_ratio << "_topkedgecnt" << topk_edgecnt;
        }
        std::string infixstr = oss.str();
        return infixstr;
    }

    // (7) System settings
    uint32_t Util::getNetCoreRmemMax()
    {
        std::string cmd = "sysctl -a 2>/dev/null | grep net.core.rmem_max";

        // Execute cmd
        FILE* stream = popen(cmd.c_str(), "r");
        if (stream == NULL)
        {
            dumpErrorMsg(kClassName, "popen() failed!");
            exit(1);
        }

        // Get cmd output string
        std::string output_str = "";
        try
        {
            const uint32_t tmp_buffer_bytecnt = 128;
            char tmp_buffer[tmp_buffer_bytecnt];
            while (fgets(tmp_buffer, tmp_buffer_bytecnt, stream) != NULL)
            {
                output_str += tmp_buffer;
                memset(tmp_buffer, 0, tmp_buffer_bytecnt);
            }

            pclose(stream);
            stream = NULL;
        }
        catch (...)
        {
            pclose(stream);
            stream = NULL;
            
            dumpErrorMsg(kClassName, "getNetCoreRmemMax() failed!");
            exit(1);
        }

        // Extract net.core.rmem_max value from output string
        size_t last_space_pos = output_str.find_last_of(' ');
        if (last_space_pos != std::string::npos)
        {
            std::string rmem_max_str = output_str.substr(last_space_pos + 1);
            uint32_t rmem_max = std::stoi(rmem_max_str);
            return rmem_max;
        }
        else
        {
            std::ostringstream oss;
            oss << "fail to get net.core.rmem_max value from output string: " << output_str;
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }

        return 0;
    }

    // (8) Others

    uint32_t Util::getTimeBasedRandomSeed()
    {
        unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
        return static_cast<uint32_t>(seed);
    }

    std::string Util::getRandomString(const uint32_t& length)
    {
        if (length == 0)
        {
            return "";
        }

        std::string random_string(length, '0');
        for (uint32_t i = 0; i < length; i++)
        {
            uint32_t random_index = string_randdist_(string_randgen_);
            assert(random_index < CHARSET.length());
            random_string[i] = CHARSET[random_index];
        }
        return random_string;
    }
}