////
//// Created by Anonymous275 on 3/3/2020.
////

#include <windows.h>
#include <iostream>
#include <string>
#include <cstring>
#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383
std::string keyName;

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
                std::string Path = reinterpret_cast<const char *const>(buffer);
                return Path.substr(0,Path.length()-2);
            }
        }
    }else{
        std::cout << "Error Failed to Find Beamng\n";
    }
    delete [] buffer;
    return "";
}

void Start();
int main()
{
    HKEY hKey;
    LONG dwRegOPenKey = RegOpenKeyEx(HKEY_CLASSES_ROOT, "beamng\\DefaultIcon", 0, KEY_READ, &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS){
        keyName = QueryKey(hKey);
        std::cout << keyName << std::endl; //Prints the exe path
    }else{
        std::cout << "Error Failed to Find Beamng\n";
    }
    RegCloseKey(hKey);

    /// Update, Mods ect...

    Start(); //Proxy main start

    system("pause");
    return 0;
}