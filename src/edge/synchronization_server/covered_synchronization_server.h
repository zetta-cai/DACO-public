/*
 * CoveredSynchronizationServer: perform remote victim synchronization for received cross-edge messages in parallel to avoid foreground time cost of victim syncset recovery and victim tracker update.
 *
 * NOTE: ONLY COVERED needs such synchronization server, and both cache server and beacon server will push SynchronizationServerItem for victim synchronization.
 * 
 * By Siyuan Sheng (2023.12.13).
 */

#ifndef COVERED_SYNCHRONIZATION_SERVER_H
#define COVERED_SYNCHRONIZATION_SERVER_H

#include <string>

#include "edge/synchronization_server/synchronization_server_item.h"
#include "edge/synchronization_server/synchronization_server_param.h"

namespace covered
{
    class CoveredSynchronizationServer
    {
    public:
        static void* launchCoveredSynchronizationServer(void* synchronization_server_param_ptr);
    
        CoveredSynchronizationServer(SynchronizationServerParam* synchronization_server_param_ptr);
        virtual ~CoveredSynchronizationServer();

        void start();
    private:
        static const std::string kClassName;

        // Return if edge node is finished
        bool processRemoteVictimSynchronization_(const SynchronizationServerItem& synchronization_server_item); // Process remote victim synchronization

        void checkPointers_() const;

        // Member variables

        // Const variable
        std::string instance_name_;
        const SynchronizationServerParam* synchronization_server_param_ptr_;
    };
}

#endif