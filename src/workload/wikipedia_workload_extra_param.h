/*
 * WikipediaWorkloadExtraParam: extra parameters of wikipedia image/text workloads.
 * 
 * By Siyuan Sheng (2024.02.15).
 */

#ifndef WIKIPEDIA_WORKLOAD_EXTRA_PARAM_H
#define WIKIPEDIA_WORKLOAD_EXTRA_PARAM_H

#include <string>

#include "workload/workload_extra_param_base.h"

namespace covered
{
    class WikipediaWorkloadExtraParam : public WorkloadExtraParamBase
    {
    public:
        static const std::string WIKI_TRACE_TEXT_TYPE;
        static const std::string WIKI_TRACE_IMAGE_TYPE;

        WikipediaWorkloadExtraParam(const std::string& wiki_trace_type);
        virtual ~WikipediaWorkloadExtraParam();

        std::string getWikiTraceType() const;

        virtual const WikipediaWorkloadExtraParam& operator=(const WikipediaWorkloadExtraParam& other);
    private:
        static const std::string kClassName;

        std::string wiki_trace_type_;
    };
}

#endif