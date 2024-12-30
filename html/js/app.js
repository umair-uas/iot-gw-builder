// Fetch System Info
async function fetchSystemInfo() {
    try {
        const response = await fetch('/api/system_info');
        if (!response.ok) throw new Error('Failed to fetch system info');
        const data = await response.json();
        updateSystemInfo(data);
    } catch (error) {
        console.error('Error fetching system info:', error);
        showError('system', 'System info unavailable.');
    }
}

// Fetch Wi-Fi Status
async function fetchWifiStatus() {
    try {
        const response = await fetch('/api/wifi_status');
        if (!response.ok) throw new Error('Failed to fetch Wi-Fi status');
        const data = await response.json();
        updateWifiStatus(data);
    } catch (error) {
        console.error('Error fetching Wi-Fi status:', error);
        showError('wifi', 'Wi-Fi status unavailable.');
    }
}

// Fetch Connected Clients
async function fetchClients() {
    try {
        const response = await fetch('/api/clients');
        if (!response.ok) throw new Error('Failed to fetch clients');
        const data = await response.json();
        updateClients(data);
    } catch (error) {
        console.error('Error fetching client count:', error);
        showError('clients', 'Client data unavailable.');
    }
}

// Update System Info on Page
function updateSystemInfo(data) {
    document.getElementById('uptime').innerText = `${data.uptime} seconds`;
    document.getElementById('heap').innerText = `${data.heap_free} bytes`;
    document.getElementById('chip').innerText = data.chip;
    document.getElementById('revision').innerText = data.revision;
    document.getElementById('features').innerText = data.features;
    document.getElementById('flash').innerText = `${data.flash_size}`;
}

// Update Wi-Fi Status on Page
function updateWifiStatus(data) {
    document.getElementById('wifi_ssid').innerText = data.ssid;
    document.getElementById('wifi_ip').innerText = data.ip;
    document.getElementById('wifi_rssi').innerText = `${data.rssi} dBm`;
}

// Update Client Data on Page
function updateClients(data) {
    document.getElementById('client_count').innerText = data.count;
}

// Show Error on Page
function showError(section, message) {
    const element = document.getElementById(`${section}_error`);
    if (element) {
        element.innerText = message;
        element.style.display = 'block';
    }
}

// Load Data Based on Page
function loadPageData() {
    const path = window.location.pathname;

    if (path === '/' || path === '/index.html') {
        fetchSystemInfo();
        fetchWifiStatus();
        fetchClients();
    } else if (path === '/status') {
        fetchSystemInfo();
    } else if (path === '/clients') {
        fetchClients();
    }
}

// Refresh Data Periodically
setInterval(loadPageData, 5000);

// Initial Page Load
loadPageData();
