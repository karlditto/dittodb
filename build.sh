#!/usr/bin/env bash

set -xe

gcc -ggdb -Wall -Wextra -o sql *.c -I./
