#!/bin/bash

#-------------------------------------------------------------------------------
#
#     This script creates an Ubuntu mirror for the marina testbed. It is run 
#   daily on the marinatb server to keep things up to date.
#
#-------------------------------------------------------------------------------

#set up arguments to send to debmirror
export GNUPGHOME=/space/debmirror/keyring
arch=amd64
section=main,restricted,universe,multiverse
release=xenial,xenial-security,xenial-updates
server=us.archive.ubuntu.com
inPath=/ubuntu
proto=http
outPath=/space/debmirror/ubuntu

#run debmirror
sudo -E debmirror \
  -a $arch \
  --no-source \
  -s $section \
  -h $server \
  -d $release \
  -r $inPath \
  --progress \
  --method=$proto \
  $outPath

