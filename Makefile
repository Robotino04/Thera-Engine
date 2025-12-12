#!/usr/bin/env bash

.PHONY: test untest monitor all clean

TEST_PROFILE ?= release
ifeq (${TEST_PROFILE},debug)
TEST_PROFILE = dev
endif

ifeq (${TEST_PROFILE},dev)
TEST_BIN = target/debug/thera
else
TEST_BIN = target/${TEST_PROFILE}/thera
endif

RES_DIR = resources/

THERA_SRC = $(shell find src/ -type f -name '*.rs')

TESTING_BOOK = ${RES_DIR}/book-ply6-unifen_Q.txt
TESTING_BOOK_ARGS = -openings file="${TESTING_BOOK}" format=epd order=random policy=default

COMMON_TEST_ARGS = \
	-engine "name=Thera Latest" cmd=${TEST_BIN} \
	-engine "name=Thera ${REFERENCE_ENGINE}" cmd=${REFERENCE_ENGINE} \
	-games 1000000 -concurrency $(shell nproc) \
	-recover \
	-repeat 2 \
	-each tc=1 timemargin=200 proto=uci \
	-ratinginterval 10 \
	${TESTING_BOOK_ARGS}

MONITOR_FILE = /tmp/match.log

all: ${TEST_BIN}

${TEST_BIN}: ${THERA_SRC}
	cargo build --profile ${TEST_PROFILE}

test: ${TESTING_BOOK} ${TEST_BIN} ${REFERENCE_ENGINE}
	cutechess-cli ${COMMON_TEST_ARGS} -sprt elo0=0 elo1=10 alpha=0.05 beta=0.05 | tee ${MONITOR_FILE}

untest: ${TESTING_BOOK} ${TEST_BIN} ${REFERENCE_ENGINE}
	cutechess-cli ${COMMON_TEST_ARGS} -sprt elo0=-10 elo1=0 alpha=0.05 beta=0.05 | tee ${MONITOR_FILE}

monitor: monitor.awk
	tail -F /tmp/match.log | awk -f monitor.awk

clean:
	cargo clean
