DEMO_DIR="${RIFT_INSTALL}/demos"
DEMO_TEST_DIR=$DEMO_DIR/tests
SYSTEM_DIR="${RIFT_INSTALL}/usr/bin"
SYSTEM_TEST_DIR="${RIFT_INSTALL}/usr/rift/systemtest"
PYTEST_DIR="${SYSTEM_TEST_DIR}/pytest"
SYSTEM_TEST_UTIL_DIR="${RIFT_INSTALL}/usr/rift/systemtest/util"

SCRIPT_SYSTEST_WRAPPER="${SYSTEM_TEST_UTIL_DIR}/systest_wrapper.sh"
SCRIPT_SYSTEM="${SYSTEM_TEST_DIR}/mission_control/mission_control.py"
SCRIPT_BOOT_OPENSTACK="${SYSTEM_TEST_UTIL_DIR}/test_openstack_wrapper.py"
SCRIPT_TEST=""
LONG_OPTS="cloud-host:,cloud-type:,dts-trace:,filter:,mark:,wait,repeat:,repeat-system:,wait-system:,repeat-keyword:,repeat-mark:,fail-on-error"
SHORT_OPTS="h:,c:,d:,k:,m:,w,r:,,,,"

cmdargs="${@}"
test_prefix=""
cloud_type='lxc'
cloud_host="127.0.0.1"
repeat=1
filter=""
mark=""
repeat_keyword=""
repeat_mark=""
repeat_system=1
wait_system=1000
test_prefix=""
fail_on_error=false
teardown_wait=false
no_cntr_mgr=true
restconf=false
netconf=true

echo "Comand lines out: $cmdargs"
echo

system_args="\
    --mode ethsim \
    --collapsed"

function append_args()
{
    local args_list=$1
    local arg="$2"
    local val="$3"

    # if val is true " --switch" will be appended
    # if val has non-zero lenght and not a boolean " --argument value" will be appended
    if [ "$val" ] ; then
        if [ "$val" = true ] ; then
            eval $args_list+="' --$arg'"
        elif [ "$val" = false ] ; then
            :
        else
            eval $args_list+="' --$arg $val'"
        fi
    fi
}


function update_up_cmd()
{
    up_cmd="$SYSTEM_TEST_UTIL_DIR/wait_until_system_started.py "
    append_args up_cmd max-wait $wait_system
    append_args up_cmd confd-host $confd_host
}

function construct_test_args()
{
    base_name=${test_prefix}_${cloud_type}
    JUNITXML_FILE=${RIFT_MODULE_TEST}/${base_name}.xml
    JUNIT_PREFIX=${base_name^^}

    append_args test_args restconf ${restconf}
    append_args test_args netconf ${netconf}
    append_args test_args repeat ${repeat}
    append_args test_args cloud-host ${cloud_host}

    append_args test_args junitprefix $JUNIT_PREFIX
    append_args test_args junitxml $JUNITXML_FILE
    append_args test_args ${cloud_type} true
    append_args test_args fail-on-error ${fail_on_error}
    append_args test_args repeat-keyword ${repeat_keyword}
    append_args test_args repeat-mark ${repeat_mark}
    append_args test_args filter ${filter}
    append_args test_args mark ${mark}
    append_args test_args dts-trace ${dts_trace}
}

function construct_test_comand()
{
    append_args systest_args wait "${teardown_wait}"
    construct_test_args

    case "${cloud_type}" in
        lxc)
            update_up_cmd
            systest_args+=" --up_cmd \"${up_cmd}\""
            test_cmd="${SCRIPT_SYSTEST_WRAPPER} ${systest_args}"
            append_args test_cmd system_cmd "\"${SCRIPT_SYSTEM} ${system_args}\""
            append_args test_cmd test_cmd "\"${SCRIPT_TEST} ${test_args}\""
        ;;
        openstack)
            confd_host="CONFD_HOST"
            update_up_cmd

            append_args system_args no-cntr-mgr "${no_cntr_mgr}"
            test_cmd="${SCRIPT_BOOT_OPENSTACK}"
            append_args test_cmd up-cmd "\"${up_cmd}\""
            append_args test_cmd cloud-host "\"${cloud_host}\""
            append_args test_cmd systest-script "\"${SCRIPT_SYSTEST_WRAPPER}\""
            append_args test_cmd systest-args "\"${systest_args}\""
            append_args test_cmd system-script "\"${SCRIPT_SYSTEM}\""
            append_args test_cmd system-args "\"${system_args}\""
            append_args test_cmd test-script "\"${SCRIPT_TEST}\""
            append_args test_cmd test-args "\"${test_args}\""
        ;;
        *)
            echo "error - unrecognized cloud-type type: ${cloud_type}"
           exit 1
        ;;
    esac
}

function parse_args()
{
    if ! ARGPARSE=$(getopt -o ${SHORT_OPTS} -l ${LONG_OPTS} -- "$@")
    then
        exit 1
    fi
    eval set -- "$ARGPARSE"

    while [ $# -gt 0 ]
    do
        case "$1" in
        -h|--cloud-host) cloud_host="$2"
          shift;;
        -c|--cloud-type) cloud_type="$2"
          shift;;
        -d|--dts-trace) dts_trace="${DEMO_DIR}/dts_trace.py -v --client $2"
          shift;;
        --fail-on-error) fail_on_error=true
          ;;
        -k|--filter) filter="$2"
          shift;;
        -m|--mark) mark="$2"
          shift;;
        -w|--wait) teardown_wait=true
          ;;
        --wait-system) wait_system=$2
          shift;;
        -r|--repeat) repeat=$((${2}+1))
          shift;;
        --repeat-system) repeat_system=$((${2}+1))
          shift;;
        --repeat-keyword) repeat_keyword=$2
          shift;;
        --repeat-mark) repeat_mark=$2
          shift;;
        --) shift
          break;;
        -*) echo "$0: error - unrecognized option $1" 1>&2
          exit 1;;
        *) echo "$0: error - not an option $1" 1>&2
          exit 1;;
        esac
        shift
    done
}

function pretty_print_junit_xml()
{
    xmllint --format --output ${JUNITXML_FILE} ${JUNITXML_FILE} 2>/dev/null
}
