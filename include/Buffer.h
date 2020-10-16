///
/// Created by Anonymous275 on 8/25/2020
///
#pragma once
void ServerParser(const std::string& Data);
class Buffer{
public:
    void Handle(const std::string& Data){
        Buf += Data;
        Manage();
    }
    void clear(){
        Buf.clear();
    }
private:
    std::string Buf;
    void Manage(){
        if(!Buf.empty()){
            std::string::size_type p;
            if (Buf.at(0) == '\n'){
                p = Buf.find('\n',1);
                if(p != -1){
                    std::string R = Buf.substr(1,p-1);
                    std::string_view B(R.c_str(),R.find(char(0)));
                    ServerParser(B.data());
                    Buf = Buf.substr(p+1);
                    Manage();
                }
            }else{
                p = Buf.find('\n');
                if(p == -1)Buf.clear();
                else{
                    Buf = Buf.substr(p);
                    Manage();
                }
            }
        }
    }
};

