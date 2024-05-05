#include <restapi/RestAPIClient.h>

#include <openssl/hmac.h>
#include <openssl/sha.h>


#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace snap::restapi;

namespace
{
    /*std::string b2Hex(char *byte_arr, int n)
    {
        const static std::string HexCodes = "0123456789abcdef";
        std::string HexString;
        for (int i = 0; i < n; ++i)
        {
            unsigned char BinValue = byte_arr[i];
            HexString += HexCodes[(BinValue >> 4) & 0x0F];
            HexString += HexCodes[BinValue & 0x0F];
        }

        return HexString;
    }

    std::string hmacSHA256(const std::string& key, const std::string& data)
    {
        unsigned char *digest;
        digest = ::HMAC(EVP_sha256(), key.data(), static_cast<int>(key.length()), (unsigned char *) data.data(), data.length(), NULL, NULL);
        return b2Hex((char *)digest, 32);
    }*/
}

class TestRestAPIClient : public ::testing::Test
{
};

TEST_F(TestRestAPIClient, GET)
{
    RestAPIClient restAPIClient;
    Request request(Method::GET);
    request.setResource("https://fapi.binance.com/fapi/v1/premiumIndex");
    request.append("symbol", "ETHUSDT");
    auto response = restAPIClient.getResponse(request);
    std::cout << response << std::endl;
    ASSERT_FALSE(response.empty());
}

/*
TEST_F(TestRestAPIClient, getall)
{
    RestAPIClient restAPIClient;
    Request request(Method::GET);
    request.setResource("https://api.binance.com/sapi/v1/capital/config/getall");
     struct timeval time;
    gettimeofday(&time, NULL);
    long long int timeMS = time.tv_sec * 1000 + (time.tv_usec / 1000);
    request.append("timestamp", timeMS);
    request.append("signature",hmacSHA256("SE6lx68m0zuXmkSYNC1qdPErCNQesvNKEnnvyYbDX8jmurJAx6p8eePPgiTqutzK", request.getData()));
    request.addHttpHeader("X-MBX-APIKEY", "bWN9s540oSHkl1bMFABec3nGQkprWdHN1samG5rTaWgUsrSXEyOlyIFMWpeEC9vc");
    auto response = restAPIClient.getResponse(request);
    std::cout << response << std::endl;
    ASSERT_FALSE(response.empty());
}*/