// Fetch System Info
async function fetchSystemInfo() {
    try {
        const response = await fetch('/api/system_info');
        const data = await response.json();
        document.getElementById('uptime').innerText = `${data.uptime} seconds`;
        document.getElementById('heap').innerText = `${data.heap} bytes`;
    } catch (error) {
        console.error('Error fetching system info:', error);
    }
}

// Fetch Wi-Fi Status
async function fetchWifiStatus() {
    try {
        const response = await fetch('/api/wifi_status');
        const data = await response.json();
        document.getElementById('wifi_ssid').innerText = data.ssid;
        document.getElementById('wifi_ip').innerText = data.ip;
        document.getElementById('wifi_rssi').innerText = `${data.rssi} dBm`;
    } catch (error) {
        console.error('Error fetching Wi-Fi status:', error);
    }
}

// Fetch Connected Clients
async function fetchClientCount() {
    try {
        const response = await fetch('/api/clients');
        const data = await response.json();
        document.getElementById('client_count').innerText = data.count;
    } catch (error) {
        console.error('Error fetching client count:', error);
    }
}

// Refresh Data Periodically
setInterval(() => {
    fetchSystemInfo();
    fetchWifiStatus();
    fetchClientCount();
}, 5000);

// Initial Fetch
fetchSystemInfo();
fetchWifiStatus();
fetchClientCount();

