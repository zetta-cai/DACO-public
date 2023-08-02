#include "common/param/client_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/param/edge_param.h"
#include "common/util.h"

namespace covered
{
    const std::string ClientParam::kClassName("ClientParam");

    uint32_t ClientParam::clientcnt_ = 0;
    bool ClientParam::is_warmup_speedup_ = true;
    uint32_t ClientParam::keycnt_ = 0;
    uint32_t ClientParam::opcnt_ = 0;
    uint32_t ClientParam::perclient_workercnt_ = 0;

    void ClientParam::setParameters(const uint32_t& clientcnt, const bool& is_warmup_speedup, const uint32_t& keycnt, const uint32_t& opcnt, const uint32_t& perclient_workercnt)
    {
        // NOTE: ClientParam::setParameters() does NOT rely on any other module
        if (is_valid_)
        {
            return; // NO need to set parameters once again

            //Util::dumpErrorMsg(kClassName, "ClientParam::setParameters cannot be invoked more than once!");
            //exit(1);
        }
        else
        {
            Util::dumpNormalMsg(kClassName, "invoke setParameters()!");
        }

        clientcnt_ = clientcnt;
        is_warmup_speedup_ = is_warmup_speedup;
        keycnt_ = keycnt;
        opcnt_ = opcnt;
        perclient_workercnt_ = perclient_workercnt;

        verifyIntegrity_();

        is_valid_ = true;
        return;
    }

    uint32_t ClientParam::getClientcnt()
    {
        checkIsValid_();
        return clientcnt_;
    }

    bool ClientParam::isWarmupSpeedup()
    {
        checkIsValid_();
        return is_warmup_speedup_;
    }

    uint32_t ClientParam::getKeycnt()
    {
        checkIsValid_();
        return keycnt_;
    }

    uint32_t ClientParam::getOpcnt()
    {
        checkIsValid_();
        return opcnt_;
    }

    uint32_t ClientParam::getPerclientWorkercnt()
    {
        checkIsValid_();
        return perclient_workercnt_;
    }

    std::string ClientParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Client count: " << clientcnt_ << std::endl;
        oss << "Warmup speedup flag: " << (is_warmup_speedup_?"true":"false") << std::endl;
        oss << "Key count (dataset size): " << keycnt_ << std::endl;
        oss << "Operation count (workload size): " << opcnt_ << std::endl;
        oss << "Per-client worker count: " << perclient_workercnt_ << std::endl;

        return oss.str();  
    }

    void ClientParam::verifyIntegrity_()
    {
        assert(clientcnt_ > 0);
        assert(keycnt_ > 0);
        assert(opcnt_ > 0);
        assert(perclient_workercnt_ > 0);

        if (EdgeParam::isEdgecntValid())
        {
            uint32_t edgecnt = EdgeParam::getEdgecnt();
            if (clientcnt_ < edgecnt)
            {
                std::ostringstream oss;
                oss << "clientcnt " << clientcnt_ << " should >= edgecnt " << edgecnt << " for edge-client mapping!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }

        return;
    }

    void ClientParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid ClientParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }
}