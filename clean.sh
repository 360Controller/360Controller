#!/bin/bash
for i in 360Controller 360Daemon DriverTool Feedback360 Pref360Control Wireless360Controller WirelessGamingReceiver Install360Controller; do
    rm -rf $i/build
done
rm -rf build
echo "Deleted build products."
