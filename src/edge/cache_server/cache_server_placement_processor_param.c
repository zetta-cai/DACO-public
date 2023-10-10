#include "edge/cache_server/cache_server_placement_processor_param.h"

#include "common/util.h"

namespace covered
{
    const std::string CacheServerPlacementProcessorParam::kClassName("CacheServerPlacementProcessorParam");

    CacheServerPlacementProcessorParam::CacheServerPlacementProcessorParam()
    {
        cache_server_ptr_ = NULL;
        notify_placement_request_buffer_ptr_ = NULL;
        //local_cache_admission_buffer_ptr_ = NULL;
    }

    CacheServerPlacementProcessorParam::CacheServerPlacementProcessorParam(CacheServer* cache_server_ptr, const uint32_t& data_request_buffer_size)
    {
        cache_server_ptr_ = cache_server_ptr;
        
        // Allocate ring buffer for notify placement requests
        const bool notify_placement_with_multi_providers = false; // ONLY one provider (i.e., edge cache server) for notify placement requests
        notify_placement_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(CacheServerItem(), data_request_buffer_size, notify_placement_with_multi_providers);
        assert(notify_placement_request_buffer_ptr_ != NULL);

        // Allocate ring buffer for local edge cache admissions
        //const bool local_cache_admission_with_multi_providers = true; // Multiple providers (edge cache server workers after hybrid data fetching; edge cache server workers and edge beacon server for local placement notifications)
        //local_cache_admission_buffer_ptr_ = new RingBuffer<LocalCacheAdmissionItem>(LocalCacheAdmissionItem(), data_request_buffer_size, local_cache_admission_with_multi_providers);
        //assert(local_cache_admission_buffer_ptr_ != NULL);
    }

    CacheServerPlacementProcessorParam::~CacheServerPlacementProcessorParam()
    {
        // NOTE: no need to release cache_server_ptr_, which will be released outside CacheServerPlacementProcessorParam (e.g., by simulator)

        if (notify_placement_request_buffer_ptr_ != NULL)
        {
            delete notify_placement_request_buffer_ptr_;
            notify_placement_request_buffer_ptr_ = NULL;
        }

        //if (local_cache_admission_buffer_ptr_ != NULL)
        //{
        //    delete local_cache_admission_buffer_ptr_;
        //    local_cache_admission_buffer_ptr_ = NULL;
        //}
    }

    const CacheServerPlacementProcessorParam& CacheServerPlacementProcessorParam::operator=(const CacheServerPlacementProcessorParam& other)
    {
        // Shallow copy is okay, as cache_server_ptr_ is maintained outside CacheServerPlacementProcessorParam (e.g., by simulator)
        cache_server_ptr_ = other.cache_server_ptr_;

        // Must deep copy the ring buffer of notify placement requests
        if (notify_placement_request_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete notify_placement_request_buffer_ptr_;
            notify_placement_request_buffer_ptr_ = NULL;
        }
        if (other.notify_placement_request_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            const bool other_with_multi_providers = other.notify_placement_request_buffer_ptr_->withMultiProviders();
            assert(!other_with_multi_providers); // ONLY one provider (i.e., edge cache server) for notify placement requests

            notify_placement_request_buffer_ptr_ = new RingBuffer<CacheServerItem>(other.notify_placement_request_buffer_ptr_->getDefaultElement(), other.notify_placement_request_buffer_ptr_->getBufferSize(), other_with_multi_providers);
            assert(notify_placement_request_buffer_ptr_ != NULL);

            *notify_placement_request_buffer_ptr_ = *(other.notify_placement_request_buffer_ptr_);
        }

        // Must deep copy the ring buffer of local edge cache admissions
        /*if (local_cache_admission_buffer_ptr_ != NULL) // Release original ring buffer if any
        {
            delete local_cache_admission_buffer_ptr_;
            local_cache_admission_buffer_ptr_ = NULL;
        }
        if (other.local_cache_admission_buffer_ptr_ != NULL) // Deep copy other's ring buffer if any
        {
            const bool other_with_multi_providers = other.local_cache_admission_buffer_ptr_->withMultiProviders();
            assert(other_with_multi_providers); // Multiple providers (edge cache server workers after hybrid data fetching; edge cache server workers and edge beacon server for local placement notifications)

            local_cache_admission_buffer_ptr_ = new RingBuffer<LocalCacheAdmissionItem>(other.local_cache_admission_buffer_ptr_->getDefaultElement(), other.local_cache_admission_buffer_ptr_->getBufferSize(), other_with_multi_providers);
            assert(local_cache_admission_buffer_ptr_ != NULL);

            *local_cache_admission_buffer_ptr_ = *(other.local_cache_admission_buffer_ptr_);
        }*/

        return *this;
    }

    CacheServer* CacheServerPlacementProcessorParam::getCacheServerPtr() const
    {
        assert(cache_server_ptr_ != NULL);
        return cache_server_ptr_;
    }

    RingBuffer<CacheServerItem>* CacheServerPlacementProcessorParam::getNotifyPlacementRequestBufferPtr() const
    {
        assert(notify_placement_request_buffer_ptr_ != NULL);
        return notify_placement_request_buffer_ptr_;
    }

    /*RingBuffer<LocalCacheAdmissionItem>* CacheServerPlacementProcessorParam::getLocalCacheAdmissionBufferPtr() const
    {
        assert(local_cache_admission_buffer_ptr_ != NULL);
        return local_cache_admission_buffer_ptr_;
    }*/
}