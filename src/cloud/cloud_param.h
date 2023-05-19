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

        bool isCloudRunning();
        void setCloudRunning();
        void resetCloudRunning();
    private:
        static const std::string kClassName;

         volatile std::atomic<bool> local_cloud_running_;
    };
}

#endif