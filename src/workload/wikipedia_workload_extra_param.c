#include "workload/wikipedia_workload_extra_param.h"

#include "common/util.h"

namespace covered
{
    const std::string WikipediaWorkloadExtraParam::WIKI_TRACE_TEXT_TYPE("text");
    const std::string WikipediaWorkloadExtraParam::WIKI_TRACE_IMAGE_TYPE("image");

    const std::string WikipediaWorkloadExtraParam::kClassName("WikipediaWorkloadExtraParam");

    WikipediaWorkloadExtraParam::WikipediaWorkloadExtraParam(const std::string& wiki_trace_type) : WorkloadExtraParamBase()
    {
        if (wiki_trace_type != WIKI_TRACE_TEXT_TYPE && wiki_trace_type != WIKI_TRACE_IMAGE_TYPE)
        {
            Util::dumpErrorMsg(kClassName, "invalid wiki trace type: " + wiki_trace_type);
            exit(1);
        }

        wiki_trace_type_ = wiki_trace_type;
    }

    WikipediaWorkloadExtraParam::~WikipediaWorkloadExtraParam()
    {
    }

    std::string WikipediaWorkloadExtraParam::getWikiTraceType() const
    {
        return wiki_trace_type_;
    }

    const WikipediaWorkloadExtraParam& WikipediaWorkloadExtraParam::operator=(const WikipediaWorkloadExtraParam& other)
    {
        WorkloadExtraParamBase::operator=(other);

        wiki_trace_type_ = other.wiki_trace_type_;

        return *this;
    }
}