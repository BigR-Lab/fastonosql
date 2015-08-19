#pragma once

#include "common/value.h"
#include "common/convert2string.h"

namespace fastonosql
{
    class IFastoObjectObserver;
    class FastoObject
            : public common::intrusive_ptr_base<FastoObject>
    {
	public:
        typedef std::vector<FastoObject*> child_container_type;

        FastoObject(FastoObject* parent, common::Value* val, const std::string& delemitr = std::string());
        virtual ~FastoObject();

        common::Value::Type type() const;
        virtual std::string toString() const;

        static FastoObject* createRoot(const std::string& text, IFastoObjectObserver* observer = NULL);

        child_container_type childrens() const;
        void addChildren(FastoObject* child);
        FastoObject* parent() const;
        void clear();
        std::string delemitr() const;

        common::Value* value() const;
        void setValue(common::Value* val);

    protected:
        IFastoObjectObserver* observer_;
        common::scoped_ptr<common::Value> value_;

    private:
        DISALLOW_COPY_AND_ASSIGN(FastoObject);

        FastoObject* const parent_;
		child_container_type childrens_;
        const std::string delemitr_;
    };

    class FastoObjectCommand
            : public FastoObject
    {
    public:
        virtual ~FastoObjectCommand();
        common::CommandValue* cmd() const;
        virtual std::string toString() const;

        virtual std::string inputCmd() const = 0;
        virtual std::string inputArgs() const = 0;

        virtual bool isReadOnly() const = 0;

        std::string inputCommand() const;
        common::Value::CommandLoggingType commandLoggingType() const;

    protected:
        FastoObjectCommand(FastoObject* parent, common::CommandValue* cmd, const std::string &delemitr);
    };

    std::string stableCommand(const char* command);
    std::pair<std::string, std::string> getKeyValueFromLine(const std::string& input);
    std::string getOppositeCommand(const std::string& command, const std::vector<std::pair<std::string, std::string > >& srcOppositeCommands);

    class FastoObjectArray
            : public FastoObject
    {
    public:
        FastoObjectArray(FastoObject* parent, common::ArrayValue* ar, const std::string& delemitr);

        // Appends a Value to the end of the list.
        void append(common::Value* in_value);
        virtual std::string toString() const;

        common::ArrayValue* array() const;
    };

    class FastoObjectSet
            : public FastoObject
    {
    public:
        FastoObjectSet(FastoObject* parent, common::SetValue* ar, const std::string& delemitr);

        // Insert a Value to the set.
        void insert(common::Value* in_value);
        virtual std::string toString() const;

        common::SetValue* set() const;
    };

    class FastoObjectZSet
            : public FastoObject
    {
    public:
        FastoObjectZSet(FastoObject* parent, common::ZSetValue *ar, const std::string& delemitr);

        // Insert a Value to the map.
        void insert(common::Value* key, common::Value* value);
        virtual std::string toString() const;

        common::ZSetValue* zset() const;
    };

    class FastoObjectHash
            : public FastoObject
    {
    public:
        FastoObjectHash(FastoObject* parent, common::HashValue *ar, const std::string& delemitr);

        // Insert a Value to the map.
        void insert(common::Value* key, common::Value* value);
        virtual std::string toString() const;

        common::HashValue* hash() const;
    };

    class IFastoObjectObserver
    {
    public:
        virtual void addedChildren(FastoObject* child) = 0;
        virtual void updated(FastoObject* item, common::Value* val) = 0;
    };

    typedef common::intrusive_ptr<FastoObject> FastoObjectIPtr;
    typedef common::intrusive_ptr<FastoObjectCommand> FastoObjectCommandIPtr;

    /*static FastoObjectIPtr createFastoObjectByType(common::Value::Type type)
    {
        if(type == common::Value::TYPE_ARRAY){
            return new FastoObjectArray(NULL, common::Value::createArrayValue(), std::string());
        }
        else if(type == common::Value::TYPE_HASH){
            return new FastoObjectHash(NULL, common::Value::createHashValue(), std::string());
        }
        else if(type == common::Value::TYPE_SET){
            return new FastoObjectSet(NULL, common::Value::createSetValue(), std::string());
        }
        else if(type == common::Value::TYPE_ZSET){
            return new FastoObjectZSet(NULL, common::Value::createZSetValue(), std::string());
        }
        else if(type == common::Value::TYPE_COMMAND){
            return new FastoObjectCommand(NULL, common::Value::createCommand(), std::string());
        }
        else if(type == common::Value::TYPE_STRING){
            return new FastoObject(NULL, common::Value::createStringValue(), std::string());
        }
    }*/
}

namespace common
{
    std::string convertToString(fastonosql::FastoObject* obj);
}
