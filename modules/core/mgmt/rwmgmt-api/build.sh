#!/usr/bin/make -f

# 
# (c) Copyright RIFT.io, 2013-2016, All Rights Reserved
#


# Renamed this from 'Makefile' to build.sh to avoid conflict w/Cmake

# To run single test
#  make test_foo.py
TEST = $(shell find . -name 'test_*.py')
TEST_OUT = $(firstword $(TMPDIR) /tmp)

check: $(TEST)

.PHONY:$(TEST)
$(TEST):
	RIFT_MODULE_TEST=$(TEST_OUT) PYTHONPATH=. python $@
