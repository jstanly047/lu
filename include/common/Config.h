#pragma once
#include <string>
#include <set>
#include <map>
#include <variant>
#include <string_view>
#include <ctime>
#include <filesystem>
#include <glog/logging.h>

namespace lu::common
{
    class Config
    {
    public:
        using Value=std::variant<bool, char, int, unsigned int, long long, unsigned long long, float, double, std::string, std::time_t>;
        enum Type:int
        {
            Boolean = 1,
            Char = 2,
            Int = 3,
            UnsignedInt = 4,
            LongLong = 5,
            UnsignedLongLong = 6,
            Float = 7,
            Double = 8,
            String = 9,
            EpochTime = 10,
        };

        Config(const std::string_view& name, Value value) :
            m_name(name),
            m_value(value)
        {
        }

        const std::string& getName() const { return m_name; }
        template <typename T>
        const T& getValue() const { return std::get<T>(m_value); }

    private:
        std::string m_name;
        Value m_value;
    };

    class Configurations
    {
        struct ConfigComparator
        {
            bool operator()(const Config &lhs, const Config &rhs) const
            {
                return lhs.getName() < rhs.getName();
            }
        };

        using ConfigSet = std::set<Config, ConfigComparator> ;

    public:
        //Configurations(const Configurations&) = delete;
        //Configurations operator=(const Configurations&) = delete;
        //Configurations(Configurations&&) = delete;
        //Configurations& operator=(Configurations&&) = delete;

        static Configurations getFromFile(const std::filesystem::path& root, const std::string& fileName);
        static bool loadFromFile(const std::string& fileName);
        template <typename T>
        static const T& getValue(const std::string& group, const std::string& configName)
        {
            return getConfigurations().get<T>(group, configName);
        }

        template <typename T>
        const T& get(const std::string& group, const std::string& configName) const;

    private:
        Configurations(){}
        static Configurations& getConfigurations();
        bool loadFile(const std::string& fileName);

        std::set<Config, ConfigComparator> m_configs;
        std::map<std::string, ConfigSet> m_configsByGroup;
    };
}