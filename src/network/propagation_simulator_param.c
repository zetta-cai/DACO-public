#include "network/propagation_simulator_param.h"

#include <assert.h>

#include "common/util.h"

namespace covered
{
    const std::string PropagationSimulatorParam::kClassName("PropagationSimulatorParam");

    PropagationSimulatorParam::LinkDistParams::LinkDistParams()
    : type(LinkDistType::UNIFORM), l(0), r(0), lambda(0), k(0.0) {}

    // 带参构造：根据分布类型初始化对应参数
    
    PropagationSimulatorParam::LinkDistParams::LinkDistParams(LinkDistType t, uint32_t l_val, uint32_t r_val, uint32_t extra1 = 0, double extra2 = 0.0)
        : type(t), l(l_val), r(r_val) {
        switch (type) {
            case LinkDistType::POISSON:
                lambda = extra1;  // 泊松只需λ
                break;
            case LinkDistType::PARETO:
                k = extra2;       // 帕累托只需k
                break;
            case LinkDistType::UNIFORM:
                uniform_l = l;
                uniform_r = r;
                break;
            default:  // UNIFORM无需额外参数
                break;
        }
    }

    PropagationSimulatorParam::LinkDistParams::LinkDistParams(std::string args, uint32_t seed){
        // Parse the string
        // Expected format: distname_l_r_x1_x2_U
        std::istringstream iss(args);
        std::string token;
        std::vector<std::string> tokens;

        while (std::getline(iss, token, '_')) {
            tokens.push_back(token);
        }

        if (tokens.size() < 5) {
            throw std::invalid_argument("Invalid argument format for LinkDistParams");
        }

        // Extract parameters
        std::string distname = tokens[0];
        l = std::stoul(tokens[1]);
        r = std::stoul(tokens[2]);
        if (distname == "uniform") {
            type = LinkDistType::UNIFORM;
            uint32_t x1 = std::stoul(tokens[3]); // Bug: Should be lambda
            uint32_t x2 = std::stoul(tokens[4]);
            if (x2 >= r) {
                throw std::invalid_argument("x2 must be smaller than r for UNIFORM distribution");
            }
            std::uniform_int_distribution<uint32_t> uniform_dist(x1, x2);
            std::mt19937_64 randgen(seed);
            uniform_r = uniform_dist(randgen);
            uniform_l = l;
        } else if (distname == "poisson") {
            type = LinkDistType::POISSON;
            uint32_t x1 = std::stoul(tokens[3]);
            uint32_t x2 = std::stoul(tokens[4]);
            std::uniform_int_distribution<uint32_t> uniform_dist(x1, x2);
            std::mt19937_64 randgen(seed);
            lambda = uniform_dist(randgen);
        } else if (distname == "pareto") {
            type = LinkDistType::PARETO;
            double x1 = std::stod(tokens[3]);
            double x2 = std::stod(tokens[4]);
            std::uniform_real_distribution<double> uniform_dist(x1, x2);
            std::mt19937_64 randgen(seed);
            k = uniform_dist(randgen);
        } else {
            throw std::invalid_argument("Unsupported distribution type: " + distname);
        }

    }

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
        // print latency bound
        // std::cout << "Propagation latency distribution: " << propagation_latency_distname << " with range [" << propagation_latency_lbound_us << ", " << propagation_latency_rbound_us << "] us" << std::endl;

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

        // check _p2p_latency_array
        // 1. if empty set link_size to -1, this simulate for a node, all links are the same
        if (_p2p_latency_array.empty()) {
            link_size = 0;
        } else {
            link_size = _p2p_latency_array.size();
        }
        is_rnd_link = false;
        // check contents in _p2p_latency_array, if all value is INT_MAX, set is_rnd_link to true
        if(link_size != 0){
            bool is_all_int_max = true;
            for (uint32_t i = 0; i < link_size; i++) {
                if (_p2p_latency_array[i] != UINT32_MAX) {
                    is_all_int_max = false;
                    break;
                }
            }
            if (is_all_int_max) {
                is_rnd_link = true;
            } else {
                is_rnd_link = false;
            }
        } 
        // print link_size is_rnd_link
        // std::cout << "Link size: " << link_size << ", is_rnd_link: " << (is_rnd_link ? "true" : "false") << std::endl;

        // 2. if is_rnd_link is true, generate random seeds for each link
        //   (seed_base + node_idx * node_factor) * link_factor + link_idx, (seed_base + node_idx * node_factor) is propagation_latency_random_seed
        if(is_rnd_link){
            link_seeds.resize(link_size);
            for (uint32_t i = 0; i < link_size; i++) {
                link_seeds[i] = propagation_latency_random_seed * PROPAGATION_SIMULATION_LINK_FACTOR + i;
                link_randgens.push_back(std::mt19937_64(link_seeds[i]));
            }
        }
        // 3. initialize std::vector<LinkDistParams> link_dist_params_;  use LinkDistParams(args, seed)
        // args is propagation_latency_distname

        if(is_rnd_link){
            link_dist_params_.resize(link_size);
            for (uint32_t i = 0; i < link_size; i++) {
                link_dist_params_[i] = LinkDistParams(propagation_latency_distname, link_seeds[i]);
            }
        }
        // if is is_rnd_link info print all links parameters in a list
        // if uniform, print uniform_r
        // if poisson print lamba
        // if pareto print k
        if(is_rnd_link){
            // std::cout << "Link size: " << link_size << ", is_rnd_link: " << (is_rnd_link ? "true" : "false") << std::endl;

            // std::ostringstream oss;
            oss << "Link distribution parameters: [";
            for (uint32_t i = 0; i < link_size; i++) {
                switch (link_dist_params_[i].type) {
                    case LinkDistType::UNIFORM:
                        oss << link_dist_params_[i].uniform_r;
                        break;
                    case LinkDistType::POISSON:
                        oss << link_dist_params_[i].lambda;
                        break;
                    case LinkDistType::PARETO:
                        oss << link_dist_params_[i].k;
                        break;
                    default:
                        oss << "UNKNOWN distribution type";
                        break;
                }
                if (i != link_size - 1) {
                    oss << ", ";
                }
            }
            oss << "]";
            Util::dumpNormalMsg(instance_name_, oss.str());
        }
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
            // std::cout<<"lambda: "<<lambda<<"lbound: "<<propagation_latency_lbound_us_<<" rbound: "<<propagation_latency_rbound_us_<<std::endl;
            assert(lambda > 0.0 && "Poisson distribution lambda must be positive");
            assert(lambda >= (double)propagation_latency_lbound_us_ && lambda <= (double)propagation_latency_rbound_us_ && "Poisson distribution lambda must be within [lbound, rbound]");
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
        if(is_rnd_link){//using constant latency matrix
            return genPropagationLatency_of_j_rnd_(j);
        }
        else {
            return genPropagationLatency_of_j_(j);
        }
    }

    uint32_t PropagationSimulatorParam::genPropagationLatency_of_j_(int j)
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
    
    uint32_t PropagationSimulatorParam::genPropagationLatency_of_j_rnd_(int j)
    {
        // NOTE: NO need to acquire write lock here, which has been done in PropagationSimulatorParam::push() and PropagationSimulatorParam::genPropagationLatency()

        assert(j >= 0 && j < (int)p2p_latency_array.size());
        uint32_t propagation_latency = 0;
        // get seed and link dist params for j
        LinkDistParams link_dist_param = link_dist_params_[j];
        std::mt19937_64 link_randgen = link_randgens[j];
        if(link_dist_param.type == LinkDistType::UNIFORM){
            std::uniform_int_distribution<uint32_t> uniform_dist(link_dist_param.uniform_l, link_dist_param.uniform_r);
            propagation_latency = uniform_dist(link_randgen);

            assert(propagation_latency >= link_dist_param.uniform_l);
            assert(propagation_latency <= link_dist_param.uniform_r);

        } else if(link_dist_param.type == LinkDistType::POISSON){
            std::poisson_distribution<uint32_t> poisson_dist(link_dist_param.lambda);
            propagation_latency = poisson_dist(link_randgen);
    
            // 截断处理：超出范围时取边界值
            if (propagation_latency < link_dist_param.l) {
                propagation_latency = link_dist_param.l;
            } else if (propagation_latency > link_dist_param.r) {
                propagation_latency = link_dist_param.r;
            }
        } else if(link_dist_param.type == LinkDistType::PARETO){

            double k = link_dist_param.k;
            assert(k > 0.0 && "Pareto distribution k must be positive");

            const double L = link_dist_param.l;
            const double U = link_dist_param.r;

            std::uniform_real_distribution<double> uniform_dist(0.0, 1.0);
            double u = uniform_dist(link_randgen);
            if (u >= 1.0) u = 1.0 - 1e-10;

            const double factor = 1.0 - pow(L / U, k);
            const double term = 1.0 - u * factor;
            assert(term > 0.0 && "Invalid term in Pareto calculation (must be positive)");

            const double raw_delay = L * pow(term, -1.0 / k);
            propagation_latency = static_cast<uint32_t>(raw_delay);


            assert(propagation_latency >= L && propagation_latency <= U 
                && "Pareto generated latency out of [left, right] range");
        } else{
            // error
            std::ostringstream oss;
            oss << "propagation latency distribution link_dist_param.type " << " is not supported!" << std::endl;
            Util::dumpErrorMsg(instance_name_, oss.str());
            exit(1);
        }
        

        assert(propagation_latency > 0 && "Generated propagation latency must be positive");

        return propagation_latency;
    }
} // namespace covered