#include "core/rocksdb/rocksdb_config.h"

#include "common/sprintf.h"
#include "common/utils.h"

#include "fasto/qt/logger.h"

namespace fastonosql
{
    namespace
    {
        int parseOptions(int argc, char **argv, rocksdbConfig& cfg) {
            int i;

            for (i = 0; i < argc; i++) {
                int lastarg = i==argc-1;

                if (!strcmp(argv[i],"-h") && !lastarg) {
                    common::utils::freeifnotnull(cfg.hostip_);
                    cfg.hostip_ = strdup(argv[++i]);
                }
                else if (!strcmp(argv[i],"-p") && !lastarg) {
                    cfg.hostport_ = atoi(argv[++i]);
                }
                else if (!strcmp(argv[i],"-d") && !lastarg) {
                    common::utils::freeifnotnull(cfg.mb_delim_);
                    cfg.mb_delim_ = strdup(argv[++i]);
                }
                else if (!strcmp(argv[i], "-f") && !lastarg) {
                    cfg.dbname_ = argv[++i];
                }
                else if (!strcmp(argv[i],"-c")) {
                    cfg.options_.create_if_missing = true;
                }
                else {
                    if (argv[i][0] == '-') {
                        const uint16_t size_buff = 256;
                        char buff[size_buff] = {0};
                        common::SNPrintf(buff, sizeof(buff), "Unrecognized option or bad number of args for: '%s'", argv[i]);
                        LOG_MSG(buff, common::logging::L_WARNING, true);
                        break;
                    } else {
                        /* Likely the command name, stop here. */
                        break;
                    }
                }
            }
            return i;
        }
    }

    rocksdbConfig::rocksdbConfig()
       : ConnectionConfig("127.0.0.1", 1111)
    {
    }

    rocksdbConfig::rocksdbConfig(const rocksdbConfig &other)
        : ConnectionConfig(other.hostip_, other.hostport_)
    {
        copy(other);
    }

    rocksdbConfig& rocksdbConfig::operator=(const rocksdbConfig &other)
    {
        copy(other);
        return *this;
    }

    void rocksdbConfig::copy(const rocksdbConfig& other)
    {
        using namespace common::utils;
        dbname_ = other.dbname_;
        options_ = other.options_;
        ConnectionConfig::copy(other);
    }

    rocksdbConfig::~rocksdbConfig()
    {
    }
}

namespace common
{
    std::string convertToString(const fastonosql::rocksdbConfig &conf)
    {
        std::vector<std::string> argv = conf.args();

        if(!conf.dbname_.empty()){
            argv.push_back("-f");
            argv.push_back(conf.dbname_);
        }

        if(conf.options_.create_if_missing){
            argv.push_back("-c");
        }

        std::string result;
        for(int i = 0; i < argv.size(); ++i){
            result+= argv[i];
            if(i != argv.size()-1){
                result+=" ";
            }
        }

        return result;
    }

    template<>
    fastonosql::rocksdbConfig convertFromString(const std::string& line)
    {
        fastonosql::rocksdbConfig cfg;
        enum { kMaxArgs = 64 };
        int argc = 0;
        char *argv[kMaxArgs] = {0};

        char* p2 = strtok((char*)line.c_str(), " ");
        while(p2){
            argv[argc++] = p2;
            p2 = strtok(0, " ");
        }

        fastonosql::parseOptions(argc, argv, cfg);
        return cfg;
    }
}
