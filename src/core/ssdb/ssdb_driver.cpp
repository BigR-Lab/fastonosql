#include "core/ssdb/ssdb_driver.h"

#include "common/sprintf.h"
#include "common/utils.h"
#include "fasto/qt/logger.h"

#include "core/command_logger.h"

#include "core/ssdb/ssdb_config.h"
#include "core/ssdb/ssdb_infos.h"

#include <SSDB.h>

#define INFO_REQUEST "INFO"
#define GET_KEYS_PATTERN_1ARGS_I "KEYS a z %d"
#define DELETE_KEY_PATTERN_1ARGS_S "DEL %s"
#define GET_SERVER_TYPE ""

#define GET_KEY_PATTERN_1ARGS_S "GET %s"
#define GET_KEY_LIST_PATTERN_1ARGS_S "LRANGE %s 0 -1"
#define GET_KEY_SET_PATTERN_1ARGS_S "SMEMBERS %s"
#define GET_KEY_ZSET_PATTERN_1ARGS_S "ZRANGE %s 0 -1"
#define GET_KEY_HASH_PATTERN_1ARGS_S "HGET %s"

#define SET_KEY_PATTERN_2ARGS_SS "SET %s %s"
#define SET_KEY_LIST_PATTERN_2ARGS_SS "LPUSH %s %s"
#define SET_KEY_SET_PATTERN_2ARGS_SS "SADD %s %s"
#define SET_KEY_ZSET_PATTERN_2ARGS_SS "ZADD %s %s"
#define SET_KEY_HASH_PATTERN_2ARGS_SS "HMSET %s %s"

namespace fastonosql
{
    namespace
    {
        common::Error createConnection(const ssdbConfig& config, const SSHInfo& sinfo, ssdb::Client** context)
        {
            DCHECK(*context == NULL);
            UNUSED(sinfo);
            ssdb::Client *lcontext = ssdb::Client::connect(config.hostip_, config.hostport_);
            if (!lcontext){
                return common::make_error_value("Fail connect to server!", common::ErrorValue::E_ERROR);
            }

            *context = lcontext;

            return common::Error();
        }

        common::Error createConnection(SsdbConnectionSettings* settings, ssdb::Client** context)
        {
            if(!settings){
                return common::make_error_value("Invalid input argument", common::ErrorValue::E_ERROR);
            }

            ssdbConfig config = settings->info();
            SSHInfo sinfo = settings->sshInfo();
            return createConnection(config, sinfo, context);
        }
    }

    common::Error testConnection(SsdbConnectionSettings* settings)
    {
        ssdb::Client* ssdb = NULL;
        common::Error er = createConnection(settings, &ssdb);
        if(er){
            return er;
        }

        delete ssdb;

        return common::Error();
    }

    struct SsdbDriver::pimpl
    {
        pimpl()
            : ssdb_(NULL)
        {

        }

        bool isConnected() const
        {
            if(!ssdb_){
                return false;
            }

            return true;
        }

        common::Error connect()
        {
            if(isConnected()){
                return common::Error();
            }

            clear();
            init();

            ssdb::Client* context = NULL;
            common::Error er = createConnection(config_, sinfo_, &context);
            if(er){
                return er;
            }

            ssdb_ = context;

            return common::Error();
        }

        common::Error disconnect()
        {
            if(!isConnected()){
                return common::Error();
            }

            clear();
            return common::Error();
        }

        common::Error info(const char* args, SsdbServerInfo::Common& statsout)
        {
            std::vector<std::string> ret;
            ssdb::Status st = ssdb_->info(args ? args : std::string(), &ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "info function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }

            for(int i = 0; i < ret.size(); i += 2){
                if(ret[i] == SSDB_VERSION_LABEL){
                    statsout.version_ = ret[i + 1];
                }
                else if (ret[i] == SSDB_LINKS_LABEL){
                    statsout.links_ = common::convertFromString<uint32_t>(ret[i + 1]);
                }
                else if(ret[i] == SSDB_TOTAL_CALLS_LABEL){
                    statsout.total_calls_ = common::convertFromString<uint32_t>(ret[i + 1]);
                }
                else if(ret[i] == SSDB_DBSIZE_LABEL){
                    statsout.dbsize_ = common::convertFromString<uint32_t>(ret[i + 1]);
                }
                else if(ret[i] == SSDB_BINLOGS_LABEL){
                    statsout.binlogs_ = ret[i + 1];
                }
            }

            return common::Error();
        }

        common::Error dbsize(size_t& size) WARN_UNUSED_RESULT
        {
            int64_t sz = 0;
            ssdb::Status st = ssdb_->dbsize(&sz);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "Couldn't determine DBSIZE error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }

            size = sz;
            return common::Error();
        }

        ~pimpl()
        {
            clear();
        }

        ssdbConfig config_;
        SSHInfo sinfo_;

        common::Error execute_impl(FastoObject* out, int argc, char **argv)
        {
            if(strcasecmp(argv[0], "get") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid get input argument", common::ErrorValue::E_ERROR);
                }

                std::string ret;
                common::Error er = get(argv[1], &ret);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue(ret);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "set") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid set input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = set(argv[1], argv[2]);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "dbsize") == 0){
                if(argc != 1){
                    return common::make_error_value("Invalid dbsize input argument", common::ErrorValue::E_ERROR);
                }

                size_t ret = 0;
                common::Error er = dbsize(ret);
                if(!er){
                    common::FundamentalValue *val = common::Value::createUIntegerValue(ret);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "auth") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid auth input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = auth(argv[1]);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("OK");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "setx") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid setx input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = setx(argv[1], argv[2], atoi(argv[3]));
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "del") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid del input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = del(argv[1]);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("DELETED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "incr") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid incr input argument", common::ErrorValue::E_ERROR);
                }

                int64_t ret = 0;
                common::Error er = incr(argv[1], atoll(argv[2]), &ret);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(ret);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "keys") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid keys input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysout;
                common::Error er = keys(argv[1], argv[2], atoll(argv[3]), &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "scan") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid scan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysout;
                common::Error er = scan(argv[1], argv[2], atoll(argv[3]), &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "rscan") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid rscan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysout;
                common::Error er = rscan(argv[1], argv[2], atoll(argv[3]), &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_get") == 0){
                if(argc < 2){
                    return common::make_error_value("Invalid multi_get input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysget;
                for(int i = 1; i < argc; ++i){
                    keysget.push_back(argv[i]);
                }

                std::vector<std::string> keysout;
                common::Error er = multi_get(keysget, &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_del") == 0){
                if(argc < 2){
                    return common::make_error_value("Invalid multi_del input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysget;
                for(int i = 1; i < argc; ++i){
                    keysget.push_back(argv[i]);
                }

                common::Error er = multi_del(keysget);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("DELETED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_set") == 0){
                if(argc < 2 || argc % 2){
                    return common::make_error_value("Invalid multi_del input argument", common::ErrorValue::E_ERROR);
                }

                std::map<std::string, std::string> keysset;
                for(int i = 1; i < argc; i += 2){
                    keysset[argv[i]] = argv[i + 1];
                }

                common::Error er = multi_set(keysset);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hget") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid hget input argument", common::ErrorValue::E_ERROR);
                }

                std::string ret;
                common::Error er = hget(argv[1], argv[2], &ret);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue(ret);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hset") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid hset input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = hset(argv[1], argv[2], argv[3]);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hdel") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid hset input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = hdel(argv[1], argv[2]);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("DELETED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hincr") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid hincr input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = hincr(argv[1], argv[2], atoll(argv[3]), &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hsize") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid hsize input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = hsize(argv[1], &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hclear") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid hclear input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = hclear(argv[1], &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hkeys") == 0){
                if(argc != 5){
                    return common::make_error_value("Invalid hclear input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysout;
                common::Error er = hkeys(argv[1], argv[2], argv[3], atoll(argv[4]), &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hscan") == 0){
                if(argc != 5){
                    return common::make_error_value("Invalid hscan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysout;
                common::Error er = hscan(argv[1], argv[2], argv[3], atoll(argv[4]), &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "hrscan") == 0){
                if(argc != 5){
                    return common::make_error_value("Invalid hrscan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysout;
                common::Error er = hrscan(argv[1], argv[2], argv[3], atoll(argv[4]), &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_hget") == 0){
                if(argc < 2){
                    return common::make_error_value("Invalid multi_get input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysget;
                for(int i = 2; i < argc; ++i){
                    keysget.push_back(argv[i]);
                }

                std::vector<std::string> keysout;
                common::Error er = multi_hget(argv[1], keysget, &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_hset") == 0){
                if(argc < 2 || (argc % 2 == 0)){
                    return common::make_error_value("Invalid multi_hset input argument", common::ErrorValue::E_ERROR);
                }

                std::map<std::string, std::string> keys;
                for(int i = 2; i < argc; i += 2){
                    keys[argv[i]] = argv[i + 1];
                }

                common::Error er = multi_hset(argv[1], keys);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zget") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid zget input argument", common::ErrorValue::E_ERROR);
                }

                int64_t ret;
                common::Error er = zget(argv[1], argv[2], &ret);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(ret);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zset") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid zset input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = zset(argv[1], argv[2], atoll(argv[3]));
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zdel") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid zdel input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = zdel(argv[1], argv[2]);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("DELETED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zincr") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid zincr input argument", common::ErrorValue::E_ERROR);
                }

                int64_t ret = 0;
                common::Error er = zincr(argv[1], argv[2], atoll(argv[3]), &ret);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(ret);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zsize") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid zsize input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = zsize(argv[1], &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zclear") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid zclear input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = zclear(argv[1], &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zrank") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid zrank input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = zrank(argv[1], argv[2], &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zzrank") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid zzrank input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = zrrank(argv[1], argv[2], &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zrange") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid zrange input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> res;
                common::Error er = zrange(argv[1], atoll(argv[2]), atoll(argv[3]), &res);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < res.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(res[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zrrange") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid zrrange input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> res;
                common::Error er = zrrange(argv[1], atoll(argv[2]), atoll(argv[3]), &res);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < res.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(res[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zkeys") == 0){
                if(argc != 6){
                    return common::make_error_value("Invalid zkeys input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> res;
                int64_t st = atoll(argv[3]);
                int64_t end = atoll(argv[4]);
                common::Error er = zkeys(argv[1], argv[2], &st, &end, atoll(argv[5]), &res);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < res.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(res[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zscan") == 0){
                if(argc != 6){
                    return common::make_error_value("Invalid zscan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> res;
                int64_t st = atoll(argv[3]);
                int64_t end = atoll(argv[4]);
                common::Error er = zscan(argv[1], argv[2], &st, &end, atoll(argv[5]), &res);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < res.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(res[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "zrscan") == 0){
                if(argc != 6){
                    return common::make_error_value("Invalid zrscan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> res;
                int64_t st = atoll(argv[3]);
                int64_t end = atoll(argv[4]);
                common::Error er = zrscan(argv[1], argv[2], &st, &end, atoll(argv[5]), &res);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < res.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(res[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_zget") == 0){
                if(argc < 2){
                    return common::make_error_value("Invalid zrscan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysget;
                for(int i = 2; i < argc; ++i){
                    keysget.push_back(argv[i]);
                }

                std::vector<std::string> res;
                common::Error er = multi_zget(argv[1], keysget, &res);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < res.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(res[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_zset") == 0){
                if(argc < 2 || (argc % 2 == 0)){
                    return common::make_error_value("Invalid zrscan input argument", common::ErrorValue::E_ERROR);
                }

                std::map<std::string, int64_t> keysget;
                for(int i = 2; i < argc; i += 2){
                    keysget[argv[i]] = atoll(argv[i+1]);
                }

                common::Error er = multi_zset(argv[1], keysget);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "multi_zdel") == 0){
                if(argc < 2){
                    return common::make_error_value("Invalid zrscan input argument", common::ErrorValue::E_ERROR);
                }

                std::vector<std::string> keysget;
                for(int i = 2; i < argc; ++i){
                    keysget.push_back(argv[i]);
                }

                common::Error er = multi_zdel(argv[1], keysget);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("DELETED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "info") == 0){
                if(argc > 2){
                    return common::make_error_value("Invalid info input argument", common::ErrorValue::E_ERROR);
                }

                SsdbServerInfo::Common statsout;
                common::Error er = info(argc == 2 ? argv[1] : 0, statsout);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue(SsdbServerInfo(statsout).toString());
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "qpop") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid qpop input argument", common::ErrorValue::E_ERROR);
                }

                std::string ret;
                common::Error er = qpop(argv[1], &ret);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue(ret);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "qpush") == 0){
                if(argc != 3){
                    return common::make_error_value("Invalid qpush input argument", common::ErrorValue::E_ERROR);
                }

                common::Error er = qpush(argv[1], argv[2]);
                if(!er){
                    common::StringValue *val = common::Value::createStringValue("STORED");
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "qslice") == 0){
                if(argc != 4){
                    return common::make_error_value("Invalid qslice input argument", common::ErrorValue::E_ERROR);
                }

                int64_t begin = atoll(argv[2]);
                int64_t end = atoll(argv[3]);

                std::vector<std::string> keysout;
                common::Error er = qslice(argv[1], begin, end, &keysout);
                if(!er){
                    common::ArrayValue* ar = common::Value::createArrayValue();
                    for(int i = 0; i < keysout.size(); ++i){
                        common::StringValue *val = common::Value::createStringValue(keysout[i]);
                        ar->append(val);
                    }
                    FastoObjectArray* child = new FastoObjectArray(out, ar, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else if(strcasecmp(argv[0], "qclear") == 0){
                if(argc != 2){
                    return common::make_error_value("Invalid qclear input argument", common::ErrorValue::E_ERROR);
                }

                int64_t res = 0;
                common::Error er = qclear(argv[1], &res);
                if(!er){
                    common::FundamentalValue *val = common::Value::createIntegerValue(res);
                    FastoObject* child = new FastoObject(out, val, config_.mb_delim_);
                    out->addChildren(child);
                }
                return er;
            }
            else{
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "Not supported command: %s", argv[0]);
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
        }

    private:

        common::Error auth(const std::string& password)
        {
            ssdb::Status st = ssdb_->auth(password);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "password function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error get(const std::string& key, std::string* ret_val)
        {
            ssdb::Status st = ssdb_->get(key, ret_val);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "get function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error set(const std::string& key, const std::string& value)
        {
            ssdb::Status st = ssdb_->set(key, value);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "set function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error setx(const std::string& key, const std::string& value, int ttl)
        {
            ssdb::Status st = ssdb_->setx(key, value, ttl);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "setx function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error del(const std::string& key)
        {
            ssdb::Status st = ssdb_->del(key);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "del function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error incr(const std::string& key, int64_t incrby, int64_t *ret)
        {
            ssdb::Status st = ssdb_->incr(key, incrby, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "Incr function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error keys(const std::string &key_start, const std::string &key_end, uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->keys(key_start, key_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "Keys function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error scan(const std::string &key_start, const std::string &key_end, uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->scan(key_start, key_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "scan function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error rscan(const std::string &key_start, const std::string &key_end, uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->rscan(key_start, key_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "rscan function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_get(const std::vector<std::string>& keys, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->multi_get(keys, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "multi_get function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_set(const std::map<std::string, std::string> &kvs)
        {
            ssdb::Status st = ssdb_->multi_set(kvs);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "multi_set function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_del(const std::vector<std::string>& keys)
        {
            ssdb::Status st = ssdb_->multi_del(keys);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "multi_del function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        /******************** hash *************************/

        common::Error hget(const std::string& name, const std::string& key, std::string *val)
        {
            ssdb::Status st = ssdb_->hget(name, key, val);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hget function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hset(const std::string& name, const std::string& key, const std::string& val)
        {
            ssdb::Status st = ssdb_->hset(name, key, val);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hset function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hdel(const std::string& name, const std::string& key)
        {
            ssdb::Status st = ssdb_->hdel(name, key);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hdel function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hincr(const std::string &name, const std::string &key, int64_t incrby, int64_t *ret)
        {
            ssdb::Status st = ssdb_->hincr(name, key, incrby, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hincr function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hsize(const std::string &name, int64_t *ret)
        {
            ssdb::Status st = ssdb_->hsize(name, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hset function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hclear(const std::string &name, int64_t *ret)
        {
            ssdb::Status st = ssdb_->hclear(name, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hclear function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hkeys(const std::string &name, const std::string &key_start, const std::string &key_end, uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->hkeys(name, key_start, key_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hkeys function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hscan(const std::string &name, const std::string &key_start, const std::string &key_end, uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->hscan(name, key_start, key_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hscan function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error hrscan(const std::string &name, const std::string &key_start, const std::string &key_end, uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->hrscan(name, key_start, key_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hrscan function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_hget(const std::string &name, const std::vector<std::string> &keys, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->multi_hget(name, keys, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "hrscan function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_hset(const std::string &name, const std::map<std::string, std::string> &keys)
        {
            ssdb::Status st = ssdb_->multi_hset(name, keys);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "multi_hset function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        /******************** zset *************************/

        common::Error zget(const std::string &name, const std::string &key, int64_t *ret)
        {
            ssdb::Status st = ssdb_->zget(name, key, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zget function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zset(const std::string &name, const std::string &key, int64_t score)
        {
            ssdb::Status st = ssdb_->zset(name, key, score);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zset function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zdel(const std::string &name, const std::string &key)
        {
            ssdb::Status st = ssdb_->zdel(name, key);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "Zdel function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zincr(const std::string &name, const std::string &key, int64_t incrby, int64_t *ret)
        {
            ssdb::Status st = ssdb_->zincr(name, key, incrby, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "Zincr function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zsize(const std::string &name, int64_t *ret)
        {
            ssdb::Status st = ssdb_->zsize(name, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zsize function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zclear(const std::string &name, int64_t *ret)
        {
            ssdb::Status st = ssdb_->zclear(name, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zclear function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zrank(const std::string &name, const std::string &key, int64_t *ret)
        {
            ssdb::Status st = ssdb_->zrank(name, key, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zrank function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zrrank(const std::string &name, const std::string &key, int64_t *ret)
        {
            ssdb::Status st = ssdb_->zrrank(name, key, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zrrank function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zrange(const std::string &name,
                uint64_t offset, uint64_t limit,
                std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->zrange(name, offset, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zrange function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zrrange(const std::string &name,
                uint64_t offset, uint64_t limit,
                std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->zrrange(name, offset, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zrrange function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zkeys(const std::string &name, const std::string &key_start,
            int64_t *score_start, int64_t *score_end,
            uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->zkeys(name, key_start, score_start, score_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zkeys function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zscan(const std::string &name, const std::string &key_start,
            int64_t *score_start, int64_t *score_end,
            uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->zscan(name, key_start, score_start, score_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zscan function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error zrscan(const std::string &name, const std::string &key_start,
            int64_t *score_start, int64_t *score_end,
            uint64_t limit, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->zrscan(name, key_start, score_start, score_end, limit, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "zrscan function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_zget(const std::string &name, const std::vector<std::string> &keys,
            std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->multi_zget(name, keys, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "multi_zget function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_zset(const std::string &name, const std::map<std::string, int64_t> &kss)
        {
            ssdb::Status st = ssdb_->multi_zset(name, kss);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "multi_zset function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error multi_zdel(const std::string &name, const std::vector<std::string> &keys)
        {
            ssdb::Status st = ssdb_->multi_zdel(name, keys);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "multi_zdel function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error qpush(const std::string &name, const std::string &item)
        {
            ssdb::Status st = ssdb_->qpush(name, item);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "qpush function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error qpop(const std::string &name, std::string *item)
        {
            ssdb::Status st = ssdb_->qpop(name, item);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "qpop function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error qslice(const std::string &name, int64_t begin, int64_t end, std::vector<std::string> *ret)
        {
            ssdb::Status st = ssdb_->qslice(name, begin, end, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "qslice function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        common::Error qclear(const std::string &name, int64_t *ret)
        {
            ssdb::Status st = ssdb_->qclear(name, ret);
            if (st.error()){
                char buff[1024] = {0};
                common::SNPrintf(buff, sizeof(buff), "qclear function error: %s", st.code());
                return common::make_error_value(buff, common::ErrorValue::E_ERROR);
            }
            return common::Error();
        }

        void init()
        {

        }

        void clear()
        {
            delete ssdb_;
            ssdb_ = NULL;
        }

        ssdb::Client* ssdb_;
    };

    SsdbDriver::SsdbDriver(IConnectionSettingsBaseSPtr settings)
        : IDriver(settings, SSDB), impl_(new pimpl)
    {

    }

    SsdbDriver::~SsdbDriver()
    {
        delete impl_;
    }

    bool SsdbDriver::isConnected() const
    {
        return impl_->isConnected();
    }

    bool SsdbDriver::isAuthenticated() const
    {
        return impl_->isConnected();
    }

    // ============== commands =============//
    common::Error SsdbDriver::commandDeleteImpl(CommandDeleteKey* command, std::string& cmdstring) const
    {
        char patternResult[1024] = {0};
        NDbKValue key = command->key();
        common::SNPrintf(patternResult, sizeof(patternResult), DELETE_KEY_PATTERN_1ARGS_S, key.keyString());
        cmdstring = patternResult;

        return common::Error();
    }

    common::Error SsdbDriver::commandLoadImpl(CommandLoadKey* command, std::string& cmdstring) const
    {
        char patternResult[1024] = {0};
        NDbKValue key = command->key();
        common::Value::Type t = key.type();
        if(t == common::Value::TYPE_ARRAY){
            common::SNPrintf(patternResult, sizeof(patternResult), GET_KEY_LIST_PATTERN_1ARGS_S, key.keyString());
        }
        else if(t == common::Value::TYPE_SET){
            common::SNPrintf(patternResult, sizeof(patternResult), GET_KEY_SET_PATTERN_1ARGS_S, key.keyString());
        }
        else if(t == common::Value::TYPE_ZSET){
            common::SNPrintf(patternResult, sizeof(patternResult), GET_KEY_ZSET_PATTERN_1ARGS_S, key.keyString());
        }
        else if(t == common::Value::TYPE_HASH){
            common::SNPrintf(patternResult, sizeof(patternResult), GET_KEY_HASH_PATTERN_1ARGS_S, key.keyString());
        }
        else{
            common::SNPrintf(patternResult, sizeof(patternResult), GET_KEY_PATTERN_1ARGS_S, key.keyString());
        }
        cmdstring = patternResult;

        return common::Error();
    }

    common::Error SsdbDriver::commandCreateImpl(CommandCreateKey* command, std::string& cmdstring) const
    {
        char patternResult[1024] = {0};
        NDbKValue key = command->key();
        NValue val = command->value();
        common::Value* rval = val.get();
        std::string key_str = key.keyString();
        std::string value_str = common::convertToString(rval, " ");
        common::Value::Type t = key.type();
        if(t == common::Value::TYPE_ARRAY){
            common::SNPrintf(patternResult, sizeof(patternResult), SET_KEY_LIST_PATTERN_2ARGS_SS, key_str, value_str);
        }
        else if(t == common::Value::TYPE_SET){
            common::SNPrintf(patternResult, sizeof(patternResult), SET_KEY_SET_PATTERN_2ARGS_SS, key_str, value_str);
        }
        else if(t == common::Value::TYPE_ZSET){
            common::SNPrintf(patternResult, sizeof(patternResult), SET_KEY_ZSET_PATTERN_2ARGS_SS, key_str, value_str);
        }
        else if(t == common::Value::TYPE_HASH){
            common::SNPrintf(patternResult, sizeof(patternResult), SET_KEY_HASH_PATTERN_2ARGS_SS, key_str, value_str);
        }
        else{
            common::SNPrintf(patternResult, sizeof(patternResult), SET_KEY_PATTERN_2ARGS_SS, key_str, value_str);
        }
        cmdstring = patternResult;

        return common::Error();
    }

    common::Error SsdbDriver::commandChangeTTLImpl(CommandChangeTTL* command, std::string& cmdstring) const
    {
        UNUSED(command);
        UNUSED(cmdstring);
        char errorMsg[1024] = {0};
        common::SNPrintf(errorMsg, sizeof(errorMsg), "Sorry, but now " PROJECT_NAME_TITLE " not supported change ttl command for %s.", common::convertToString(connectionType()));
        return common::make_error_value(errorMsg, common::ErrorValue::E_ERROR);
    }
     // ============== commands =============//

    common::net::hostAndPort SsdbDriver::address() const
    {
        return common::net::hostAndPort(impl_->config_.hostip_, impl_->config_.hostport_);
    }

    std::string SsdbDriver::outputDelemitr() const
    {
        return impl_->config_.mb_delim_;
    }

    const char* SsdbDriver::versionApi()
    {
        return "1.9.2";
    }

    void SsdbDriver::initImpl()
    {
    }

    void SsdbDriver::clearImpl()
    {
    }

    common::Error SsdbDriver::executeImpl(FastoObject* out, int argc, char **argv)
    {
        return impl_->execute_impl(out, argc, argv);
    }

    common::Error SsdbDriver::serverInfo(ServerInfo **info)
    {
        LOG_COMMAND(Command(INFO_REQUEST, common::Value::C_INNER));
        SsdbServerInfo::Common cm;
        common::Error err = impl_->info(NULL, cm);
        if(!err){
            *info = new SsdbServerInfo(cm);
        }

        return err;
    }

    common::Error SsdbDriver::serverDiscoveryInfo(ServerInfo** sinfo, ServerDiscoveryInfo** dinfo, DataBaseInfo **dbinfo)
    {
        ServerInfo *lsinfo = NULL;
        common::Error er = serverInfo(&lsinfo);
        if(er){
            return er;
        }

        FastoObjectIPtr root = FastoObject::createRoot(GET_SERVER_TYPE);
        FastoObjectCommand* cmd = createCommand<SsdbCommand>(root, GET_SERVER_TYPE, common::Value::C_INNER);
        er = execute(cmd);

        if(!er){
            FastoObject::child_container_type ch = root->childrens();
            if(ch.size()){
                //*dinfo = makeOwnRedisDiscoveryInfo(ch[0]);
            }
        }

        DataBaseInfo* ldbinfo = NULL;
        er = currentDataBaseInfo(&ldbinfo);
        if(er){
            delete lsinfo;
            return er;
        }

        *sinfo = lsinfo;
        *dbinfo = ldbinfo;
        return er;
    }

    common::Error SsdbDriver::currentDataBaseInfo(DataBaseInfo** info)
    {
        size_t dbsize = 0;
        impl_->dbsize(dbsize);
        SsdbDataBaseInfo *sinfo = new SsdbDataBaseInfo("0", true, dbsize);
        *info = sinfo;
        return common::Error();
    }

    void SsdbDriver::handleConnectEvent(events::ConnectRequestEvent *ev)
    {
        QObject *sender = ev->sender();
        notifyProgress(sender, 0);
            events::ConnectResponceEvent::value_type res(ev->value());
            SsdbConnectionSettings *set = dynamic_cast<SsdbConnectionSettings*>(settings_.get());
            if(set){
                impl_->config_ = set->info();
                impl_->sinfo_ = set->sshInfo();
        notifyProgress(sender, 25);
                    common::Error er = impl_->connect();
                    if(er){
                        res.setErrorInfo(er);
                    }
        notifyProgress(sender, 75);
            }
            reply(sender, new events::ConnectResponceEvent(this, res));
        notifyProgress(sender, 100);
    }

    void SsdbDriver::handleDisconnectEvent(events::DisconnectRequestEvent* ev)
    {
        QObject *sender = ev->sender();
        notifyProgress(sender, 0);
            events::DisconnectResponceEvent::value_type res(ev->value());
        notifyProgress(sender, 50);

            common::Error er = impl_->disconnect();
            if(er){
                res.setErrorInfo(er);
            }

            reply(sender, new events::DisconnectResponceEvent(this, res));
        notifyProgress(sender, 100);
    }

    void SsdbDriver::handleExecuteEvent(events::ExecuteRequestEvent* ev)
    {
        QObject *sender = ev->sender();
        notifyProgress(sender, 0);
            events::ExecuteRequestEvent::value_type res(ev->value());
            const char *inputLine = common::utils::c_strornull(res.text_);

            common::Error er;
            if(inputLine){
                size_t length = strlen(inputLine);
                int offset = 0;
                RootLocker lock = make_locker(sender, inputLine);
                FastoObjectIPtr outRoot = lock.root_;
                double step = 100.0f/length;
                for(size_t n = 0; n < length; ++n){
                    if(interrupt_){
                        er.reset(new common::ErrorValue("Interrupted exec.", common::ErrorValue::E_INTERRUPTED));
                        res.setErrorInfo(er);
                        break;
                    }
                    if(inputLine[n] == '\n' || n == length-1){
        notifyProgress(sender, step * n);
                        char command[128] = {0};
                        if(n == length-1){
                            strcpy(command, inputLine + offset);
                        }
                        else{
                            strncpy(command, inputLine + offset, n - offset);
                        }
                        offset = n + 1;
                        FastoObjectCommand* cmd = createCommand<SsdbCommand>(outRoot, stableCommand(command), common::Value::C_USER);
                        er = execute(cmd);
                        if(er){
                            res.setErrorInfo(er);
                            break;
                        }
                    }
                }
            }
            else{
                er.reset(new common::ErrorValue("Empty command line.", common::ErrorValue::E_ERROR));
            }

            if(er){
                LOG_ERROR(er, true);
            }
        notifyProgress(sender, 100);
    }

    void SsdbDriver::handleCommandRequestEvent(events::CommandRequestEvent* ev)
    {
        QObject *sender = ev->sender();
        notifyProgress(sender, 0);
            events::CommandResponceEvent::value_type res(ev->value());
            std::string cmdtext;
            common::Error er = commandByType(res.cmd_, cmdtext);
            if(er){
                res.setErrorInfo(er);
                reply(sender, new events::CommandResponceEvent(this, res));
                notifyProgress(sender, 100);
                return;
            }

            RootLocker lock = make_locker(sender, cmdtext);
            FastoObjectIPtr root = lock.root_;
            FastoObjectCommand* cmd = createCommand<SsdbCommand>(root, cmdtext, common::Value::C_INNER);
        notifyProgress(sender, 50);
            er = execute(cmd);
            if(er){
                res.setErrorInfo(er);
            }
            reply(sender, new events::CommandResponceEvent(this, res));
        notifyProgress(sender, 100);
    }

    void SsdbDriver::handleLoadDatabaseInfosEvent(events::LoadDatabasesInfoRequestEvent* ev)
    {
        QObject *sender = ev->sender();
    notifyProgress(sender, 0);
        events::LoadDatabasesInfoResponceEvent::value_type res(ev->value());
    notifyProgress(sender, 50);
        res.databases_.push_back(currentDatabaseInfo());
        reply(sender, new events::LoadDatabasesInfoResponceEvent(this, res));
    notifyProgress(sender, 100);
    }

    void SsdbDriver::handleLoadDatabaseContentEvent(events::LoadDatabaseContentRequestEvent *ev)
    {
        QObject *sender = ev->sender();
        notifyProgress(sender, 0);
            events::LoadDatabaseContentResponceEvent::value_type res(ev->value());
            char patternResult[1024] = {0};
            common::SNPrintf(patternResult, sizeof(patternResult), GET_KEYS_PATTERN_1ARGS_I, res.countKeys_);
            FastoObjectIPtr root = FastoObject::createRoot(patternResult);
        notifyProgress(sender, 50);
            FastoObjectCommand* cmd = createCommand<SsdbCommand>(root, patternResult, common::Value::C_INNER);
            common::Error er = execute(cmd);
            if(er){
                res.setErrorInfo(er);
            }
            else{
                FastoObject::child_container_type rchildrens = cmd->childrens();
                if(rchildrens.size()){
                    DCHECK(rchildrens.size() == 1);
                    FastoObjectArray* array = dynamic_cast<FastoObjectArray*>(rchildrens[0]);
                    if(!array){
                        goto done;
                    }
                    common::ArrayValue* ar = array->array();
                    if(!ar){
                        goto done;
                    }

                    for(int i = 0; i < ar->size(); ++i)
                    {
                        std::string key;
                        bool isok = ar->getString(i, &key);
                        if(isok){
                            NKey k(key);
                            NDbKValue ress(k, NValue());
                            res.keys_.push_back(ress);
                        }
                    }
                }
            }
    done:
        notifyProgress(sender, 75);
            reply(sender, new events::LoadDatabaseContentResponceEvent(this, res));
        notifyProgress(sender, 100);
    }

    void SsdbDriver::handleSetDefaultDatabaseEvent(events::SetDefaultDatabaseRequestEvent* ev)
    {
        QObject *sender = ev->sender();
        notifyProgress(sender, 0);
            events::SetDefaultDatabaseResponceEvent::value_type res(ev->value());
        notifyProgress(sender, 50);
            reply(sender, new events::SetDefaultDatabaseResponceEvent(this, res));
        notifyProgress(sender, 100);
    }

    void SsdbDriver::handleLoadServerInfoEvent(events::ServerInfoRequestEvent* ev)
    {
        QObject *sender = ev->sender();
        notifyProgress(sender, 0);
            events::ServerInfoResponceEvent::value_type res(ev->value());
        notifyProgress(sender, 50);
            LOG_COMMAND(Command(INFO_REQUEST, common::Value::C_INNER));
            SsdbServerInfo::Common cm;
            common::Error err = impl_->info(NULL, cm);
            if(err){
                res.setErrorInfo(err);
            }
            else{
                ServerInfoSPtr mem(new SsdbServerInfo(cm));
                res.setInfo(mem);
            }
        notifyProgress(sender, 75);
            reply(sender, new events::ServerInfoResponceEvent(this, res));
        notifyProgress(sender, 100);
    }

    void SsdbDriver::handleProcessCommandLineArgs(events::ProcessConfigArgsRequestEvent* ev)
    {

    }

    ServerInfoSPtr SsdbDriver::makeServerInfoFromString(const std::string& val)
    {
        ServerInfoSPtr res(makeSsdbServerInfo(val));
        return res;
    }
}
