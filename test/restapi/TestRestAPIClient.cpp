#include <restapi/RestAPIClient.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace snap::restapi;

class TestRestAPIClient : public ::testing::Test
{
};

TEST_F(TestRestAPIClient, GET)
{
    RestAPIClient restAPIClient;
    Request request(Method::GET, "https://fapi.binance.com/fapi/v1/premiumIndex");
    request.append("symbol", "ETHUSDT");
    auto response = restAPIClient.getResponse(request);
    std::cout << response << std::endl;
    ASSERT_FALSE(response.empty());
}