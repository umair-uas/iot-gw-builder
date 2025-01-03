#!/bin/bash

ESP32_IP="192.168.0.109"

echo "Testing Home Page..."
curl -k https://$ESP32_IP/ -o index.html

echo "Testing Status Page..."
curl -k https://$ESP32_IP/status -o status.html

echo "Testing Clients Page..."
curl -k https://$ESP32_IP/clients -o clients.html

echo "Testing Settings Page..."
curl -k https://$ESP32_IP/settings -o settings.html

echo "Testing System Info API..."
curl -k https://$ESP32_IP/api/system_info

echo "Testing Wi-Fi Status API..."
curl -k https://$ESP32_IP/api/wifi_status

echo "Testing Clients API..."
curl -k https://$ESP32_IP/api/clients

echo "Testing Non-Existent URI..."
curl -k https://$ESP32_IP/nonexistent

