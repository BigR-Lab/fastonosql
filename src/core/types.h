#pragma once

#include "global/global.h"

#include "core/connection_types.h"

#include "common/net/net.h"

#define UNDEFINED_SINCE 0x00000000U
#define UNDEFINED_SINCE_STR "Undefined"
#define UNDEFINED_EXAMPLE_STR "Unspecified"
#define UNDEFINED_STR_IN_PROGRESS "Undefined in progress"
#define INFINITE_COMMAND_ARGS UINT8_MAX

namespace fastonosql
{
    struct CommandInfo
    {
        CommandInfo(const std::string& name, const std::string& params,
                    const std::string& summary, const uint32_t since,
                    const std::string& example, uint8_t required_arguments_count, uint8_t optional_arguments_count);

        uint16_t maxArgumentsCount() const;
        uint8_t minArgumentsCount() const;

        const std::string name_;
        const std::string params_;
        const std::string summary_;
        const uint32_t since_;
        const std::string example_;

        const uint8_t required_arguments_count_;
        const uint8_t optional_arguments_count_;
    };

    std::string convertVersionNumberToReadableString(uint32_t version);

    struct NKey
    {
        explicit NKey(const std::string& key, int32_t ttl_sec = -1);

        std::string key_;
        int32_t ttl_sec_;
    };

    typedef common::ValueSPtr NValue;

    class NDbValue
    {
    public:
        NDbValue(const NKey& key, NValue value);

        NKey key() const;
        NValue value() const;
        common::Value::Type type() const;

        void setTTL(int32_t ttl);
        void setValue(NValue value);

        std::string keyString() const;

    private:
        NKey key_;
        NValue value_;
    };

    class ServerDiscoveryInfo
    {
    public:
        virtual ~ServerDiscoveryInfo();

        connectionTypes connectionType() const;
        serverTypes type() const;
        bool self() const;

        std::string name() const;
        void setName(const std::string& name);

        common::net::hostAndPort host() const;
        void setHost(const common::net::hostAndPort& host);

    protected:
        DISALLOW_COPY_AND_ASSIGN(ServerDiscoveryInfo);

        ServerDiscoveryInfo(connectionTypes ctype, serverTypes type, bool self);
        common::net::hostAndPort host_;
        std::string name_;

    private:
        const bool self_;
        const serverTypes type_;
        const connectionTypes ctype_;
    };

    typedef common::shared_ptr<ServerDiscoveryInfo> ServerDiscoveryInfoSPtr;

    class ServerInfo
    {
    public:
        explicit ServerInfo(connectionTypes type);
        virtual ~ServerInfo();

        connectionTypes type() const;
        virtual std::string toString() const = 0;
        virtual uint32_t version() const = 0;
        virtual common::Value* valueByIndexes(unsigned char property, unsigned char field) const = 0;        

    protected:
        DISALLOW_COPY_AND_ASSIGN(ServerInfo);

    private:
        const connectionTypes type_;
    };

    struct FieldByIndex
    {
        virtual common::Value* valueByIndex(unsigned char index) const = 0;
    };

    struct Field
    {
        Field(const std::string& name, common::Value::Type type);

        bool isIntegral() const;
        std::string name_;
        common::Value::Type type_;
    };

    template<connectionTypes ct>
    struct DBTraits
    {
        static std::vector<common::Value::Type> supportedTypes();
        static std::vector<std::string> infoHeaders();
        static std::vector< std::vector<Field> > infoFields();
    };

    std::vector<common::Value::Type> supportedTypesFromType(connectionTypes type);
    std::vector<std::string> infoHeadersFromType(connectionTypes type);
    std::vector< std::vector<Field> > infoFieldsFromType(connectionTypes type);

    typedef common::shared_ptr<ServerInfo> ServerInfoSPtr;

    struct ServerInfoSnapShoot
    {
        ServerInfoSnapShoot();
        ServerInfoSnapShoot(common::time64_t msec, ServerInfoSPtr info);
        bool isValid() const;

        common::time64_t msec_;
        ServerInfoSPtr info_;
    };

    typedef std::pair<std::string, std::string> PropertyType;

    struct ServerPropertyInfo
    {
        ServerPropertyInfo();
        std::vector<PropertyType> propertyes_;
    };

    ServerPropertyInfo makeServerProperty(FastoObjectArray* array);

    class DataBaseInfo
    {
    public:
        typedef std::vector<NDbValue> keys_cont_type;
        connectionTypes type() const;
        std::string name() const;
        void setSize(size_t sz);
        size_t size() const;

        bool isDefault() const;
        void setIsDefault(bool isDef);

        virtual DataBaseInfo* clone() const = 0;
        virtual ~DataBaseInfo();

        keys_cont_type keys() const;
        void setKeys(const keys_cont_type& keys);

    protected:
        DataBaseInfo(const std::string& name, size_t size, bool isDefault, connectionTypes type);

    private:
        std::string name_;
        size_t size_;
        bool isDefault_;
        keys_cont_type keys_;

        const connectionTypes type_;
    };

    inline bool operator == (const DataBaseInfo& lhs, const DataBaseInfo& rhs)
    {
        return lhs.name() == rhs.name() && lhs.size() == rhs.size() && lhs.isDefault() == rhs.isDefault() && lhs.type() == rhs.type();
    }

    class CommandKey
    {
    public:
        enum cmdtype
        {
            C_NONE,
            C_DELETE,
            C_LOAD,
            C_CREATE,
            C_CHANGE_TTL
        };

        cmdtype type() const;
        NDbValue key() const;

        virtual ~CommandKey();

    protected:
        CommandKey(const NDbValue& key, cmdtype type);

        const cmdtype type_;
        const NDbValue key_;
    };

    class CommandDeleteKey
            : public CommandKey
    {
    public:
        explicit CommandDeleteKey(const NDbValue& key);
    };

    class CommandLoadKey
            : public CommandKey
    {
    public:
        explicit CommandLoadKey(const NDbValue& key);
    };

    class CommandCreateKey
            : public CommandKey
    {
    public:
        explicit CommandCreateKey(const NDbValue& dbv);
        NValue value() const;
    };

    class CommandChangeTTL
            : public CommandKey
    {
    public:
        CommandChangeTTL(const NDbValue& dbv, int32_t newTTL);
        int32_t newTTL() const;
        NDbValue newKey() const;

    private:
        int32_t new_ttl_;
    };

    typedef common::shared_ptr<CommandKey> CommandKeySPtr;

    template<typename Command>
    FastoObjectCommand* createCommand(FastoObject* parent, const std::string& input, common::Value::CommandLoggingType ct)
    {
        if(input.empty()){
            return NULL;
        }

        DCHECK(parent);
        if(!parent){
            return NULL;
        }

        common::CommandValue* cmd = common::Value::createCommand(input, ct);
        FastoObjectCommand* fs = new Command(parent, cmd, parent->delemitr());
        parent->addChildren(fs);
        return fs;
    }

    template<typename Command>
    FastoObjectCommand* createCommand(FastoObjectIPtr parent, const std::string& input, common::Value::CommandLoggingType ct)
    {
        return createCommand<Command>(parent.get(), input, ct);
    }
}
