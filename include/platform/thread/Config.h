#pragma once

#include <string>

namespace lu::platform::thread
{
    struct ServerConfig
    {
        bool CREATE_NEW_THREAD = false;
        //bool ACT_AS_CLIENT_HANDLER = false;
        unsigned int NUMBER_OF_CLIENT_HANDLE_THREADS = 2u;
        unsigned int NUMBER_OF_CONNECTION_IN_WAITING_QUEUE = 50u;
        unsigned int NUMBER_OF_EVENTS_PER_HANDLE = 10u;
        unsigned int CLIENT_HANDLER_NUMBER_OF_EVENTS_PER_HANDLE = NUMBER_OF_EVENTS_PER_HANDLE;
        unsigned int TIMER_IN_MSEC = 1000u;
        std::string TIMER_NAME = "SER";
        bool REPEAT_TIMER = true;
        bool IPV6 = false;
    };

    struct EventThreadConfig
    {
        EventThreadConfig(const ServerConfig& serverConfig):
            NUMBER_OF_EVENTS_PER_HANDLE(serverConfig.CLIENT_HANDLER_NUMBER_OF_EVENTS_PER_HANDLE),
            TIMER_IN_MSEC(serverConfig.TIMER_IN_MSEC),
            TIMER_NAME(serverConfig.TIMER_NAME),
            REPEAT_TIMER(serverConfig.REPEAT_TIMER)
        {

        }

        EventThreadConfig(){}

        int NUMBER_OF_EVENTS_PER_HANDLE = 10;
        unsigned int TIMER_IN_MSEC = 1000u;
        std::string TIMER_NAME = "SCH";
        bool REPEAT_TIMER = true;
    };
}