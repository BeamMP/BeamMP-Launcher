///
/// Created by Anonymous275 on 3/16/2020
///

#include <Windows.h>
#include <string>
#include <vector>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
void Exit(const std::string& Msg);
int TraceBack = 0;

std::vector<std::string> Data;
std::string Result;

std::string HTA(std::string hex)
{
    std::string ascii;
    for (size_t i = 0; i < hex.length(); i += 2)
    {
        std::string part = hex.substr(i, 2);
        char ch = stoul(part, nullptr, 16);
        ascii += ch;
    }
    return ascii;
}
LONG OpenKey(HKEY root,const char* path,PHKEY hKey){
    return RegOpenKeyEx(root, reinterpret_cast<LPCSTR>(path), 0, KEY_READ, hKey);
}



std::string QueryKey(HKEY hKey,int ID)
{
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
            NULL,                    // reserved
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
    if (cSubKeys)
    {
        for (i=0; i<cSubKeys; i++)
        {
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKey, i,
                                   achKey,
                                   &cbName,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &ftLastWriteTime);
            if (retCode == ERROR_SUCCESS)
            {
                if(strcmp(achKey,"Steam App 284160") == 0){
                    return achKey;
                }
            }
        }
    }
    if (cValues)
    {
        for (i=0, retCode = ERROR_SUCCESS; i<cValues; i++)
        {
            cchValue = MAX_VALUE_NAME;
            achValue[0] = '\0';
            retCode = RegEnumValue(hKey, i,
                                   achValue,
                                   &cchValue,
                                   NULL,
                                   NULL,
                                   NULL,
                                   NULL);

            if (retCode == ERROR_SUCCESS )
            {
                DWORD lpData = cbMaxValueData;
                buffer[0] = '\0';
                LONG dwRes = RegQueryValueEx(hKey, achValue, 0, NULL, buffer, &lpData);
                std::string data = reinterpret_cast<const char *const>(buffer);
                std::string key = achValue;
                switch (ID){
                    case 1: if(key == "InstallLocation" && (data.find("BeamNG") != std::string::npos)) {return data;} break;
                    case 2: if(key == "Name" && data == "BeamNG.drive") {return data;} break;
                    case 3: return data.substr(0,data.length()-2); break;
                    default: break;
                }
                /*if(data.find(':') != std::string::npos){
                    return data.substr(0,data.length()-2);
                }*/
            }
        }
    }
    delete [] buffer;
    return "";
}

std::vector<std::string> Check(){
    std::string K1 = R"(SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\)";
    std::string K2 = R"(Software\Valve\Steam\Apps\284160)";
    std::string K3 = R"(beamng\DefaultIcon)";
    HKEY hKey;
    LONG dwRegOPenKey = OpenKey(HKEY_LOCAL_MACHINE, K1.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 0);
        if(Result.empty()){Exit("Error! you do not own BeamNG!");}
        Data.push_back(Result);
        K1 += Result;
        TraceBack++;
    }else{Exit("Error! Please Contact Support");}

    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_LOCAL_MACHINE, K1.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 1);
        if(Result.empty()){Exit("Error! you do not own BeamNG!");}
        Data.push_back(Result);
        TraceBack++;
    }else{Exit("You do not own the game on this machine!");}
    K1.clear();
    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K2.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 2);
        if(Result.empty()){Exit("Error! you do not own BeamNG!");}
        Data.push_back(Result);
        TraceBack++;
    }else{Exit("You do not own the game on this machine!");}
    K2.clear();
    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_CLASSES_ROOT, K3.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 3);
        if(Result.empty()){
            Exit("Error! Please Contact Support");
        }else if(Result.find(Data.at(1)) != 0){
            Exit("Warning! you own the game but a cracked game was found on your machine!");
        }
        TraceBack++;
    }
    K3.clear();
    RegCloseKey(hKey);
    return Data;
}
