#pragma once

#include "core/types.h"

#define SSDB_COMMON_LABEL "# Common"

#define SSDB_VERSION_LABEL "version"
#define SSDB_LINKS_LABEL "links"
#define SSDB_TOTAL_CALLS_LABEL "total_calls"
#define SSDB_DBSIZE_LABEL "dbsize"
#define SSDB_BINLOGS_LABEL "binlogs"

namespace fastonosql
{
    extern const std::vector<std::string> LeveldbHeaders;
    extern const std::vector<std::vector<Field> > LeveldbFields;

    class LeveldbServerInfo
            : public ServerInfo
    {
    public:
        struct Common
                : FieldByIndex
        {
            Common();
            explicit Common(const std::string& common_text);
            common::Value* valueByIndex(unsigned char index) const;

            std::string version_;
            uint32_t links_;
            uint32_t total_calls_;
            uint32_t dbsize_;
            std::string binlogs_;
        } common_;

        LeveldbServerInfo();
        LeveldbServerInfo(const Common& common);
        virtual common::Value* valueByIndexes(unsigned char property, unsigned char field) const;
        virtual std::string toString() const;
    };

    std::ostream& operator << (std::ostream& out, const LeveldbServerInfo& value);

    LeveldbServerInfo* makeLeveldbServerInfo(const std::string &content);
    LeveldbServerInfo* makeLeveldbServerInfo(FastoObject *root);

    class LeveldbDataBaseInfo
            : public DataBaseInfo
    {
    public:
        LeveldbDataBaseInfo(const std::string& name, size_t size, bool isDefault);
        virtual DataBaseInfo* clone() const;
    };

    template<>
    struct DBTraits<LEVELDB>
    {
        static const std::vector<common::Value::Type> supportedTypes;
    };
}
