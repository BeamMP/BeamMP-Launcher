///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///
#define WIN32_LEAN_AND_MEAN
#include "Memory/Memory.h"
#include <MinHook.h>

#pragma once
template <class FuncType>
class Hook {
    FuncType targetPtr;
    FuncType detourFunc;
    bool Attached = false;
public:

    Hook(FuncType src, FuncType dest) : targetPtr(src), detourFunc(dest) {
        auto status = MH_CreateHook((void*)targetPtr, (void*)detourFunc, (void**)&Original);
        if(status != MH_OK) {
            Memory::Print(std::string("MH Error -> ") + MH_StatusToString(status));
            return;
        }
    }

    void Enable() {
        if(!Attached){
            auto status = MH_EnableHook((void*)targetPtr);
            if(status != MH_OK) {
                Memory::Print(std::string("MH Error -> ") + MH_StatusToString(status));
                return;
            }
            Attached = true;
        }
    }

    void Disable() {
        if(Attached){
            auto status = MH_DisableHook((void*)targetPtr);
            if(status != MH_OK) {
                Memory::Print(std::string("MH Error -> ") + MH_StatusToString(status));
                return;
            }
            Attached = false;
        }
    }

    FuncType Original{};
};
