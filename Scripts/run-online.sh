#!/bin/bash

# (C) 2016 University of Bristol. See License.txt

HERE=$(cd `dirname $0`; pwd)
SPDZROOT=$HERE/..

bits=${2:-128}
g=${3:-40}
mem=${4:-empty}

. $HERE/run-common.sh

Scripts/setup-online.sh 2 ${bits} ${g}

run_player Player-Online.x ${1:-test_all} -lgp ${bits} -lg2 ${g} -m ${mem} || exit 1
