#! /bin/sh

${CONFD_DIR}/bin/confd_load -F x -o | xsltproc xsl-show-snmp.xsl -

exit 0
