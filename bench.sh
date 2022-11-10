#!/bin/bash

perf stat -e cycles,instructions,cache-references,cache-misses $1 

perf record -e cycles,instructions,cache-references,cache-misses $1
