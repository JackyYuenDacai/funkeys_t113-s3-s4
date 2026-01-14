#!/bin/sh
bt_hciattach="hciattach"

reset_bluetooth_power()
{
	echo 0 > /sys/class/rfkill/rfkill0/state;
	sleep 1
	echo 1 > /sys/class/rfkill/rfkill0/state;
	sleep 1
}

start_hci_attach()
{
	h=`ps | grep "$bt_hciattach" | grep -v grep`
	[ -n "$h" ] && {
		killall "$bt_hciattach"
	}

	# reset_bluetooth_power

	"$bt_hciattach" -n ttyS1 xradio >/dev/null 2>&1 &

	wait_hci0_count=0
	while true
	do
		[ -d /sys/class/bluetooth/hci0 ] && break
		usleep 100000
		let wait_hci0_count++
		[ $wait_hci0_count -eq 70 ] && {
			echo "bring up hci0 failed"
			exit 1
		}
	done
}

start() {

	if [ -d "/sys/class/bluetooth/hci0" ];then
		echo "Bluetooth init has been completed!!"
	else
		start_hci_attach
	fi

	r=`ps | grep hcidump | grep -v grep`
	[ -z "$r" ] && {
		hcidump -z &
	}

	d=`ps | grep bluetoothd | grep -v grep`
	[ -z "$d" ] && {
		# bluetoothd -n &
		/etc/bluetooth/bluetoothd start
		sleep 1
	}
}

ble_start() {
	if [ -d "/sys/class/bluetooth/hci0" ];then
		echo "Bluetooth init has been completed!!"
	else
		start_hci_attach
	fi

	hci_is_up=`hciconfig hci0 | grep UP`
	[ -z "$hci_is_up" ] && {
		hciconfig hci0 up
	}
}

xr819s_stop() {
	hcitool -i hci0 off
	echo 0 > /sys/class/rfkill/rfkill1/state;
	sleep 1
	h=`ps | grep "$bt_hciattach" | grep -v grep`
	[ -n "$h" ] && {
			killall "$bt_hciattach"
			sleep 1
	}
	exit 1
}

stop() {
	d=`ps | grep bluetoothd | grep -v grep`
	[ -n "$d" ] && {
		killall bluetoothd
		sleep 1
	}

	t=`ps | grep hcidump | grep -v grep`
	[ -n "$t" ] && {
		killall hcidump
	}
	# xr819s_stop

	h=`ps | grep "$bt_hciattach" | grep -v grep`
	[ -n "$h" ] && {
		killall "$bt_hciattach"
		sleep 1
	}
	echo 0 > /sys/class/rfkill/rfkill0/state;
	sleep 1
	echo "stop bluetoothd and hciattach"
}

case "$1" in
	start|"")
		start
		;;
	stop)
		stop
		;;
	ble_start)
		ble_start
		;;
	hci_attach)
		start_hci_attach
		;;
	*)
		echo "Usage: $0 {start|stop}"
		exit 1
esac
