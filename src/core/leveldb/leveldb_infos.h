#pragma once

#include "core/types.h"

#define LEVELDB_STATS_LABEL "# Stats"

#define LEVELDB_CAMPACTIONS_LEVEL_LABEL "compactions_level"
#define LEVELDB_FILE_SIZE_MB_LABEL "file_size_mb"
#define LEVELDB_TIME_SEC_LABEL "time_sec"
#define LEVELDB_READ_MB_LABEL "read_mb"
#define LEVELDB_WRITE_MB_LABEL "write_mb"

namespace fastonosql
{
    extern const std::vector<std::string> LeveldbHeaders;
    extern const std::vector<std::vector<Field> > LeveldbFields;

    class LeveldbServerInfo
            : public ServerInfo
    {
    public:
        //Compactions\nLevel  Files Size(MB) Time(sec) Read(MB) Write(MB)\n
        struct Stats
                : FieldByIndex
        {
            Stats();
            explicit Stats(const std::string& common_text);
            common::Value* valueByIndex(unsigned char index) const;

            uint32_t compactions_level_;
            uint32_t file_size_mb_;
            uint32_t time_sec_;
            uint32_t read_mb_;
            uint32_t write_mb_;
        } stats_;

        LeveldbServerInfo();
        LeveldbServerInfo(const Stats& stats);
        virtual common::Value* valueByIndexes(unsigned char property, unsigned char field) const;
        virtual std::string toString() const;
        virtual uint32_t version() const;
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
