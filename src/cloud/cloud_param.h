/*
 * CloudParam: parameters to launch the cloud node.
 * 
 * By Siyuan Sheng (2023.05.19).
 */

#ifndef CLOUD_PARAM_H
#define CLOUD_PARAM_H

#include <atomic>
#include <string>

namespace covered
{
    class CloudParam
    {
    public:
        CloudParam();
        ~CloudParam();

        const CloudParam& operator=(const CloudParam& other);

        uint32_t getCloudIdx() const;

        bool isCloudRunning();
        void setCloudRunning();
        void resetCloudRunning();
    private:
        static const std::string kClassName;

        volatile std::atomic<bool> cloud_running_;

        uint32_t cloud_idx_; // TODO: only support 1 cloud node now!
    };
}

#endif