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

std::string HTA(const std::string& hex)
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
                if(strcmp(achKey,HTA("537465616d2041707020323834313630").c_str()) == 0){
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
                    case 1: if(key == HTA("496e7374616c6c4c6f636174696f6e") && (data.find(HTA("4265616d4e47")) != std::string::npos)) {return data;} break;
                    case 2: if(key == HTA("4e616d65") && data == HTA("4265616d4e472e6472697665")) {return data;} break;
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
    /*HKEY_CLASSES_ROOT\\beamng\\DefaultIcon
    HKEY_LOCAL_MACHINE\\SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\Steam App 284160
    HKEY_CURRENT_USER\\Software\Valve\Steam\Apps\284160*/
    //Sandbox Scramble technique
    std::string K1 = HTA("534f4654574152455c4d6963726f736f66745c57696e646f77735c43757272656e7456657273696f6e5c556e696e7374616c6c5c");
    std::string K2 = HTA("536f6674776172655c56616c76655c537465616d5c417070735c323834313630");
    std::string K3 = HTA("6265616d6e675c44656661756c7449636f6e");
    std::string MSG1 = HTA("4572726f722120796f7520646f206e6f74206f776e204265616d4e4721"); //Error! you do not own BeamNG!
    std::string MSG2 = HTA("4572726f722120506c6561736520436f6e7461637420537570706f7274"); //Error! Please Contact Support
    std::string MSG3 = HTA("596f7520646f206e6f74206f776e207468652067616d65206f6e2074686973206d616368696e6521"); //You do not own the game on this machine!
    std::string MSG = HTA("5761726e696e672120796f75206f776e207468652067616d6520627574206120637261636b65642067616d652077617320666f756e64206f6e20796f7572206d616368696e6521");
    //Warning! you own the game but a cracked game was found on your machine!

    HKEY hKey;
    LONG dwRegOPenKey = OpenKey(HKEY_LOCAL_MACHINE, K1.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 0);
        if(Result.empty()){Exit(MSG1);}
        Data.push_back(Result);
        K1 += Result;
        TraceBack++;
    }else{Exit(MSG2);}

    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_LOCAL_MACHINE, K1.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 1);
        if(Result.empty()){Exit(MSG1);}
        Data.push_back(Result);
        TraceBack++;
    }else{Exit(MSG3);}
    K1.clear();
    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K2.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 2);
        if(Result.empty()){Exit(MSG1);}
        Data.push_back(Result);
        TraceBack++;
    }else{Exit(MSG3);}
    K2.clear();
    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_CLASSES_ROOT, K3.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 3);
        if(Result.empty()){
            Exit(MSG2);
        }else if(Result.find(Data.at(1)) != 0){
            Exit(MSG);
        }
        TraceBack++;
    }
    //Memory Cleaning
    K3.clear();
    MSG.clear();
    MSG1.clear();
    MSG2.clear();
    MSG3.clear();
    RegCloseKey(hKey);
    return Data;
}
