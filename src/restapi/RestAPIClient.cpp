#include <restapi/RestAPIClient.h>
#include <common/Defs.h>

#include <glog/logging.h>

#include <mutex>

using namespace snap::restapi;


namespace
{
    size_t dataCallback(void *content, size_t size, size_t nmemb, std::string *buffer)
    {
        buffer->append((char *)content, size * nmemb);
        return size * nmemb;
    }

    void initCurlGlobal()
    {
        ::curl_global_init(CURL_GLOBAL_DEFAULT);
    }

    const std::string methodStr[] = {"GET", "POST", "PUT", "DELETE"};
}

RestAPIClient::RestAPIClient()
{
    init();
}

void RestAPIClient::init()
{
    static std::once_flag onceFlag;
    std::call_once(onceFlag, initCurlGlobal);
    m_curl = ::curl_easy_init();
}

std::string RestAPIClient::getResponse(Request &request)
{
    std::string response;

    if (UNLIKELY(m_curl == nullptr))
    {
        return response;
    }

    if (request.getMethod() == Method::GET)
    {
        auto uri = request.getResource() + "?" + request.getData();
        curl_easy_setopt(m_curl, CURLOPT_URL, uri.c_str());
    }
    else
    {
        curl_easy_setopt(m_curl, CURLOPT_URL, request.getResource().c_str());
        if (request.getMethod() == Method::PUT || request.getMethod() == Method::DELETE)
        {
            curl_easy_setopt(m_curl, CURLOPT_CUSTOMREQUEST, methodStr[static_cast<int>(request.getMethod())].c_str());
        }

        curl_easy_setopt(m_curl, CURLOPT_POSTFIELDS, request.getData().c_str());
    }

    curl_easy_setopt(m_curl, CURLOPT_WRITEFUNCTION, dataCallback);
    curl_easy_setopt(m_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(m_curl, CURLOPT_SSL_VERIFYPEER, false);
    curl_easy_setopt(m_curl, CURLOPT_ENCODING, "gzip");

    if (request.getExtraHttpHeader().empty() == false)
    {

        struct curl_slist *chunk = nullptr;
        for (auto& extraHttpHeader: request.getExtraHttpHeader())
        {
            chunk = curl_slist_append(chunk, extraHttpHeader.c_str());
        }

        curl_easy_setopt(m_curl, CURLOPT_HTTPHEADER, chunk);
    }

    

    auto res = ::curl_easy_perform(m_curl);

    if (res != CURLE_OK)
    {
       LOG(ERROR) << "ResetAPI failed : " <<  curl_easy_strerror(res);
    }

    return response;
}

RestAPIClient::~RestAPIClient()
{
    ::curl_easy_cleanup(m_curl);
}
