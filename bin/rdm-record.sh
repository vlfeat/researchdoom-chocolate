#!/bin/bash
# rdm-record.sh -- Record a game session using ResearchDoom

# rdm-record.sh wad/doom2.wad data/demo.lmp data/demo/
# http://doomedsda.us/wad945m240.html

RDM_ROOT="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )/.."
RDM=${RDM_ROOT}/bin/doom
RDM_CONFIG=${RDM_ROOT}/bin/rdm.cfg

(
set -x
${RDM} \
    -iwad "$1" \
    -playdemo "$2" \
    -config ${RDM_ROOT}/etc/rdm.cfg \
    -extraconfig ${RDM_ROOT}/etc/rdm-extra.cfg \
    -rdm-outdir "$3" \
    -rdm-hideplayer \
    -rdm-syncframes \
    -rdm-rgb \
    -rdm-depth \
    -rdm-objects
)
