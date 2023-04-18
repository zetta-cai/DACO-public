/*
 * Key: a general key with variable length to encapsulate underlying workload generators.
 * 
 * By Siyuan Sheng (2023.04.18).
 */

#ifndef KEY_H
#define KEY_H

namespace covered
{
    class Key
    {
    public:
        Key(const std::string& keystr);
        ~Key();
    private:
        static const std::string kClassName;

        std::string keystr_;
    }
}

#endif