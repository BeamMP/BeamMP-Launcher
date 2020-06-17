///
/// Created by Anonymous275 on 3/16/2020
///
#include <filesystem>
#include <Windows.h>
#include <string>
#include <vector>
#include <array>
#include <thread>

#define MAX_KEY_LENGTH 255
#define MAX_VALUE_NAME 16383

void Exit(const std::string& Msg);
int TraceBack = 0;

std::vector<std::string> SData;


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
    if (cSubKeys)
    {
        for (i=0; i<cSubKeys; i++)
        {
            cbName = MAX_KEY_LENGTH;
            retCode = RegEnumKeyEx(hKey, i,
                                   achKey,
                                   &cbName,
                                   nullptr,
                                   nullptr,
                                   nullptr,
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
                                   nullptr,
                                   nullptr,
                                   nullptr,
                                   nullptr);

            if (retCode == ERROR_SUCCESS )
            {
                DWORD lpData = cbMaxValueData;
                buffer[0] = '\0';
                LONG dwRes = RegQueryValueEx(hKey, achValue, nullptr, nullptr, buffer, &lpData);
                std::string data = reinterpret_cast<const char *const>(buffer);
                std::string key = achValue;
                switch (ID){
                    case 1: if(key == HTA("537465616d50617468"))return data;break;
                    case 2: if(key == HTA("4e616d65") && data == HTA("4265616d4e472e6472697665"))return data;break;
                    case 3: return data.substr(0,data.length()-2);
                    case 4: if(key == HTA("75736572706174685f6f76657272696465"))return data;
                    default: break;
                }
            }
        }
    }
    delete [] buffer;
    return "";
}
namespace fs = std::experimental::filesystem;
void FileList(std::vector<std::string>&a,const std::string& Path){
    for (const auto &entry : fs::directory_iterator(Path)) {
        int pos = entry.path().filename().string().find('.');
        if (pos != std::string::npos) {
            a.emplace_back(entry.path().string());
        }else FileList(a,entry.path().string());
    }
}
bool Continue = false;
void Find(const std::string& FName,const std::string& Path){
    std::vector<std::string> FS;
    FileList(FS,Path);
    for(const std::string&a : FS){
        if(a.find(FName)!=std::string::npos)Continue = true;
    }
    FS.clear();
}
void ExitError(){
    std::string MSG2 = HTA("4572726f722120506c6561736520436f6e7461637420537570706f7274");
    Exit(MSG2 + " Code 2");
}
void Check(){
    /*.HKEY_CURRENT_USER\Software\Valve\Steam
    HKEY_CURRENT_USER\\Software\Valve\Steam\Apps\284160
    HKEY_CLASSES_ROOT\\beamng\\DefaultIcon */
    //Sandbox Scramble technique
    std::string Result;
    std::string K1 = HTA("536f6674776172655c56616c76655c537465616d");
    std::string K2 = HTA("536f6674776172655c56616c76655c537465616d5c417070735c323834313630");
    std::string K3 = HTA("6265616d6e675c44656661756c7449636f6e");
    std::string MSG1 = HTA("4572726f722120796f7520646f206e6f74206f776e204265616d4e4721"); //Error! you do not own BeamNG!
    std::string MSG2 = HTA("4572726f722120506c6561736520436f6e7461637420537570706f7274"); //Error! Please Contact Support
    std::string MSG3 = HTA("596f7520646f206e6f74206f776e207468652067616d65206f6e2074686973206d616368696e6521"); //You do not own the game on this machine!
    //std::string MSG = HTA("5761726e696e672120796f75206f776e207468652067616d6520627574206120637261636b65642067616d652077617320666f756e64206f6e20796f7572206d616368696e6521");
    //not used : Warning! you own the game but a cracked game was found on your machine!
    HKEY hKey;
    LONG dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K1.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 1);
        if(Result.empty()){Exit(MSG1 + " Code 1");}
        SData.push_back(Result);
        Result += HTA("2f7573657264617461");
        struct stat buffer{};
        if(stat(Result.c_str(), &buffer) == 0){
            auto *F = new std::thread(Find,HTA("3238343136302e6a736f6e"),Result);
            F->join();
            delete F;
            if(!Continue)Exit(MSG2 + " Code 2");
        }else Exit(MSG2 + ". Code: 3");
        Result.clear();
        TraceBack++;
    }else{Exit(MSG2 + ". Code: 4");}
    K1.clear();
    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_CURRENT_USER, K2.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 2);
        if(Result.empty()){Exit(MSG1 + " Code 3");}
        SData.push_back(Result);
        TraceBack++;
    }else{Exit(MSG3);}
    K2.clear();
    RegCloseKey(hKey);
    dwRegOPenKey = OpenKey(HKEY_CLASSES_ROOT, K3.c_str(), &hKey);
    if(dwRegOPenKey == ERROR_SUCCESS) {
        Result = QueryKey(hKey, 3);
        if(Result.empty()){
            Exit(MSG2 + ". Code: 5");
        }
        SData.push_back(Result);
        TraceBack++;
    }else{Exit(MSG2+ ". Code : 5");}
    //Memory Cleaning
    K3.clear();
    //MSG.clear();
    MSG1.clear();
    MSG2.clear();
    MSG3.clear();
    Result.clear();
    RegCloseKey(hKey);
}

std::string HWID(){
    SYSTEM_INFO siSysInfo;
    GetSystemInfo(&siSysInfo);
    int I1 = siSysInfo.dwOemId;
    int I2 = siSysInfo.dwNumberOfProcessors;
    int I3 = siSysInfo.dwProcessorType;
    int I4 = siSysInfo.dwActiveProcessorMask;
    int I5 = siSysInfo.wProcessorLevel;
    int I6 = siSysInfo.wProcessorRevision;
    return std::to_string((I1*I2+I3)*(I4*I5+I6));
}

char* HashMD5(char* data, DWORD *result)
{
    DWORD dwStatus = 0;
    DWORD cbHash = 16;
    int i = 0;
    HCRYPTPROV cryptProv;
    HCRYPTHASH cryptHash;
    BYTE hash[16];
    char hex[] = "0123456789abcdef";
    char *strHash;
    strHash = (char*)malloc(500);
    memset(strHash, '\0', 500);
    if (!CryptAcquireContext(&cryptProv, NULL, MS_DEF_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT))
    {
        dwStatus = GetLastError();
        printf("CryptAcquireContext failed: %d\n", dwStatus);
        *result = dwStatus;
        return nullptr;
    }
    if (!CryptCreateHash(cryptProv, CALG_MD5, 0, 0, &cryptHash))
    {
        dwStatus = GetLastError();
        printf("CryptCreateHash failed: %d\n", dwStatus);
        CryptReleaseContext(cryptProv, 0);
        *result = dwStatus;
        return nullptr;
    }
    if (!CryptHashData(cryptHash, (BYTE*)data, strlen(data), 0))
    {
        dwStatus = GetLastError();
        printf("CryptHashData failed: %d\n", dwStatus);
        CryptReleaseContext(cryptProv, 0);
        CryptDestroyHash(cryptHash);
        *result = dwStatus;
        return nullptr;
    }
    if (!CryptGetHashParam(cryptHash, HP_HASHVAL, hash, &cbHash, 0))
    {
        dwStatus = GetLastError();
        printf("CryptGetHashParam failed: %d\n", dwStatus);
        CryptReleaseContext(cryptProv, 0);
        CryptDestroyHash(cryptHash);
        *result = dwStatus;
        return nullptr;
    }
    for (i = 0; i < cbHash; i++)
    {
        strHash[i * 2] = hex[hash[i] >> 4];
        strHash[(i * 2) + 1] = hex[hash[i] & 0xF];
    }
    CryptReleaseContext(cryptProv, 0);
    CryptDestroyHash(cryptHash);
    return strHash;
}
std::string getHardwareID()
{
    DWORD err;
    std::string hash = HashMD5((char*)HWID().c_str(), &err);
    return hash;
}
