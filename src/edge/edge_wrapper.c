#include "edge/edge_wrapper.h"

#include <assert.h>
#include <sstream>

#include "common/util.h"
#include "edge/beacon_server/beacon_server_base.h"
#include "edge/cache_server/cache_server_base.h"

namespace covered
{
    const std::string EdgeWrapper::kClassName("EdgeWrapper");

    EdgeWrapper::EdgeWrapper(const std::string& cache_name, const std::string& hash_name, EdgeParam* edge_param_ptr, const uint32_t& capacity_bytes) : cache_name_(cache_name), edge_param_ptr_(edge_param_ptr), capacity_bytes_(capacity_bytes)
    {
        if (edge_param_ptr == NULL)
        {
            Util::dumpErrorMsg(kClassName, "edge_param_ptr is NULL!");
            exit(1);
        }

        // Differentiate different edge nodes
        std::ostringstream oss;
        oss << kClassName << " edge" << edge_param_ptr->getEdgeIdx();
        instance_name_ = oss.str();
        
        // Allocate local edge cache to store hot objects
        edge_cache_ptr_ = CacheWrapperBase::getEdgeCache(cache_name_, edge_param_ptr);
        assert(edge_cache_ptr_ != NULL);

        // Allocate cooperation wrapper for cooperative edge caching
        cooperation_wrapper_ptr_ = CooperationWrapperBase::getCooperationWrapper(cache_name, hash_name, edge_param_ptr);
        assert(cooperation_wrapper_ptr_ != NULL);
    }
        
    EdgeWrapper::~EdgeWrapper()
    {
        // NOTE: no need to delete edge_param_ptr, as it is maintained outside EdgeWrapper

        // Release local edge cache
        assert(edge_cache_ptr_ != NULL);
        delete edge_cache_ptr_;
        edge_cache_ptr_ = NULL;

        // Release cooperative cache wrapper
        assert(cooperation_wrapper_ptr_ != NULL);
        delete cooperation_wrapper_ptr_;
        cooperation_wrapper_ptr_ = NULL;
    }

    void EdgeWrapper::start()
    {
        assert(edge_param_ptr_ != NULL);
        uint32_t edge_idx = edge_param_ptr_->getEdgeIdx();

        int pthread_returncode;
        pthread_t beacon_server_thread;
        pthread_t cache_server_thread;
        pthread_t invalidation_server_thread;

        // Launch beacon server
        pthread_returncode = pthread_create(&beacon_server_thread, NULL, launchBeaconServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to launch beacon server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch cache server
        pthread_returncode = pthread_create(&cache_server_thread, NULL, launchCacheServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to launch cache server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Launch invalidations server
        pthread_returncode = pthread_create(&invalidation_server_thread, NULL, launchInvalidationServer_, (void*)(this));
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to launch invalidation server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for beacon server
        pthread_returncode = pthread_join(beacon_server_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join beacon server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for cache server
        pthread_returncode = pthread_join(cache_server_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join cache server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        // Wait for invalidation server
        pthread_returncode = pthread_join(invalidation_server_thread, NULL); // void* retval = NULL
        if (pthread_returncode != 0)
        {
            std::ostringstream oss;
            oss << "edge " << edge_idx << " failed to join invalidation server (error code: " << pthread_returncode << ")" << std::endl;
            covered::Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return;
    }

    void* EdgeWrapper::launchBeaconServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        BeaconServerBase* beacon_server_ptr = BeaconServerBase::getBeaconServer((EdgeWrapper*)edge_wrapper_ptr);
        assert(beacon_server_ptr != NULL);
        beacon_server_ptr->start();

        pthread_exit(NULL);
        return NULL;
    }
    
    void* EdgeWrapper::launchCacheServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        CacheServerBase* cache_server_ptr = CacheServerBase::getCacheServer((EdgeWrapper*)edge_wrapper_ptr);
        assert(cache_server_ptr != NULL);
        cache_server_ptr->start();

        pthread_exit(NULL);
        return NULL;
    }
    
    void* EdgeWrapper::launchInvalidationServer_(void* edge_wrapper_ptr)
    {
        assert(edge_wrapper_ptr != NULL);

        // TODO: uncomment after introducing invalidation server
        //InvalidationServerBase* invalidation_server_ptr = InvalidationServerBase::getInvalidationServer((EdgeWrapper*)edge_wrapper_ptr);
        //assert(invalidation_server_ptr != NULL);
        //invalidation_server_ptr->start();

        pthread_exit(NULL);
        return NULL;
    }

    uint32_t EdgeWrapper::getSizeForCapacity_() const
    {
        uint32_t size = edge_cache_ptr_->getSizeForCapacity() + cooperation_wrapper_ptr_->getSizeForCapacity();
        return size;
    }
}