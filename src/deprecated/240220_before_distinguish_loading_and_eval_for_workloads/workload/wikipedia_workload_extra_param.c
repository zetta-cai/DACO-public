#include "workload/wikipedia_workload_extra_param.h"

#include "common/util.h"

namespace covered
{
    const std::string WikipediaWorkloadExtraParam::kClassName("WikipediaWorkloadExtraParam");

    WikipediaWorkloadExtraParam::WikipediaWorkloadExtraParam(const std::string& wiki_workload_name) : WorkloadExtraParamBase()
    {
        if (wiki_workload_name != Util::WIKIPEDIA_IMAGE_WORKLOAD_NAME && wiki_workload_name != Util::WIKIPEDIA_TEXT_WORKLOAD_NAME)
        {
            Util::dumpErrorMsg(kClassName, "invalid wiki workload name: " + wiki_workload_name);
            exit(1);
        }

        wiki_workload_name_ = wiki_workload_name;
    }

    WikipediaWorkloadExtraParam::~WikipediaWorkloadExtraParam()
    {
    }

    std::string WikipediaWorkloadExtraParam::getWikiWorkloadName() const
    {
        return wiki_workload_name_;
    }

    const WikipediaWorkloadExtraParam& WikipediaWorkloadExtraParam::operator=(const WikipediaWorkloadExtraParam& other)
    {
        WorkloadExtraParamBase::operator=(other);

        wiki_workload_name_ = other.wiki_workload_name_;

        return *this;
    }
}