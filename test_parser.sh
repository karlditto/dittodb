#!/usr/bin/env bash

PASSED=0
FAILED=0

{ ./sql "select * from t1"; } &>/dev/null
if [[ $? == 0 ]]; then
  ((PASSED++))
else
  ((FAILED++))
fi

{ ./sql "create table t1 (id int, text char(8)"; } &>/dev/null
if [[ $? == 0 ]]; then
  ((PASSED++))
else
  ((FAILED++))
fi

echo "PASSED=${PASSED}"
echo "FAILED=${FAILED}"
