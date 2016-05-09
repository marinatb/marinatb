#!/bin/bash

#-------------------------------------------------------------------------------
#
#     This script sets up an Ubuntu mirror for the marina testbed. It installs
#   apache2 to serve the mirror.
#
#-------------------------------------------------------------------------------

sudo apt-get install -y debmirror apache2

sudo mkdir -p /space/debmirror/ubuntu
sudo chown -R root:sudo /space/debmirror
sudo chmod -R 571 /space/debmirror

sudo mkdir -p /space/debmirror/keyring
sudo gpg \
  --no-default-keyring \
  --keyring /space/debmirror/keyring/trustedkeys.gpg \
  --import /usr/share/keyrings/ubuntu-archive-keyring.gpg

./rebuild-ubuntu-mirror.sh

sudo ln -s /space/debmirror/ubuntu /var/www/ubuntu

sudo cp rebuild-ubuntu-mirror.sh /etc/cron.daily/
