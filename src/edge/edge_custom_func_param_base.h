/*
 * EdgeCustomFuncParamBase: base class of custom function parameters for edge module.
 *
 * By Siyuan Sheng (2024.01.26).
 */

#ifndef EDGE_CUSTOM_FUNC_PARAM_BASE
#define EDGE_CUSTOM_FUNC_PARAM_BASE

#include <string>

namespace covered
{
    class EdgeCustomFuncParamBase
    {
    public:
        EdgeCustomFuncParamBase();
        virtual ~EdgeCustomFuncParamBase();
    private:
        static const std::string kClassName;
    };
}

#endif