#include "benchmark/client_param.h"

#include "common/util.h"

namespace covered
{
    const std::string ClientParam::kClassName("ClientParam");

    ClientParam::ClientParam() : NodeParamBase(NodeParamBase::CLIENT_NODE_ROLE, 0, false)
    {
        is_warmup_phase_.store(true, Util::STORE_CONCURRENCY_ORDER);
        is_stresstest_phase_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }

    ClientParam::ClientParam(const uint32_t& client_idx) : NodeParamBase(NodeParamBase::CLIENT_NODE_ROLE, client_idx, false)
    {
        is_warmup_phase_.store(true, Util::STORE_CONCURRENCY_ORDER);
        is_stresstest_phase_.store(false, Util::STORE_CONCURRENCY_ORDER);
    }
        
    ClientParam::~ClientParam() {}

    bool ClientParam::isWarmupPhase() const
    {
        return is_warmup_phase_.load(Util::LOAD_CONCURRENCY_ORDER);
    }

    void ClientParam::finishWarmupPhase()
    {
        is_warmup_phase_.store(false, Util::STORE_CONCURRENCY_ORDER);
        return;
    }

    bool ClientParam::isStresstestPhase() const
    {
        return is_stresstest_phase_.load(Util::LOAD_CONCURRENCY_ORDER);
    }
    
    void ClientParam::startStresstestPhase()
    {
        is_stresstest_phase_.store(true, Util::STORE_CONCURRENCY_ORDER);
        return;
    }

    const ClientParam& ClientParam::operator=(const ClientParam& other)
    {
        NodeParamBase::operator=(other);

        is_warmup_phase_.store(other.is_warmup_phase_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);
        is_stresstest_phase_.store(other.is_stresstest_phase_.load(Util::LOAD_CONCURRENCY_ORDER), Util::STORE_CONCURRENCY_ORDER);

        return *this;
    }
}