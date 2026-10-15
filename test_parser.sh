#!/usr/bin/env bash

PASSED=0
FAILED=0

{ ./sql "select * from t1"; } &>/dev/null
if [[ $? == 0 ]]; then
  ((PASSED++))
else
  ((FAILED++))
fi

{ ./sql "create table t1 (id integer, text char)"; } &>/dev/null
if [[ $? == 0 ]]; then
  ((PASSED++))
else
  ((FAILED++))
fi

{ ./sql 'insert into t1 values (1, "hello world")'; } &>/dev/null
if [[ $? == 0 ]]; then
  ((PASSED++))
else
  ((FAILED++))
fi

echo "PASSED=${PASSED}"
echo "FAILED=${FAILED}"
