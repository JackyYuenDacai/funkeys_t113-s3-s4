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