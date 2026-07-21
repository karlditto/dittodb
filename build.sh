#!/usr/bin/env bash

set -xe

shopt -s extglob

gcc -ggdb -Wall -Wextra -o sql !(test).c -I./
