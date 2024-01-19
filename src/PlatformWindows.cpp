#include "Platform.h"

#if defined(PLATFORM_WINDOWS)
#include <spdlog/spdlog.h>
#include <windows.h>

void plat::ReLaunch(int argc, char** argv) {
    std::string Arg;
    for (int c = 2; c <= argc; c++) {
        Arg += " ";
        Arg += argv[c - 1];
    }
    system("cls");
    ShellExecute(nullptr, "runas", argv[0], Arg.c_str(), nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(), 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void plat::URelaunch(int argc, char** argv) {
    std::string Arg;
    for (int c = 2; c <= argc; c++) {
        Arg += " ";
        Arg += argv[c - 1];
    }
    ShellExecute(nullptr, "open", argv[0], Arg.c_str(), nullptr, SW_SHOWNORMAL);
    ShowWindow(GetConsoleWindow(), 0);
    std::this_thread::sleep_for(std::chrono::seconds(1));
    exit(1);
}
void plat::set_console_title(const std::string& title) {
    SetConsoleTitleA(title);
}
void plat::clear_screen() {
    system("cls");
}
LONG OpenKey(HKEY root,const char* path,PHKEY hKey){
    return RegOpenKeyEx(root, reinterpret_cast<LPCSTR>(path), 0, KEY_READ, hKey);
}
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
std::string QueryKey(HKEY hKey,int ID){
    TCHAR    achKey[MAX_KEY_LENGTH];   // buffer for subkey name
    DWORD    cbName;                   // size of name string
    TCHAR    achClass[MAX_PATH] = TEXT("");  // buffer for class name
    DWORD    cchClassName = MAX_PATH;  // size of class string
    DWORD    cSubKeys=0;               // number of subkeys
    DWORD    cbMaxSubKey;              // longest subkey size
    DWORD    cchMaxClass;              // longest class string
    DWORD    cValues;              // number of values for key
    DWORD    cchMaxValue;          // longest value name
    DWORD    cbMaxValueData;       // longest value data
    DWORD    cbSecurityDescriptor; // size of security descriptor
    FILETIME ftLastWriteTime;      // last write time

    DWORD i, retCode;

    TCHAR  achValue[MAX_VALUE_NAME];
    DWORD cchValue = MAX_VALUE_NAME;

    retCode = RegQueryInfoKey(
        hKey,                    // key handle
        achClass,                // buffer for class name
        &cchClassName,           // size of class string
        nullptr,                    // reserved
        &cSubKeys,               // number of subkeys
        &cbMaxSubKey,            // longest subkey size
        &cchMaxClass,            // longest class string
        &cValues,                // number of values for this key
        &cchMaxValue,            // longest value name
        &cbMaxValueData,         // longest value data
        &cbSecurityDescriptor,   // security descriptor
        &ftLastWriteTime);       // last write time

    BYTE* buffer = new BYTE[cbMaxValueData];
    ZeroMemory(buffer, cbMaxValueData);
    if (cSubKeys){
        for (i=0; i<cSubKeys; i++){
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKey, i,achKey,&cbName,nullptr,nullptr,nullptr,&ftLastWriteTime);
            if (retCode == ERROR_SUCCESS){
                if(strcmp(achKey,"Steam App 284160") == 0){
                    return achKey;
                }
            }
        }
    }
    if (cValues){
        for (i=0, retCode = ERROR_SUCCESS; i<cValues; i++){
            cchValue = MAX_VALUE_NAME;
            achValue[0] = '\0';
            retCode = RegEnumValue(hKey, i,achValue,&cchValue,nullptr,nullptr,nullptr,nullptr);
            if (retCode == ERROR_SUCCESS ){
                DWORD lpData = cbMaxValueData;
                buffer[0] = '\0';
                LONG dwRes = RegQueryValueEx(hKey, achValue, nullptr, nullptr, buffer, &lpData);
                std::string data = (char *)(buffer);
                std::string key = achValue;

                switch (ID){
                case 1: if(key == "SteamExe"){
                        auto p = data.find_last_of("/\\");
                        if(p != std::string::npos){
                            return data.substr(0,p);
                        }
                    }
                    break;
                case 2: if(key == "Name" && data == "BeamNG.drive")return data;break;
                case 3: if(key == "rootpath")return data;break;
                case 4: if(key == "userpath_override")return data;
                case 5: if(key == "Local AppData")return data;
                default: break;
                }
            }
        }
    }
    delete [] buffer;
    return "";
}
std::string plat::get_game_dir_magically() {
    std::string Result;
    std::string K3 = R"(Software\BeamNG\BeamNG.drive)";
    HKEY hKey;
    LONG dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K3.c_str(), &hKey);
    if (dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 3);
        if (Result.empty()) {
            spdlog::error("Couldn't query key from HKEY_CURRENT_USER\\Software\\BeamNG\\BeamNG.drive");
            Result = "";
        }
    } else {
        spdlog::error("Couldn't open HKEY_CURRENT_USER\\Software\\BeamNG\\BeamNG.drive");
    }
    RegCloseKey(hKey);
    return Result;
}

static int CALLBACK BrowseCallbackProc(HWND hwnd, UINT uMsg, LPARAM lParam, LPARAM lpData) {

    if (uMsg == BFFM_INITIALIZED) {
        std::string tmp = (const char*)lpData;
        std::cout << "path: " << tmp << std::endl;
        SendMessage(hwnd, BFFM_SETSELECTION, TRUE, lpData);
    }

    return 0;
}
std::string plat::ask_for_folder() {
    TCHAR path[MAX_PATH];

    const char* path_param = saved_path.c_str();

    BROWSEINFO bi = { 0 };
    bi.lpszTitle = ("Browse for folder...");
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    bi.lpfn = BrowseCallbackProc;
    bi.lParam = (LPARAM)path_param;

    LPITEMIDLIST pidl = SHBrowseForFolder(&bi);

    if (pidl != 0) {
        // get the name of the folder and put it in path
        SHGetPathFromIDList(pidl, path);

        // free memory used
        IMalloc* imalloc = 0;
        if (SUCCEEDED(SHGetMalloc(&imalloc))) {
            imalloc->Free(pidl);
            imalloc->Release();
        }

        return path;
    }

    return "";
}

#endif