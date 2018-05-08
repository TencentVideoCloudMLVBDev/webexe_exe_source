#pragma once

#include <string>

class Application
{
protected:
    Application();
    Application(const Application&);
    Application& operator =(const Application&);
public:
    virtual ~Application();
    static Application& instance();

    int run(int &argc, char **argv);
    void quit(int retcode = 0);

    bool openAndWait(const std::string& jsonUTF8); // 伪协议启动程序，协议格式TXCloudRoom://liteav/params?json={}
private:
    void showNormalLiveByCommandLine(int &argc, char **argv);

    bool regProtol();   // 注册表注册伪协议
};
