/*
 * EdgescaleParam: store CLI parameters for edgescale dynamic configurations.
 *
 * See NOTEs in CommonParam.
 * 
 * By Siyuan Sheng (2023.08.03).
 */

#ifndef EDGESCALE_PARAM_H
#define EDGESCALE_PARAM_H

#include <string>

namespace covered
{
    class EdgescaleParam
    {
    public:
        static void setParameters(const uint32_t& edgecnt);

        static uint32_t getEdgecnt();

        static bool isValid();

        static std::string toString();
    private:
        static const std::string kClassName;

        static void checkIsValid_();

        static void verifyIntegrity_();

        static bool is_valid_;

        static uint32_t edgecnt_;
    };
}

#endif