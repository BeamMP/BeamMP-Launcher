///
/// Created by Anonymous275 on 3/17/2020
///

#define CURL_STATICLIB
#include "curl/curl.h"
#include <iostream>
#include <string>
void Exit(const std::string& Msg);
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
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
        curl_easy_setopt(curl, CURLOPT_PORT, port);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return readBuffer;
}

int nb_bar;
double last_progress, progress_bar_adv;
int progress_bar (void *bar, double t, double d)
{
    if(last_progress != round(d/t*100))
    {
        nb_bar = 25;
        progress_bar_adv = round(d/t*nb_bar);
        std::cout<<"\r";
        std::cout<<"Progress : [ ";
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

static size_t my_fwrite(void *buffer,size_t size,size_t nmemb,void *stream)
{
    auto *out = (struct File*)stream;
    if(!out->stream) {
        fopen_s(&out->stream,out->filename,"wb");
        if(!out->stream)return -1;
    }
    return fwrite(buffer, size, nmemb, out->stream);
}

void Download(const std::string& URL,const std::string& Path)
{
    CURL *curl;
    CURLcode res;
    struct File file = {
            Path.c_str(),
            nullptr
    };
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL,URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);
        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_bar);
        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(CURLE_OK != res) {
            Exit("Failed to download! Code : " + std::to_string(res));
        }
    }
    if(file.stream)fclose(file.stream);
    curl_global_cleanup();
    std::cout << std::endl;
}