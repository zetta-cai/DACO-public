/*
 * CloudParam: parameters to launch the cloud node (thread safe).
 * 
 * By Siyuan Sheng (2023.05.19).
 */

#ifndef CLOUD_PARAM_H
#define CLOUD_PARAM_H

#include <atomic>
#include <string>

#include "common/node_param_base.h"

namespace covered
{
    class CloudParam : public NodeParamBase
    {
    public:
        // TODO: only support 1 cloud node now!
        CloudParam();
        ~CloudParam();

        const CloudParam& operator=(const CloudParam& other);
    private:
        static const std::string kClassName;
    };
}

#endif