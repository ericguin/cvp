#!/usr/bin/env sh

gcc -o cvp -Wall -Werror -fsanitize=address -g -Og main.c
