#pragma once
#include <string>
#include <vector>
#include <ctime>

struct sockaddr;


namespace
{
    constexpr size_t getDelimiterSize(char c)
    {
        return sizeof(c);
    }

    inline size_t getDelimiterSize(const std::string& str)
    {
        return str.length();
    }

    template <typename T>
    inline T subString(const std::string& str, size_t start, size_t end)
    {
        return str.substr(start, end - start);
    }

    template <>
    inline char subString(const std::string& str, size_t start, size_t end)
    {
        return str.substr(start, end - start)[0];
    }

    template <>
    inline int subString(const std::string& str, size_t start, size_t end)
    {
        return std::atoi(str.substr(start, end - start).c_str());
    }
}

struct iovec;


namespace lu::utils
{

    class Utils
    {
    public:
        template <typename T, typename DelimeterType>
        static std::vector<T> splitString(const std::string& str, const DelimeterType& delimeter)
        {
            std::vector<T> retVal;

            if (str.empty())
            {
                return retVal;
            }

            size_t textFileDelimeterLen = getDelimiterSize(delimeter);

            size_t startPos = 0;
            size_t endPos = str.find(delimeter);

            while (endPos != std::string::npos)
            {
                retVal.push_back(subString<T>(str, startPos, endPos));
                startPos = endPos + textFileDelimeterLen;
                endPos = str.find(delimeter, startPos);
            }

            retVal.push_back(subString<T>(str, startPos, std::string::npos));
            return retVal;
        }

        static std::string getString(double value);

        static std::time_t getUTCDateTime(const std::string& dateTimeStr, const std::string& format="%Y%m%d%H%M%S");
        static std::time_t getDateTime(const std::string& dateTimeStr, const std::string& format="%Y%m%d%H%M%S");

        //TODO Change to C++23 or use chron 
        //static std::string getDateTimeStr(std::time_t time, const std::string& fromat="%Y%m%d%H%M%S");
        static bool readDataSocket(int socketId, uint8_t *buf, size_t size, ssize_t &readCount);
        static bool readDataFile(int socketId, uint8_t *buf, size_t size, ssize_t &readCount);

        static bool readDataSocket(int socketId,struct iovec* dataBufferVec, int numOfVBuffers, ssize_t &readCount);
        static std::string &toLower(std::string &str);
        static uint32_t generateRandomUint32();
    };
}
