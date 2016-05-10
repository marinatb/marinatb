#space for mirror to go
sudo mkdir -p /space/apt-mirror
sudo chown -R root:sudo /space/apt-mirror
sudo chmod -R 571 /space/apt-mirror

#setup apache
sudo apt-get install -y apt-mirror apache2
sudo cp 000-default.conf /etc/apache2/sites-enabled/
sudo service apache2 restart

#setup deb-mirror & run
sudo cp mirror.list /etc/apt/
sudo apt-mirror

sudo ln -s /var/www/ubuntu /space/apt-mirror/mirror/archive.ubuntu.com/ubuntu

