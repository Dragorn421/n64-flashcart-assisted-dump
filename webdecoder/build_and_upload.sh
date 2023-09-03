#!/bin/bash
set -ev

./build.sh

scp -r website 'vps2023:~/webserver/'
