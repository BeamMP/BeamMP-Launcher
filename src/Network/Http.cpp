///
/// Created by Anonymous275 on 7/18/2020
///
#define CURL_STATICLIB
#include "Security/Game.h"
#include "Security/Enc.h"
#include "Curl/curl.h"
#include <iostream>
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp){
    ((std::string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}
std::string HTTP_REQUEST(const std::string& IP,int port){
    CURL *curl;
    CURLcode res;
    std::string readBuffer;
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL, IP.c_str());
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl, CURLOPT_PORT, port);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(res != CURLE_OK)return "-1";
    }
    curl_global_cleanup();
    return readBuffer;
}

int nb_bar;
double last_progress, progress_bar_adv;
int progress_bar (void *bar, double t, double d){
    if(last_progress != round(d/t*100)){
        nb_bar = 25;
        progress_bar_adv = round(d/t*nb_bar);
        std::cout<<"\r";
        std::cout<<Sec("Progress : [ ");
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
        fopen_s(&out->stream,out->filename,Sec("wb"));
        if(!out->stream)return -1;
    }
    return fwrite(buffer, size, nmemb, out->stream);
}
int Download(const std::string& URL,const std::string& Path,bool close){
    CURL *curl;
    CURLcode res;
    struct File file = {Path.c_str(),nullptr};
    //curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
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
        curl_easy_cleanup(curl);
        if(res != CURLE_OK)return res;
    }
    if(file.stream)fclose(file.stream);
    if(!close)SecureMods();
    curl_global_cleanup();
    std::cout << std::endl;
    return -1;
}
