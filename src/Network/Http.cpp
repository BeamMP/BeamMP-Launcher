// Copyright (c) 2019-present Anonymous275.
// BeamMP Launcher code is not in the public domain and is not free software.
// One must be granted explicit permission by the copyright holder in order to modify or distribute any part of the source or binaries.
// Anything else is prohibited. Modified works may not be published and have be upstreamed to the official repository.
///
/// Created by Anonymous275 on 7/18/2020
///

#include <curl/curl.h>
#include <iostream>
#include <mutex>

class CurlManager{
public:
    CurlManager(){
        curl = curl_easy_init();
    }
    ~CurlManager(){
        curl_easy_cleanup(curl);
    }
    inline CURL* Get(){
        return curl;
    }
private:
    CURL *curl;
};

static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string HTTP_REQUEST(const std::string& IP,int port){
    static thread_local CurlManager M;
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);
    CURL *curl = M.Get();
    CURLcode res;
    std::string readBuffer;
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, IP.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_PORT, port);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)return "-1";
    }
    return readBuffer;
}

int nb_bar;
double last_progress, progress_bar_adv;
int progress_bar (void *bar, double t, double d){
    if(last_progress != round(d/t*100)){
        nb_bar = 25;
        progress_bar_adv = round(d/t*nb_bar);
        std::cout<<"\r";
        std::cout<< "Progress : [ ";
        if(t!=0)std::cout<<round(d/t*100);else std::cout<<0;
        std::cout << "% ] [";
        int i;
        for(i = 0; i <= progress_bar_adv; i++)std::cout<<"#";
        for(i = 0; i < nb_bar - progress_bar_adv; i++)std::cout<<".";
        std::cout<<"]";
        last_progress = round(d/t*100);
    }
    return 0;
}
struct File {
    const char *filename;
    FILE *stream;
};
static size_t my_fwrite(void *buffer,size_t size,size_t nmemb,void *stream){
    auto *out = (struct File*)stream;
    if(!out->stream) {
        fopen_s(&out->stream,out->filename,"wb");
        if(!out->stream)return -1;
    }
    return fwrite(buffer, size, nmemb, out->stream);
}
int Download(const std::string& URL,const std::string& Path,bool close){
    static thread_local CurlManager M;
    CURL *curl = M.Get();
    CURLcode res;
    struct File file = {Path.c_str(),nullptr};
    if(curl){
        curl_easy_setopt(curl, CURLOPT_URL,URL.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_bar);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        res = curl_easy_perform(curl);
        if(res != CURLE_OK)return res;
    }
    if(file.stream)fclose(file.stream);
    std::cout << std::endl;
    return -1;
}
std::string PostHTTP(const std::string& IP, const std::string& Fields) {
    static auto *header = new curl_slist{(char*)"Content-Type: application/json"};
    static thread_local CurlManager M;
    static std::mutex Lock;
    std::scoped_lock Guard(Lock);
    CURL* curl = M.Get();
    CURLcode res;
    std::string readBuffer;

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, IP.c_str());
        curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);
        curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, Fields.size());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS, Fields.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        curl_easy_setopt(curl, CURLOPT_IPRESOLVE, CURL_IPRESOLVE_V4);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
            return "-1";
    }
    return readBuffer;
}