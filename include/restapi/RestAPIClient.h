#pragma once
#include <restapi/Request.h>
#include <string>
#include <vector>
#include <curl/curl.h>


namespace snap::restapi
{
    class RestAPIClient
    {
    public:
        RestAPIClient();
        std::string getResponse(Request& );
        ~RestAPIClient();

    private:
        void init();

        CURL *m_curl{};
    };
}