#include "core/unqlite/unqlite_infos.h"

#include <ostream>
#include <sstream>

namespace
{
    using namespace fastonosql;

    const std::vector<Field> unqliteCommonFields =
    {
        Field(UNQLITE_CAMPACTIONS_LEVEL_LABEL, common::Value::TYPE_UINTEGER),
        Field(UNQLITE_FILE_SIZE_MB_LABEL, common::Value::TYPE_UINTEGER),
        Field(UNQLITE_TIME_SEC_LABEL, common::Value::TYPE_UINTEGER),
        Field(UNQLITE_READ_MB_LABEL, common::Value::TYPE_UINTEGER),
        Field(UNQLITE_WRITE_MB_LABEL, common::Value::TYPE_UINTEGER)
    };
}

namespace fastonosql
{   
    const std::vector<common::Value::Type> DBTraits<UNQLITE>::supportedTypes = {
                                            common::Value::TYPE_BOOLEAN,
                                            common::Value::TYPE_INTEGER,
                                            common::Value::TYPE_UINTEGER,
                                            common::Value::TYPE_DOUBLE,
                                            common::Value::TYPE_STRING,
                                            common::Value::TYPE_ARRAY
                                           };

    const std::vector<std::string> unqliteHeaders =
    {
        UNQLITE_STATS_LABEL
    };

    const std::vector<std::vector<Field> > unqliteFields =
    {
        unqliteCommonFields
    };

    UnqliteServerInfo::Stats::Stats()
        : compactions_level_(0), file_size_mb_(0), time_sec_(0), read_mb_(0), write_mb_(0)
    {

    }

    UnqliteServerInfo::Stats::Stats(const std::string& common_text)
    {
        const std::string &src = common_text;
        size_t pos = 0;
        size_t start = 0;

        while((pos = src.find(("\r\n"), start)) != std::string::npos){
            std::string line = src.substr(start, pos-start);
            size_t delem = line.find_first_of(':');
            std::string field = line.substr(0, delem);
            std::string value = line.substr(delem + 1);
            if(field == UNQLITE_CAMPACTIONS_LEVEL_LABEL){
                compactions_level_ = common::convertFromString<uint32_t>(value);
            }
            else if(field == UNQLITE_FILE_SIZE_MB_LABEL){
                file_size_mb_ = common::convertFromString<uint32_t>(value);
            }
            else if(field == UNQLITE_TIME_SEC_LABEL){
                time_sec_ = common::convertFromString<uint32_t>(value);
            }
            else if(field == UNQLITE_READ_MB_LABEL){
                read_mb_ = common::convertFromString<uint32_t>(value);
            }
            else if(field == UNQLITE_WRITE_MB_LABEL){
                write_mb_ = common::convertFromString<uint32_t>(value);
            }
            start = pos + 2;
        }
    }

    common::Value* UnqliteServerInfo::Stats::valueByIndex(unsigned char index) const
    {
        switch (index) {
        case 0:
            return new common::FundamentalValue(compactions_level_);
        case 1:
            return new common::FundamentalValue(file_size_mb_);
        case 2:
            return new common::FundamentalValue(time_sec_);
        case 3:
            return new common::FundamentalValue(read_mb_);
        case 4:
            return new common::FundamentalValue(write_mb_);
        default:
            NOTREACHED();
            break;
        }
        return NULL;
    }

    UnqliteServerInfo::UnqliteServerInfo()
        : ServerInfo(ROCKSDB)
    {

    }

    UnqliteServerInfo::UnqliteServerInfo(const Stats &stats)
        : ServerInfo(ROCKSDB), stats_(stats)
    {

    }

    common::Value* UnqliteServerInfo::valueByIndexes(unsigned char property, unsigned char field) const
    {
        switch (property) {
        case 0:
            return stats_.valueByIndex(field);
        default:
            NOTREACHED();
            break;
        }
        return NULL;
    }

    std::ostream& operator<<(std::ostream& out, const UnqliteServerInfo::Stats& value)
    {
        return out << UNQLITE_CAMPACTIONS_LEVEL_LABEL":" << value.compactions_level_ << ("\r\n")
                    << UNQLITE_FILE_SIZE_MB_LABEL":" << value.file_size_mb_ << ("\r\n")
                    << UNQLITE_TIME_SEC_LABEL":" << value.time_sec_ << ("\r\n")
                    << UNQLITE_READ_MB_LABEL":" << value.read_mb_ << ("\r\n")
                    << UNQLITE_WRITE_MB_LABEL":" << value.write_mb_ << ("\r\n");
    }

    std::ostream& operator<<(std::ostream& out, const UnqliteServerInfo& value)
    {
        return out << value.toString();
    }

    UnqliteServerInfo* makeUnqliteServerInfo(const std::string &content)
    {
        if(content.empty()){
            return NULL;
        }

        UnqliteServerInfo* result = new UnqliteServerInfo;

        std::string word;
        DCHECK(unqliteHeaders.size() == 1);
        for(int i = 0; i < content.size(); ++i)
        {
            word += content[i];
            if(word == unqliteHeaders[0]){
                std::string part = content.substr(i + 1);
                result->stats_ = UnqliteServerInfo::Stats(part);
                break;
            }
        }

        return result;
    }


    std::string UnqliteServerInfo::toString() const
    {
        std::stringstream str;
        str << UNQLITE_STATS_LABEL"\r\n" << stats_;
        return str.str();
    }

    uint32_t UnqliteServerInfo::version() const
    {
        return 0;
    }

    UnqliteServerInfo* makeUnqliteServerInfo(FastoObject* root)
    {
        const std::string content = common::convertToString(root);
        return makeUnqliteServerInfo(content);
    }

    UnqliteDataBaseInfo::UnqliteDataBaseInfo(const std::string& name, size_t size, bool isDefault)
        : DataBaseInfo(name, size, isDefault, UNQLITE)
    {

    }

    DataBaseInfo* UnqliteDataBaseInfo::clone() const
    {
        return new UnqliteDataBaseInfo(*this);
    }

    UnqliteCommand::UnqliteCommand(FastoObject* parent, common::CommandValue* cmd, const std::string &delemitr)
        : FastoObjectCommand(parent, cmd, delemitr)
    {

    }

    bool UnqliteCommand::isReadOnly() const
    {
        std::string key = inputCmd();
        if(key.empty()){
            return true;
        }

        std::transform(key.begin(), key.end(), key.begin(), ::tolower);
        return key != "get";
    }
}
