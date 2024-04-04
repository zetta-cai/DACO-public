/*
 * Hitflag: the type of cache hits (break the mutual dependency of declarations between statistics classes and MessageBase).
 * 
 * By Siyuan Sheng (2024.04.04).
 */

#ifndef HITFLAG_H
#define HITFLAG_H

namespace covered
{
    enum Hitflag
    {
        kLocalHit = 1, // Hit local edge cache of closest edge node
        kCooperativeHit, // Hit cooperative edge cache of some target edge node with valid object
        kCooperativeInvalid, // Hit cooperative edge cache of some target edge node yet with invalid object (only used in edge nodes)
        kGlobalMiss // Miss all edge nodes
    };
}

#endif