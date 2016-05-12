#!/bin/bash

rm -f marinatb.tgz

tar czf \
  /tmp/marinatb.tgz \
  -C `pwd`/../.. \
  marinatb \
  --exclude='build' \
  --exclude='pkg' \
  --warning=no-file-changed

mv /tmp/marinatb.tgz .
