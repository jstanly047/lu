#pragma once

#include <string>


namespace lu::socket{

    template<typename DataHandler, typename ServerCallback>
    class Server
    {
    public:
        Server(const Server& ) = delete;
        Server& operator=(const Server& ) = delete;
        Server(ServerCallback& serverCallback) : m_serverCallback(serverCallback) {}
        bool init(int timerInMSec = -1);
        bool start(const std::string& service, int maxEvents = 10);

    private:
        int m_epollFD{};
        int m_timerInMSec = -1;
        bool m_serverStarted = false;
        ServerCallback& m_serverCallback;
    };
}