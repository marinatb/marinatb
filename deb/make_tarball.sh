#!/bin/bash

rm -f marinatb.tgz

tar czf \
  /tmp/marinatb.tgz \
  -C `pwd`/../.. \
  marinatb \
  --exclude='build' \
  --exclude='pkg' \
  --exclude='deb' \
  --exclude='.git' \
  --exclude='.gitignore' \
  --exclude="*.sw*" \
  --exclude=".git*" \
  --warning=no-file-changed

mv /tmp/marinatb.tgz .
