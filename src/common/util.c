#include "common/util.h"

#include <iostream> // cerr
#include <sstream> // ostringstream
#include <cmath> // pow

#include <boost/filesystem.hpp>
#include <boost/system.hpp>

#include "common/config.h"
#include "common/param.h"

namespace covered
{
    // Type conversion
    const int64_t Util::MAX_UINT16 = 65536;
    // Network
    const std::string Util::LOCALHOST_IPSTR("127.0.0.1");
    const std::string Util::ANY_IPSTR("0.0.0.0");
    const uint32_t Util::UDP_MAX_PKT_PAYLOAD = 65507; // 65535(ipmax) - 20(iphdr) - 8(udphdr)
    const uint32_t Util::UDP_FRAGHDR_SIZE = 4 * sizeof(uint32_t); // 4(fragment_idx) + 4(fragment_cnt) + 4(msg_payload_size) + 4(msg_seqnum)
    const uint32_t Util::UDP_MAX_FRAG_PAYLOAD = Util::UDP_MAX_PKT_PAYLOAD - Util::UDP_FRAGHDR_SIZE;
    const uint16_t Util::UDP_MAX_PORT = 4096; // UDP port has to be larger than 4096
    // Atomicity
    std::memory_order Util::LOAD_CONCURRENCY_ORDER = std::memory_order_acquire;
    std::memory_order Util::STORE_CONCURRENCY_ORDER = std::memory_order_release;
    // Workflow control
    const unsigned int Util::SLEEP_INTERVAL_US = 1 * 1000 * 1000; // 1s
    // Workload generation
    const uint32_t Util::KVPAIR_GENERATION_SEED = 0;
    // Time measurement
    const int Util::START_YEAR = 1900;
    const long Util::NANOSECONDS_PERSECOND = 1000000000L;
    const uint32_t Util::SECOND_PRECISION = 2;

    const std::string Util::kClassName("Util");

    // I/O

    void Util::dumpNormalMsg(const std::string& class_name, const std::string& normal_message)
    {
        std::string cur_timestr = getCurrentTimespec();
        std::cout << cur_timestr << " " << class_name << ": " << normal_message << std::endl;
        return;
    }

    void Util::dumpDebugMsg(const std::string& class_name, const std::string& debug_message)
    {
        std::string cur_timestr = getCurrentTimespec();
        // \033 means ESC character, 1 means bold, 32 means green foreground, 0 means reset, and m is end character
        std::cout << "\033[1;32m" << cur_timestr << " [DEBUG] " << class_name << ": " << debug_message << std::endl << "\033[0m";
        return;
    }

    void Util::dumpWarnMsg(const std::string& class_name, const std::string& warn_message)
    {
        std::string cur_timestr = getCurrentTimespec();
        // \033 means ESC character, 1 means bold, 33 means yellow foreground, 0 means reset, and m is end character
        std::cout << "\033[1;33m" << cur_timestr << " [WARN] " << class_name << ": " << warn_message << std::endl << "\033[0m";
        return;
    }

    void Util::dumpErrorMsg(const std::string& class_name, const std::string& error_message)
    {
        std::string cur_timestr = getCurrentTimespec();
        // \033 means ESC character, 1 means bold, 31 means red foreground, 0 means reset, and m is end character
        std::cerr << "\033[1;31m" << cur_timestr << " [ERROR] " << class_name << ": " << error_message << std::endl << "\033[0m";
        return;
    }

    bool Util::isFileExist(const std::string& filepath)
    {
        // Get boost::file_status
        boost::filesystem::path boost_filepath(filepath);
        boost::system::error_code boost_errcode;
        boost::filesystem::file_status boost_filestatus = boost::filesystem::status(boost_filepath, boost_errcode);

        // An error occurs
        bool is_error = bool(boost_errcode); // boost_errcode.m_val != 0
        if (is_error)
        {
            dumpWarnMsg(kClassName, boost_errcode.message());
            return false;
        }

        // File not exist
        if (!boost::filesystem::exists(boost_filestatus))
        {
            std::ostringstream oss;
            oss << filepath << " does not exist!";
            dumpWarnMsg(kClassName, oss.str());
            return false;
        }

        // Is a directory
        if (boost::filesystem::is_directory(boost_filestatus))
        {
            std::ostringstream oss;
            oss << filepath << " is a directory!";
            dumpWarnMsg(kClassName, oss.str());
            return false;
        }

        return true;
    }

    // Time measurement

    struct timespec Util::getCurrentTimespec()
    {
        struct timespec current_timespec;
        clock_gettime(CLOCK_REALTIME, &current_timespec);
        return current_timespec;
    }

    std::string Util::getCurrentTimestr()
    {
        // Calculate reuiqred data
        struct timespec current_timespec = getCurrentTimespec();
        struct tm* current_datetime = localtime(current_timespec.tv_sec);
        int year = current_datetime.tm_year + START_YEAR;
        int month = current_datetime.tm_month + 1;
        int day = current_datetime.tm_day;
        int hour = current_datetime.tm_hour;
        int minute = current_datetime.tm_min;
        double second = static_cast<double>(current_datetime.tm_sec);
        double nanosecond = static_cast<double>(current_datetime.tv_nsec) / static_cast<double>(NANOSECONDS_PERSECOND);
        if (nanosecond >= (1.0d / pow(10, static_cast<double>(SECOND_PRECISION))))
        {
            second += nanosecond;
        }

        // Dump time string in a fixed format (keep the first two digits after decimal point for second)
        std::ostringstream oss;
        std::streamsize default_precision = oss.precision();
        oss << year << "." << month << "." day << " " << hour << ":" << minute << ":";
        oss.setf(std::fixed);
        oss.precision(SECOND_PRECISION);
        oss << second;
        oss.unsetf(std::fixed);
        oss.precision(default_precision);
        return oss.str();
    }

    double Util::getDeltaTime(const struct timespec& current_timespec, const struct timespec& previous_timespec)
    {
        struct timespec delta_timespec;
        delta_timespec.tv_sec = current_timespec.tv_sec - previous_timespec.tv_sec;
		delta_timespec.tv_nsec = current_timespec.tv_nsec - previous_timespec.tv_nsec;
		if (delta_timespec.tv_nsec < 0)
        {
			delta_timespec.tv_sec--;
			delta_timespec.tv_nsec += NANOSECONDS_PERSECOND;
		}

        double delta_time = delta_timespec.tv_sec * 1000 * 1000 + double(delta_timespec.tv_nsec) / 1000.0;
        return delta_time;
    }

    // Type conversion

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
            oss << "cannot convert " << val << " (> " << MAX_UINT16 << ") to uint16_t!";
            dumpErrorMsg(kClassName, oss.str());
            exit(1);
        }
    }

    // Client-edge-cloud scenario

    /*uint16_t Util::getLocalClientSendreqStartport(const uint32_t& global_client_idx)
    {
        int64_t local_client_sendreq_startport = 0;
        int64_t global_client_sendreq_startport = static_cast<int64_t>(covered::Config::getGlobalClientSendreqStartport());
        if (covered::Param::isSimulation())
        {
            int64_t perclient_workercnt = static_cast<int64_t>(covered::Param::getPerclientWorkercnt());
            local_client_sendreq_startport = global_client_sendreq_startport + static_cast<int64_t>(global_client_idx) * perclient_workercnt;
        }
        else
        {
            local_client_sendreq_startport = global_client_sendreq_startport;
        }
        return covered::Util::toUint16(local_client_sendreq_startport);
    }*/

    std::string Util::getLocalEdgeNodeIpstr(const uint32_t& global_client_idx)
    {
        std::string local_edge_node_ipstr = "";
        if (covered::Param::isSimulation())
        {
            local_edge_node_ipstr = covered::Util::LOCALHOST_IPSTR;
        }
        else
        {
            // TODO: set local_edge_node_ipstr based on covered::Config
            covered::Util::dumpErrorMsg(kClassName, "NOT support getLocalEdgeNodeIpstr for prototype now!");
            exit(1);
        }
        return local_edge_node_ipstr;
    }

    uint32_t Util::getGlobalWorkerIdx(const uint32_t& global_client_idx, const uint32_t local_worker_idx)
    {
        uint32_t global_worker_idx = global_client_idx * Param::getPerclientWorkercnt() + local_worker_idx;
        return global_worker_idx;
    }

    uint16_t Util::getLocalEdgeRecvreqPort(const uint32_t& global_edge_idx)
    {
        int64_t local_edge_recvreq_port = 0;
        int64_t global_edge_recvreq_startport = static_cast<int64_t>(covered::Config::getGlobalEdgeRecvreqStartport());
        if (covered::Param::isSimulation())
        {
            local_edge_recvreq_port = global_edge_recvreq_startport + global_edge_idx;
        }
        else
        {
            local_edge_recvreq_port = global_edge_recvreq_startport;
        }
        return covered::Util::toUint16(local_edge_recvreq_port);
    }

    // Network

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
            fragment_payload_size = msg_payload_size % UDP_MAX_FRAG_PAYLOAD;
        }
        assert(fragment_payload_size <= UDP_MAX_FRAG_PAYLOAD);
        return fragment_payload_size;
    }
}