#include <common/Config.h>
#include <utils/DelimiterTextParser.h>
#include <fstream>
#include <glog/logging.h>

using namespace lu::common;

namespace
{
    const char DELIMITER = ',';
}

Configurations &Configurations::getConfigurations()
{
    static Configurations config;
    return config;
}

bool Configurations::loadFromFile(const std::string &fileName)
{
    return getConfigurations().loadFile(fileName);
}

Configurations Configurations::getFromFile(const std::filesystem::path& root, const std::string& fileName)
{
    auto absPath = root / fileName;
    Configurations configurations;
    configurations.loadFile(absPath);
    return configurations;
}

bool Configurations::loadFile(const std::string &fileName)
{
    std::ifstream inputFileRead(fileName, std::ios_base::in);

    if (!inputFileRead.is_open())
    {
        LOG(ERROR) << "Can not open configuration files [" << fileName << "]";
        return false;
    }

    std::string line;
    lu::utils::DelimiterTextParser delimiterTextParser(line, DELIMITER, 0);
    for(;std::getline(inputFileRead, line);)
    {
        delimiterTextParser.nextLine(line);
        auto groupName = delimiterTextParser.next();
        auto configName = delimiterTextParser.next();

        auto itr = m_configsByGroup.emplace(groupName, ConfigSet());

        switch (static_cast<Config::Type>(delimiterTextParser.nextInt()))
        {
        case Config::Boolean:
            itr.first->second.emplace(configName, delimiterTextParser.nextBool());
            break;
        case Config::Char:
            itr.first->second.emplace(configName, delimiterTextParser.nextChar());
            break;
        case Config::Int:
            itr.first->second.emplace(configName, delimiterTextParser.nextInt());
            break;
        case Config::UnsignedInt:
            itr.first->second.emplace(configName, delimiterTextParser.nextUInt());
            break;
        case Config::LongLong:
            itr.first->second.emplace(configName, delimiterTextParser.nextLongLong());
            break;
        case Config::UnsignedLongLong:
            itr.first->second.emplace(configName, delimiterTextParser.nextULongLong());
            break;
        case Config::Float:
            itr.first->second.emplace(configName, delimiterTextParser.nextFloat());
            break;
        case Config::Double:
            itr.first->second.emplace(configName, delimiterTextParser.nextDouble());
            break;
        case Config::String:
            itr.first->second.emplace(configName, std::string(delimiterTextParser.next()));
            break;
        case Config::EpochTime:
            itr.first->second.emplace(configName, delimiterTextParser.nextDateTime());
            break;
        default:
            LOG(ERROR) << "Unknow type for config [" << groupName  << "," << configName << "]";
            break;
        }
    }

    return true;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
template <typename T>
const T &Configurations::get(const std::string &group, const std::string &configName) const
{
    auto itrGrp = m_configsByGroup.find(group);

    if (itrGrp != m_configsByGroup.end())
    {
        Config config(configName, false);
        auto itrConfig = itrGrp->second.find(config);

        if (itrConfig != itrGrp->second.end())
        {
            return itrConfig->getValue<T>();
        }
    }

    LOG(ERROR) << "Can not find config [" << group << "][" << configName << "] in configurations";
    static T configSetting{};
    return configSetting;
}

template const bool& lu::common::Configurations::get<bool>(const std::string &group, const std::string &configName) const;
template const char& lu::common::Configurations::get<char>(const std::string &group, const std::string &configName) const;
template const int& lu::common::Configurations::get<int>(const std::string &group, const std::string &configName) const;
template const unsigned int& lu::common::Configurations::get<unsigned int>(const std::string &group, const std::string &configName) const;
template const long long& lu::common::Configurations::get<long long>(const std::string &group, const std::string &configName) const;
template const unsigned long long& lu::common::Configurations::get<unsigned long long>(const std::string &group, const std::string &configName) const;
template const float& lu::common::Configurations::get<float>(const std::string &group, const std::string &configName) const;
template const double& lu::common::Configurations::get<double>(const std::string &group, const std::string &configName) const;
template const std::string& lu::common::Configurations::get<std::string>(const std::string &group, const std::string &configName) const;
template const std::time_t& lu::common::Configurations::get<std::time_t>(const std::string &group, const std::string &configName) const;

