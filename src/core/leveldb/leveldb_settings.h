#pragma once

#include "core/connection_settings.h"

#include "core/leveldb/leveldb_config.h"

namespace fastonosql
{
    class LeveldbConnectionSettings
            : public IConnectionSettingsBase
    {
    public:
        explicit LeveldbConnectionSettings(const std::string& connectionName);

        virtual std::string commandLine() const;
        virtual void setCommandLine(const std::string& line);

        virtual void setHost(const common::net::hostAndPort& host);
        virtual common::net::hostAndPort host() const;

        leveldbConfig info() const;
        void setInfo(const leveldbConfig &info);

        virtual std::string fullAddress() const;

        virtual IConnectionSettings* clone() const;

    private:
        virtual std::string toCommandLine() const;
        virtual void initFromCommandLine(const std::string& val);
        leveldbConfig info_;
    };
}
