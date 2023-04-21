/*
 * Key: a general key with variable length to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef KEY_H
#define KEY_H

#include <string>

namespace covered
{
    class Key
    {
    public:
        Key();
        Key(const std::string& keystr);
        ~Key();

        std::string getKeystr();
    private:
        static const std::string kClassName;

        std::string keystr_;
    };
}

#endif