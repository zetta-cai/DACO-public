/*
 * CoveredBgfetchGlobalGetRequest: a request issued by an edge node to cloud to get an existing value if any for background non-blocking data fetching of non-blocking placement deployment in COVERED.
 *
 * NOTE: background request with placement edgeset to trigger non-blocking placement notification.
 * 
 * By Siyuan Sheng (2023.09.27).
 */

#ifndef COVERED_BGFETCH_GLOBAL_GET_REQUEST_H
#define COVERED_BGFETCH_GLOBAL_GET_REQUEST_H

#include <string>

#include "common/dynamic_array.h"
#include "common/key.h"
#include "message/key_edgeset_message.h"

namespace covered
{
    class CoveredBgfetchGlobalGetRequest : public KeyEdgesetMessage
    {
    public:
        CoveredBgfetchGlobalGetRequest(const Key& key, const Edgeset& edgeset, const uint32_t& source_index, const NetworkAddr& source_addr, const ExtraCommonMsghdr& extra_common_msghdr);
        CoveredBgfetchGlobalGetRequest(const DynamicArray& msg_payload);
        virtual ~CoveredBgfetchGlobalGetRequest();
    private:
        static const std::string kClassName;
    };
}

#endif