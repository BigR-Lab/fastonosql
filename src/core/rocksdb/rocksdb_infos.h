#pragma once

#include "core/types.h"

#define ROCKSDB_STATS_LABEL "# Stats"

#define ROCKSDB_CAMPACTIONS_LEVEL_LABEL "compactions_level"
#define ROCKSDB_FILE_SIZE_MB_LABEL "file_size_mb"
#define ROCKSDB_TIME_SEC_LABEL "time_sec"
#define ROCKSDB_READ_MB_LABEL "read_mb"
#define ROCKSDB_WRITE_MB_LABEL "write_mb"

namespace fastonosql
{
    extern const std::vector<std::string> rocksdbHeaders;
    extern const std::vector<std::vector<Field> > rocksdbFields;

    class RocksdbServerInfo
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

        RocksdbServerInfo();
        RocksdbServerInfo(const Stats& stats);
        virtual common::Value* valueByIndexes(unsigned char property, unsigned char field) const;
        virtual std::string toString() const;
        virtual uint32_t version() const;
    };

    std::ostream& operator << (std::ostream& out, const RocksdbServerInfo& value);

    RocksdbServerInfo* makeRocksdbServerInfo(const std::string &content);
    RocksdbServerInfo* makeRocksdbServerInfo(FastoObject *root);

    class RocksdbDataBaseInfo
            : public DataBaseInfo
    {
    public:
        RocksdbDataBaseInfo(const std::string& name, size_t size, bool isDefault);
        virtual DataBaseInfo* clone() const;
    };

    template<>
    struct DBTraits<ROCKSDB>
    {
        static const std::vector<common::Value::Type> supportedTypes;
    };

    class RocksdbCommand
            : public FastoObjectCommand
    {
    public:
        RocksdbCommand(FastoObject* parent, common::CommandValue* cmd, const std::string &delemitr);
        virtual bool isReadOnly() const;
    };
}
