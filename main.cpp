////
//// Created by Anonymous275 on 3/3/2020.
////

#include <windows.h>
#include <iostream>
#include <string>
#include <tchar.h>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
std::string keyName;

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

std::string QueryKey(HKEY hKey)
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
                if(data.find(':') != std::string::npos){
                    return data.substr(0,data.length()-2);
                }
                if(key == "Name" && data == "BeamNG.drive"){
                    return "check";
                }
            }
        }
    }else{
        std::cout << "Error Failed to Find Beamng\n";
    }
    delete [] buffer;
    return "";
}
void start();

void getPath(){
    HKEY hKey;
    LONG dwRegOPenKey = RegOpenKeyEx(HKEY_CLASSES_ROOT, HTA("6265616d6e675c44656661756c7449636f6e").c_str()/*"beamng\\DefaultIcon"*/, 0, KEY_READ, &hKey);
    if(dwRegOPenKey != ERROR_SUCCESS){
        std::cout << "Error #1\n";
    }else{
        keyName = QueryKey(hKey);
        std::cout << "full path : " << keyName << std::endl;
    }
    RegCloseKey(hKey);
}

int main()
{
    //Security
    HKEY hKey;
    LONG dwRegOPenKey = RegOpenKeyEx(HKEY_CURRENT_USER, HTA("536f6674776172655c56616c7665").c_str(), 0, KEY_READ, &hKey);
    if(dwRegOPenKey != ERROR_SUCCESS){
        std::cout << HTA("4572726f7220506c6561736520436f6e7461637420537570706f7274210a");
    }else {
        dwRegOPenKey = RegOpenKeyEx(HKEY_CURRENT_USER,
                                    HTA("536f6674776172655c56616c76655c537465616d5c417070735c323834313630").c_str(), 0,
                                    KEY_READ, &hKey);
        if (dwRegOPenKey != ERROR_SUCCESS) {
            std::cout << HTA("796f7520646f206e6f74206f776e207468652067616d65206f6e2074686973206d616368696e65210a");
        } else {
            keyName = QueryKey(hKey);
            if (!keyName.empty()) {
                std::cout << HTA("796f75206f776e207468652067616d65206f6e2074686973206d616368696e65210a");
                getPath();
            } else {
                std::cout << HTA("796f7520646f206e6f74206f776e207468652067616d65206f6e2074686973206d616368696e65210a");
            }
        }
        //Software\Classes\beamng\DefaultIcon
        RegCloseKey(hKey);
    }

    /// Update, Mods ect...

    //Start(); //Proxy main start

    system("pause");
    return 0;
}