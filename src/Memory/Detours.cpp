///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#define WIN32_LEAN_AND_MEAN
#include "Memory/Detours.h"
#include <windows.h>
#include "detours/detours.h"

void Detours::Attach() {
    if(!Attached){
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourAttach(&targetPtr,detourFunc);
        DetourTransactionCommit();
        Attached = true;
    }
}

void Detours::Detach() {
    if(Attached){
        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());
        DetourDetach(&targetPtr,detourFunc);
        DetourTransactionCommit();
        Attached = false;
    }
}
