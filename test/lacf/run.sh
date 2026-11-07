#!/bin/sh
# Builds and runs the LACF wrapper test on the host (no board). Run from the repository root.
set -e
L=lib/HybridFrame/src
mkdir -p /tmp/lacf_test
cc -std=c99 -O2 -DHB_FAST -c $L/hb.c -o /tmp/lacf_test/hb.o
cc -std=c99 -O2 -DHB_FAST -c $L/hb3.c -o /tmp/lacf_test/hb3.o
cc -std=c99 -O2 -DHB_FAST -c $L/hbi.c -o /tmp/lacf_test/hbi.o
cc -std=c99 -O2 -DHB_FAST -c $L/hbc.c -o /tmp/lacf_test/hbc.o
c++ -std=c++17 -O1 -DUNIT_TEST -I$L -Itest/lacf test/lacf/lacf_wrapper_test.cpp $L/hybrid_frame.cpp $L/lacf_capability.cpp /tmp/lacf_test/hb.o /tmp/lacf_test/hb3.o /tmp/lacf_test/hbi.o /tmp/lacf_test/hbc.o -o /tmp/lacf_test/lacf_test
/tmp/lacf_test/lacf_test
