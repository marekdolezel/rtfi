#!/bin/bash

TMP_DIR=$(mktemp -d)
SFMT_ZIP=SFMT-src-1.5.1.zip
push $TMP_DIR >> /dev/null
wget http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/SFMT/$SFMT_ZIP
unzip $SFMT_ZIP
pop $TMP_DIR >> /dev/null