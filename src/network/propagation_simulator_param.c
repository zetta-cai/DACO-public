#include "network/propagation_simulator_param.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string PropagationSimulatorParam::kClassName("PropagationSimulatorParam");


    PropagationSimulatorParam::PropagationSimulatorParam() : SubthreadParamBase(), node_wrapper_ptr_(NULL), propagation_latency_distname_(""), propagation_latency_lbound_us_(0), propagation_latency_avg_us_(0), propagation_latency_rbound_us_(0), propagation_latency_random_seed_(0), rwlock_for_propagation_item_buffer_("rwlock_for_propagation_item_buffer_"), is_first_item_(true), prev_timespec_(), propagation_latency_randgen_(0)
    {
        realnet_option_ = "";
        instance_name_ = "";

        propagation_item_buffer_ptr_ = NULL;

        propagation_latency_dist_ptr_ = NULL;
    }

    PropagationSimulatorParam::PropagationSimulatorParam(NodeWrapperBase* node_wrapper_ptr, const std::string& propagation_latency_distname, const uint32_t& propagation_latency_lbound_us, const uint32_t& propagation_latency_avg_us, const uint32_t& propagation_latency_rbound_us, const uint32_t& propagation_latency_random_seed, const uint32_t& propagation_item_buffer_size, const std::string& realnet_option, const std::vector<uint32_t> _p2p_latency_array) : SubthreadParamBase(), node_wrapper_ptr_(node_wrapper_ptr), propagation_latency_distname_(propagation_latency_distname), propagation_latency_lbound_us_(propagation_latency_lbound_us), propagation_latency_avg_us_(propagation_latency_avg_us), propagation_latency_rbound_us_(propagation_latency_rbound_us), propagation_latency_random_seed_(propagation_latency_random_seed), rwlock_for_propagation_item_buffer_("rwlock_for_propagation_item_buffer_"), is_first_item_(true), prev_timespec_(), propagation_latency_randgen_(propagation_latency_random_seed), p2p_latency_array(_p2p_latency_array)
    {
        assert(node_wrapper_ptr != NULL);

        // ONLY need to check for non-constant distributions
        if (propagation_latency_distname != Util::PROPAGATION_SIMULATION_CONSTANT_DISTNAME)
        {
            assert(propagation_latency_lbound_us <= propagation_latency_avg_us);
            assert(propagation_latency_avg_us <= propagation_latency_rbound_us);
            propagation_latency_delta = propagation_latency_rbound_us - propagation_latency_lbound_us;
            assert(propagation_latency_delta > 0); // Ensure the range is valid
        }

        realnet_option_ = realnet_option;

        // Differential propagation simulator parameter of different nodes
        std::ostringstream oss;
        oss << kClassName << " " << node_wrapper_ptr->getNodeRoleIdxStr();
        instance_name_ = oss.str();

        const bool with_multi_providers = false; // Although with multiple providers (all subthreads of a client/edge/cloud node), rwlock_for_propagation_item_buffer_ has already guaranteed the atomicity of propagation_item_buffer_ptr_
        propagation_item_buffer_ptr_ = new RingBuffer<PropagationItem>(PropagationItem(), propagation_item_buffer_size, with_multi_providers);
        assert(propagation_item_buffer_ptr_ != NULL);

        propagation_latency_dist_ptr_ = new std::uniform_int_distribution<uint32_t>(propagation_latency_lbound_us, propagation_latency_rbound_us);
        assert(propagation_latency_dist_ptr_ != NULL);
    }


    PropagationSimulatorParam::~PropagationSimulatorParam()
    {
        assert(propagation_item_buffer_ptr_ != NULL);

        // Release not-issued messages in propagation_item_buffer_ptr_ (NOTE: now edge is NOT running and propagation simulator will NOT pop messages from param, so ONLY one consumer and hence NO need to acquire a write lock for edge wrapper to release messages)
        while (true)
        {
            PropagationItem tmp_item;
            bool is_successful = propagation_item_buffer_ptr_->pop(tmp_item);
            if (is_successful)
            {
                MessageBase* tmp_msgptr = tmp_item.getMessagePtr();
                assert(tmp_msgptr != NULL);
                delete tmp_msgptr;
                tmp_msgptr = NULL;
            }
            else
            {
                break;
            }
        }

        // Release ring buffer itself
        delete propagation_item_buffer_ptr_;
        propagation_item_buffer_ptr_ = NULL;

        // Release random latency distribution
        assert(propagation_latency_dist_ptr_ != NULL);
        delete propagation_latency_dist_ptr_;
        propagation_latency_dist_ptr_ = NULL;
    }

    const NodeWrapperBase* PropagationSimulatorParam::getNodeWrapperPtr() const
    {
        // No need to acquire a lock due to const shared variable
        assert(node_wrapper_ptr_ != NULL);
        return node_wrapper_ptr_;
    }

    bool PropagationSimulatorParam::push(MessageBase* message_ptr, const NetworkAddr& dst_addr)
    {
        assert(message_ptr != NULL);
        assert(dst_addr.isValidAddr());

        // Acquire a write lock
        std::string context_name = "PropagationSimulatorParam::push()";
        rwlock_for_propagation_item_buffer_.acquire_lock(context_name);

        // Calculate emission latency for the current message
        // NOTE: as the incoming and outcoming links use different random seeds to generate WAN delays, using RTT/2 still follows asymmetric network settings and the range is still [left bound, right bound] of the given type of latency
        const uint32_t propagation_latency = genPropagationLatency_();
        const uint32_t cur_emission_latency_us = propagation_latency / 2;

        const bool skip_propagation_latency = message_ptr->getExtraCommonMsghdr().isSkipPropagationLatency();
        uint32_t sleep_us = 0;
        if (skip_propagation_latency) // Warmup phase
        {
            // skip_propagation_latency = true means the message is enabled with warmup speedup under warmup phase
            sleep_us = 0;
        }
        else if (realnet_option_ == Util::REALNET_LOAD_OPTION_NAME) // Real-net stresstest phase
        {
            sleep_us = 0; // NOTE: NO need to sleep manually -> we use the real-cloud transmission latency
        }
        else // Stresstest phase
        {
            // Calculate sleep interval
            struct timespec cur_timespec = Util::getCurrentTimespec();
            if (is_first_item_)
            {
                sleep_us = cur_emission_latency_us;

                prev_timespec_ = cur_timespec;
                is_first_item_ = false;
            }
            else
            {
                sleep_us = Util::getDeltaTimeUs(cur_timespec, prev_timespec_);
                if (sleep_us > cur_emission_latency_us)
                {
                    sleep_us = cur_emission_latency_us;
                }

                prev_timespec_ = cur_timespec;
            }
        }
        assert(sleep_us <= cur_emission_latency_us);

        // Push propagation item into ring buffer
        PropagationItem propagation_item(message_ptr, dst_addr, sleep_us);
        bool is_successful = propagation_item_buffer_ptr_->push(propagation_item);

        #ifdef DEBUG_PROPAGATION_SIMULATOR_PARAM
        //std::vector<PropagationItem> tmp_propagation_items = propagation_item_buffer_ptr_->getElementsForDebug();
        std::ostringstream oss;
        oss << "push to sleep " << sleep_us << " us to simulate an emission latency of " << cur_emission_latency_us << " us; keystr: " << MessageBase::getKeyFromMessage(message_ptr).getKeystr() << "; dstadrr: " << dst_addr.toString() << "; srcaddr: " << message_ptr->getSourceAddr().toString() << "; ";
        //for (uint32_t i = 0; i < tmp_propagation_items.size(); i++)
        //{
        //    oss << "tmp_propagation_items[" << i << "] sleep_us: " << tmp_propagation_items[i].getSleepUs() << "; ";
        //}
        Util::dumpDebugMsg(instance_name_, oss.str());
        #endif

        // Release a write lock
        rwlock_for_propagation_item_buffer_.unlock(context_name);

        return is_successful;
    }

    bool PropagationSimulatorParam::pop(PropagationItem& element)
    {
        // NOTE: NO need to acquire a write lock, as there will be ONLY one reader (i.e., the propagation simulator)

        // Acquire a write lock
        //std::string context_name = "PropagationSimulatorParam::pop()";
        //rwlock_for_propagation_item_buffer_.acquire_lock(context_name);

        bool is_successful = propagation_item_buffer_ptr_->pop(element);

        // Release a write lock
        //rwlock_for_propagation_item_buffer_.unlock(context_name);

        return is_successful;
    }

    uint32_t PropagationSimulatorParam::genPropagationLatency()
    {
        // Acquire a write lock
        std::string context_name = "PropagationSimulatorParam::genPropagationLatency()";
        rwlock_for_propagation_item_buffer_.acquire_lock(context_name);

        uint32_t propagation_latency = genPropagationLatency_();

        // Release a write lock
        rwlock_for_propagation_item_buffer_.unlock(context_name);

        return propagation_latency;
    }

    const PropagationSimulatorParam& PropagationSimulatorParam::operator=(const PropagationSimulatorParam& other)
    {
        SubthreadParamBase::operator=(other);

        node_wrapper_ptr_ = other.node_wrapper_ptr_;
        propagation_latency_distname_ = other.propagation_latency_distname_;
        propagation_latency_lbound_us_ = other.propagation_latency_lbound_us_;
        propagation_latency_avg_us_ = other.propagation_latency_avg_us_;
        propagation_latency_rbound_us_ = other.propagation_latency_rbound_us_;
        propagation_latency_random_seed_ = other.propagation_latency_random_seed_;
        p2p_latency_array = other.p2p_latency_array;

        instance_name_ = other.instance_name_;
        
        // Not copy mutex_lock_

        // Deep copy propagation_item_buffer_ptr_
        if (propagation_item_buffer_ptr_ != NULL)
        {
            delete propagation_item_buffer_ptr_;
            propagation_item_buffer_ptr_ = NULL;
        }
        if (other.propagation_item_buffer_ptr_ != NULL)
        {
            const bool with_multi_providers = other.propagation_item_buffer_ptr_->withMultiProviders();
            assert(!with_multi_providers); // Although with multiple providers (all subthreads of a client/edge/cloud node), rwlock_for_propagation_item_buffer_ has already guaranteed the atomicity of propagation_item_buffer_ptr_

            propagation_item_buffer_ptr_ = new RingBuffer<PropagationItem>(PropagationItem(), other.propagation_item_buffer_ptr_->getBufferSize(), with_multi_providers);
            assert(propagation_item_buffer_ptr_ != NULL);
            
            *propagation_item_buffer_ptr_ = *other.propagation_item_buffer_ptr_; // deep copy
        }

        is_first_item_ = other.is_first_item_;
        prev_timespec_ = other.prev_timespec_;

        propagation_latency_randgen_ = other.propagation_latency_randgen_;

        // Deep copy
        if (propagation_latency_dist_ptr_ != NULL)
        {
            delete propagation_latency_dist_ptr_;
            propagation_latency_dist_ptr_ = NULL;
        }
        if (other.propagation_latency_dist_ptr_ != NULL)
        {
            propagation_latency_dist_ptr_ = new std::uniform_int_distribution<uint32_t>(other.propagation_latency_lbound_us_, other.propagation_latency_rbound_us_);
            assert(propagation_latency_dist_ptr_ != NULL);
        }
        
        return *this;
    }

    uint32_t PropagationSimulatorParam::genPropagationLatency_()
    {
        // NOTE: NO need to acquire write lock here, which has been done in PropagationSimulatorParam::push() and PropagationSimulatorParam::genPropagationLatency()

        uint32_t propagation_latency = 0;
        if (propagation_latency_distname_ == Util::PROPAGATION_SIMULATION_CONSTANT_DISTNAME)
        {
            propagation_latency = propagation_latency_avg_us_;
        }
        else if (propagation_latency_distname_ == Util::PROPAGATION_SIMULATION_UNIFORM_DISTNAME)
        {
            assert(propagation_latency_dist_ptr_ != NULL);
            propagation_latency = (*propagation_latency_dist_ptr_)(propagation_latency_randgen_);

            assert(propagation_latency >= propagation_latency_lbound_us_);
            assert(propagation_latency <= propagation_latency_rbound_us_);
        }
        else if (propagation_latency_distname_.rfind(Util::PROPAGATION_SIMULATION_POISSON_DISTNAME, 0) == 0)
        {
            // 提取泊松分布参数λ（格式：POISSON_DISTNAME + "_" + lambda）
            std::string lambda_str = propagation_latency_distname_.substr(Util::PROPAGATION_SIMULATION_POISSON_DISTNAME.length() + 1);
            double lambda = std::stod(lambda_str);
            assert(lambda > 0.0 && "Poisson distribution lambda must be positive");
            assert(lambda >= propagation_latency_lbound_us_ && lambda <= propagation_latency_rbound_us_ && "Poisson distribution lambda must be within [lbound, rbound]");
            // 生成泊松分布随机数（均值为lambda）
            std::poisson_distribution<uint32_t> poisson_dist(lambda);
            propagation_latency = poisson_dist(propagation_latency_randgen_);
    
            // 截断处理：超出范围时取边界值
            if (propagation_latency < propagation_latency_lbound_us_)
            {
                propagation_latency = propagation_latency_lbound_us_;
            }
            else if (propagation_latency > propagation_latency_rbound_us_)
            {
                propagation_latency = propagation_latency_rbound_us_;
            }
        }
        else if (propagation_latency_distname_.rfind(Util::PROPAGATION_SIMULATION_PARETO_DISTNAME, 0) == 0)
        {
            // 提取帕累托分布参数k（格式：PARETO_DISTNAME + "_" + k）
            std::string k_str = propagation_latency_distname_.substr(Util::PROPAGATION_SIMULATION_PARETO_DISTNAME.length() + 1);
            double k = std::stod(k_str);
            assert(k > 0.0 && "Pareto distribution k must be positive");
                
            // 定义边界参数
            const double L = propagation_latency_lbound_us_;  // 左边界
            const double U = propagation_latency_rbound_us_;  // 右边界

            // 生成(0,1)区间的均匀随机数（用于逆变换）
            std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);
            double u = uniform_dist(propagation_latency_randgen_);  // 复用已有的随机数生成器
            if (u >= 1.0) u = 1.0 - 1e-10;  // 避免分母为0
            // 计算缩放因子和延迟值（对应Python公式）
            const double factor = 1.0 - pow(L / U, k);
            const double term = 1.0 - u * factor;
            assert(term > 0.0 && "Invalid term in Pareto calculation (must be positive)");

            // 计算最终延迟并转换为整数
            const double raw_delay = L * pow(term, -1.0 / k);
            propagation_latency = static_cast<uint32_t>(raw_delay);

            // 验证结果在有效范围内
            assert(propagation_latency >= L && propagation_latency <= U 
                && "Pareto generated latency out of [left, right] range");
        }
        else 
        {
            std::ostringstream oss;
            oss << "propagation latency distribution " << propagation_latency_distname_ << " is not supported!" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return propagation_latency;
    }

    uint32_t PropagationSimulatorParam::genPropagationLatency_of_j(int j)
    {
        // NOTE: NO need to acquire write lock here, which has been done in PropagationSimulatorParam::push() and PropagationSimulatorParam::genPropagationLatency()

        assert(j >= 0 && j < (int)p2p_latency_array.size());
        uint32_t propagation_latency_base = p2p_latency_array[j];
        uint32_t propagation_latency = 0;
        if (propagation_latency_distname_ == Util::PROPAGATION_SIMULATION_CONSTANT_DISTNAME)
        {
            propagation_latency = propagation_latency_base;
        }
        else if (propagation_latency_distname_ == Util::PROPAGATION_SIMULATION_UNIFORM_DISTNAME)
        {
            // generate a random latency based on the base latency
            assert(propagation_latency_dist_ptr_ != NULL);
            uint32_t propagation_latency_off_lbound = (*propagation_latency_dist_ptr_)(propagation_latency_randgen_);
            
            assert(propagation_latency_off_lbound >= propagation_latency_lbound_us_);
            assert(propagation_latency_off_lbound <= propagation_latency_rbound_us_);
            propagation_latency = propagation_latency_base + propagation_latency_off_lbound - propagation_latency_avg_us_;
            assert(propagation_latency >= 0);
        }
        else
        {
            std::ostringstream oss;
            oss << "propagation latency distribution " << propagation_latency_distname_ << " is not supported!" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }

        return propagation_latency;
    }
} // namespace covered