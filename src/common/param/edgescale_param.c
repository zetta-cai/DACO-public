#include "common/param/edgescale_param.h"

#include <assert.h>
#include <cstdlib> // exit
#include <sstream> // ostringstream

#include "common/param/client_param.h"
#include "common/util.h"

namespace covered
{
    const std::string EdgescaleParam::kClassName("EdgescaleParam");

    bool EdgescaleParam::is_valid_ = false;

    uint32_t EdgescaleParam::edgecnt_ = 0;

    void EdgescaleParam::setParameters(const uint32_t& edgecnt)
    {
        // NOTE: NOT rely on any other module
        if (is_valid_)
        {
            return; // NO need to set parameters once again
        }
        else
        {
            Util::dumpNormalMsg(kClassName, "invoke setParameters()!");
        }

        edgecnt_ = edgecnt;

        verifyIntegrity_();

        is_valid_ = true;
        return;
    }

    uint32_t EdgescaleParam::getEdgecnt()
    {
        checkIsValid_();
        return edgecnt_;
    }

    std::string EdgescaleParam::toString()
    {
        checkIsValid_();

        std::ostringstream oss;
        oss << "[Dynamic configurations from CLI parameters in " << kClassName << "]" << std::endl;
        oss << "Edge count: " << edgecnt_;

        return oss.str();  
    }

    void EdgescaleParam::checkIsValid_()
    {
        if (!is_valid_)
        {
            Util::dumpErrorMsg(kClassName, "invalid EdgescaleParam (CLI parameters have not been set)!");
            exit(1);
        }
        return;
    }

    void EdgescaleParam::verifyIntegrity_()
    {
        assert(edgecnt_ > 0);

        if (ClientParam::isValid())
        {
            uint32_t clientcnt = ClientParam::getClientcnt();
            if (clientcnt < edgecnt_)
            {
                std::ostringstream oss;
                oss << "clientcnt " << clientcnt << " should >= edgecnt " << edgecnt_ << " for edge-client mapping!";
                Util::dumpErrorMsg(kClassName, oss.str());
                exit(1);
            }
        }
        
        return;
    }
}