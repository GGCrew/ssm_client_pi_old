Expand File System
Change Locale (en.US UTF-8)
Change Timezone (America - Pacific)
Change Keyboard (Generic 101, US)
Change Memory Split to 256MB


sudo apt-get install
	subversion
	vim
	htop
	


vim update_ssm_client
	svn export --force svn://net-night.com/snapshowmagic/R_Pi/development/snap_show_magic/ssm_client.bin ./
	chmod +x ssm_client.bin

	mkdir ./images
	svn export --force svn://net-night.com/snapshowmagic/R_Pi/development/snap_show_magic/images/black.png ./images/
chmod +x update_ssm_client
./update_ssm_client


sudo vim /etc/network/interfaces
	auto wlan0
	allow-hotplug wlan0
	iface wlan0 inet dhcp
	wireless-essid SnapShowMagic
sudo ifdown wlan0
sudo ifup wlan0






