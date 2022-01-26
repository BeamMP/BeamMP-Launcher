///
/// Created by Anonymous275 on 1/21/22
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#pragma once
class Detours{
    void* targetPtr;
    void* detourFunc;
public:
    Detours(void* src, void* dest) : targetPtr(src), detourFunc(dest){};
    void Attach();
    void Detach();

private:
    bool Attached = false;
};
