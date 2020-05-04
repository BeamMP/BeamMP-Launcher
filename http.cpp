///
/// Created by Anonymous275 on 3/17/2020
///

#define CURL_STATICLIB
#include "curl/curl.h"
#include <iostream>

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

        if(round(d/t*100) < 10)
        { std::cout<<"0"<<round(d/t*100)<<"% ]"; }
        else
        { std::cout<<round(d/t*100)<<"% ] "; }

        std::cout<<"[";
        for(int i = 0 ; i <= progress_bar_adv ; i++)
        { std::cout<<"#"; }
        for(int i = 0 ; i < nb_bar - progress_bar_adv; i++)
        { std::cout<<"."; }

        std::cout<<"]";
        last_progress = round(d/t*100);
    }
    return 0;
}

struct FtpFile {
    const char *filename;
    FILE *stream;
};

static size_t my_fwrite(void *buffer, size_t size, size_t nmemb,
                        void *stream)
{
    auto *out = (struct FtpFile *)stream;
    if(!out->stream) {
        fopen_s(&out->stream,out->filename,"wb");
        if(!out->stream)
            return -1;
    }
    return fwrite(buffer, size, nmemb, out->stream);
}

void Download(const std::string& URL,const std::string& Path)
{

    CURL *curl;
    CURLcode res;
    struct FtpFile ftpfile = {
            Path.c_str(),
            nullptr
    };

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl = curl_easy_init();
    if(curl) {
        curl_easy_setopt(curl, CURLOPT_URL,URL.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, my_fwrite);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ftpfile);

        curl_easy_setopt(curl, CURLOPT_NOPROGRESS, FALSE);
        //progress_bar : the fonction for the progress bar
        curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, progress_bar);

        curl_easy_setopt(curl, CURLOPT_USE_SSL, CURLUSESSL_ALL);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
        if(CURLE_OK != res) {
            fprintf(stderr, "Failed to download! Code : %d\n", res);
        }
    }
    if(ftpfile.stream)fclose(ftpfile.stream);
    curl_global_cleanup();
    std::cout << std::endl;
}