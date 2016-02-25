#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <confd.h>
#include <confd_cdb.h>
#include <confd_maapi.h>

#undef _POSIX_C_SOURCE

#include "zconfd_api.h"
#include "quagga.h"     /* generated from yang */
#include "confd_global.h"

#define LOG_COMMAND(data, command, args...) cli_command( all, data, ".*\\(config\\)# ", 1, CLI_ERR_STR, command , ##args)

static int get_log_setting(cb_data_t *data, int set);
static enum cdb_iter_ret update_log(cb_data_t *data);

int confd_log_setup(struct confd_daemon_ctx *dctx) {
        int ret;
        DEBUG_LOG( "confd_log_setup ");

        extern int quagga_not_running;
        if(quagga_not_running) return CONFD_OK;

        zconfd_subscribe_cdb(dctx, LOG_PRIO, update_log, "log", NULL);

        cb_data_t data;
        zconfd_init_cb_data_ctx(&data, dctx, NULL, MOP_CREATED, NULL, NULL, NULL);

        cli_configure_command(1, all);

        CHECK_CONFD( cdb_pushd( data.datasock, "log"), "log: Failed to pushd 'log' - init");
        ret = get_log_setting( &data,  1);      //operation is ignored
        CHECK_CONFD( cdb_popd( data.datasock), "log: Failed to popd log - init");
        if ( ret == CONFD_ERR)
                return ret;

        cli_configure_command(0, all);

        DEBUG_LOG( "confd_log_setup end" );
        return CONFD_OK;
}

static enum cdb_iter_ret update_log(cb_data_t *data) {
        int ret;
        int ind = data->kp_start;

        DUMP_UPDATE( __FUNCTION__, data);

        if ( ind < 0 && data->op == MOP_DELETED) {
                CHECK_CONFD( cdb_pushd( data->datasock, "log"), "log: Failed to pushd 'log' - deleted");
          ret = get_log_setting( data,  0); //operation is ignored
                CHECK_CONFD( cdb_popd( data->datasock), "log: Failed to popd log - deleted");
                if ( ret == CONFD_ERR)
                        return ret;
          return ITER_STOP;
        }

        if ( CONFD_GET_XMLTAG(&data->kp->v[ind][0]) == NSPREF(log)) {
                CHECK_CONFD( cdb_pushd( data->datasock, "log"), "log: Failed to pushd 'log'");
                ret = get_log_setting( data,  1);       //operation is ignored
                CHECK_CONFD( cdb_popd( data->datasock), "log: Failed to popd log");
                if ( ret == CONFD_ERR)
                        return ret;
        }

        return ITER_STOP;
}

static int get_log_setting(cb_data_t *data, int set) {
        //<elem name="facility" type="LogFacilityType" minOccurs="0"/>
        if ( cdb_exists( data->datasock, "facility") == 1)
                LOG_COMMAND(data, "log facility ${ex: facility}");
        else
                LOG_COMMAND(data, "no log facility");

        //<elem name="file/name" type="xs:string"/>
        //<elem name="file/level" type="LogLevelType"/>
        if ( cdb_exists( data->datasock, "file/name") == 1)
                LOG_COMMAND(data, "log file ${ex: file/name} ${ex: file/level}");
        else
                LOG_COMMAND(data, "no log file");

        //<elem name="monitor" type="LogLevelType" minOccurs="0"/>
        if ( cdb_exists( data->datasock, "monitor") == 1)
                LOG_COMMAND(data, "log monitor ${ex: monitor}");
        else
                LOG_COMMAND(data, "no log monitor");

        //<elem name="stdout" type="LogLevelType" minOccurs="0"/>
        if ( cdb_exists( data->datasock, "stdout") == 1)
                LOG_COMMAND(data, "log stdout ${ex: stdout}");
        else
                LOG_COMMAND(data, "no log stdout");

        //<elem name="syslog" type="LogLevelType" minOccurs="0"/>
        if ( cdb_exists( data->datasock, "syslog") == 1)
                LOG_COMMAND(data, "log syslog ${ex: syslog}");
        else
                LOG_COMMAND(data, "no log syslog");

        //<elem name="record-priority" type="xs:boolean" minOccurs="0"/>
        if ( cdb_exists( data->datasock, "record-priority") == 1)
                LOG_COMMAND(data, "${bool: record-priority, '', 'no '}log record-priority");
        else
                LOG_COMMAND(data, "no log record-priority");

        return CONFD_OK;
}
