///
/// Created by Anonymous275 on 12/26/21
/// Copyright (c) 2021-present Anonymous275 read the LICENSE file for more info.
///

#include "Launcher.h"

int Entry (int argc, char *argv[]);

int main(int argc, char* argv[]) {
    Launcher launcher(argc, argv);
    return Entry(argc, argv);
}
