document.addEventListener("DOMContentLoaded", () => {
    const ledOnButton = document.getElementById("led-on");
    const ledOffButton = document.getElementById("led-off");
    const colorPicker = document.getElementById("color-picker");
    const brightnessSlider = document.getElementById("brightness-slider");
    const applyColorButton = document.getElementById("apply-color");
    const statusMessage = document.getElementById("status-message");

    // Theme toggle functionality
    const themeToggle = document.getElementById('theme-toggle');
    if (themeToggle) {
        const currentTheme = localStorage.getItem('theme') || 'light';
        document.body.classList.add(`${currentTheme}-theme`);
        document.querySelector('.navbar').classList.add(`${currentTheme}-theme`);

        themeToggle.addEventListener('click', function () {
            const newTheme = document.body.classList.contains('light-theme') ? 'dark' : 'light';
            document.body.classList.toggle('light-theme');
            document.body.classList.toggle('dark-theme');
            document.querySelector('.navbar').classList.toggle('light-theme');
            document.querySelector('.navbar').classList.toggle('dark-theme');
            localStorage.setItem('theme', newTheme);
        });
    }

    // Helper function to update status message
    function updateStatus(message, type = "info") {
        statusMessage.textContent = message;
        statusMessage.className = `status-message ${type}`;
    }

    // Send HTTP request to the ESP32
    function sendLedCommand(endpoint, data) {
        fetch(endpoint, {
            method: "POST",
            headers: {
                "Content-Type": "application/json",
            },
            body: JSON.stringify(data),
        })
        .then(response => {
            if (!response.ok) {
                throw new Error(`Server error: ${response.statusText}`);
            }
            return response.json();
        })
        .then(json => {
            updateStatus(`Command sent successfully: ${json.message}`, "success");
        })
        .catch(error => {
            console.error(error);
            updateStatus(`Error: ${error.message}`, "error");
        });
    }

    // Fetch system info and update the UI
    function fetchSystemInfo() {
        fetch("/api/system_info")
            .then(response => response.json())
            .then(data => {
                document.getElementById("wifi-status").textContent = "Connected"; // Assuming Wi-Fi is always connected
                document.getElementById("ip-address").textContent = "192.168.0.109"; // Replace with dynamic IP if available
                document.getElementById("uptime").textContent = "2 hours, 35 minutes"; // Replace with dynamic uptime if available
                document.getElementById("chip").textContent = data.chip;
                document.getElementById("cores").textContent = data.cores;
                document.getElementById("features").textContent = data.features;
                document.getElementById("revision").textContent = data.revision;
                document.getElementById("flash-size").textContent = data.flash_size;
                document.getElementById("heap-free").textContent = data.heap_free;
            })
            .catch(error => {
                console.error("Error fetching system info:", error);
            });
    }

    // Fetch system info on page load
    fetchSystemInfo();

    // Turn LED On
    ledOnButton.addEventListener("click", () => {
        updateStatus("Turning LED on...");
        sendLedCommand("/api/led", { state: "on" });
    });

    // Turn LED Off
    ledOffButton.addEventListener("click", () => {
        updateStatus("Turning LED off...");
        sendLedCommand("/api/led", { state: "off" });
    });

    // Apply selected color
    applyColorButton.addEventListener("click", () => {
        const color = colorPicker.value;
        const brightness = brightnessSlider.value;
        updateStatus(`Applying color ${color} with brightness ${brightness}%...`);
        sendLedCommand("/api/led/color", { color, brightness });
    });

    // Sync brightness slider value display
    brightnessSlider.addEventListener("input", (e) => {
        const brightnessValue = document.getElementById("brightness-value");
        brightnessValue.textContent = `${e.target.value}%`;
    });

    // LED control functionality
    const ledColor = document.getElementById('ledColor');
    const ledBrightness = document.getElementById('ledBrightness');
    const brightnessValue = document.getElementById('brightnessValue');
    const turnOnButton = document.getElementById('turnOnButton');
    const turnOffButton = document.getElementById('turnOffButton');

    if (ledBrightness && brightnessValue) {
        ledBrightness.addEventListener('input', function () {
            brightnessValue.textContent = ledBrightness.value;
            fetch('/api/led/brightness', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ brightness: ledBrightness.value })
            }).then(response => {
                if (!response.ok) {
                    console.error('Failed to set brightness');
                }
            }).catch(error => {
                console.error('Error:', error);
            });
        });
    }

    if (turnOnButton) {
        turnOnButton.addEventListener('click', function () {
            const color = ledColor.value;
            fetch('/api/led/on', {
                method: 'POST',
                headers: {
                    'Content-Type': 'application/json'
                },
                body: JSON.stringify({ color: color })
            }).then(response => {
                if (!response.ok) {
                    console.error('Failed to turn on LED');
                }
            }).catch(error => {
                console.error('Error:', error);
            });
        });
    }

    if (turnOffButton) {
        turnOffButton.addEventListener('click', function () {
            fetch('/api/led/off', {
                method: 'POST'
            }).then(response => {
                if (!response.ok) {
                    console.error('Failed to turn off LED');
                }
            }).catch(error => {
                console.error('Error:', error);
            });
        });
    }
});

