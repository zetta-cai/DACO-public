/*
 * EdgeWrapper: the edge node to receive requests and send responses.
 * 
 * By Siyuan Sheng (2023.04.22).
 */

#ifndef EDGE_WRAPPER_H
#define EDGE_WRAPPER_H

#include <string>

#include "edge/edge_param.h"
#include "message/message_base.h"

namespace covered
{
    class EdgeWrapper
    {
    public:
        static void* launchEdge(void* local_edge_param_ptr);

        EdgeWrapper(EdgeParam* local_edge_param_ptr);
        ~EdgeWrapper();

        void start();
    private:
        static const std::string kClassName;

        MessageBase* processDataRequest_(MessageBase* request_ptr);
        MessageBase* processControlRequest_(MessageBase* request_ptr);

        void blockForInvalidation_(const Key& key);
        void triggerIndependentAdmission(const Key& key, const Value& value);

        EdgeParam* local_edge_param_ptr_;
        CacheWrapperBase* local_edge_cache_ptr_;
    };
}

#endif