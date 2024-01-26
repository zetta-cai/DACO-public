/*
 * CooperationCustomFuncParamBase: base class of custom function parameters for cooperation module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef COOPERATION_CUSTOM_FUNC_PARAM_BASE
#define COOPERATION_CUSTOM_FUNC_PARAM_BASE

#include <string>

namespace covered
{
    class CooperationCustomFuncParamBase
    {
    public:
        CooperationCustomFuncParamBase();
        virtual ~CooperationCustomFuncParamBase();
    private:
        static const std::string kClassName;
    };
}

#endif