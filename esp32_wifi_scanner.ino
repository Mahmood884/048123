/*
 * ESP32 Advanced WiFi Scanner & Network Tool
 *
 * Features:
 * - Scan all available WiFi networks
 * - Display detailed network information (SSID, BSSID, Channel, RSSI, Encryption)
 * - Create Access Point for web interface
 * - Track connected clients
 * - Educational security information
 *
 * LEGAL NOTICE:
 * This tool is for educational purposes and authorized security testing only.
 * Only use on networks you own or have explicit permission to test.
 */

#include <WiFi.h>
#include <WebServer.h>
#include <esp_wifi.h>

// Access Point Configuration
const char* ap_ssid = "ESP32-WiFi-Scanner";
const char* ap_password = "12345678";

WebServer server(80);

// Network scan results
struct NetworkInfo {
  String ssid;
  String bssid;
  int32_t rssi;
  uint8_t channel;
  uint8_t encryption;
  bool hidden;
};

std::vector<NetworkInfo> scannedNetworks;
std::vector<String> connectedClients;

// Get encryption type as string
String getEncryptionType(uint8_t encType) {
  switch (encType) {
    case WIFI_AUTH_OPEN: return "Open";
    case WIFI_AUTH_WEP: return "WEP";
    case WIFI_AUTH_WPA_PSK: return "WPA-PSK";
    case WIFI_AUTH_WPA2_PSK: return "WPA2-PSK";
    case WIFI_AUTH_WPA_WPA2_PSK: return "WPA/WPA2-PSK";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-Enterprise";
    case WIFI_AUTH_WPA3_PSK: return "WPA3-PSK";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "WPA2/WPA3-PSK";
    default: return "Unknown";
  }
}

// Get security strength indicator
String getSecurityStrength(uint8_t encType) {
  switch (encType) {
    case WIFI_AUTH_OPEN: return "⚠️ Very Weak (No encryption)";
    case WIFI_AUTH_WEP: return "🔴 Weak (WEP is crackable)";
    case WIFI_AUTH_WPA_PSK: return "🟡 Moderate (WPA is outdated)";
    case WIFI_AUTH_WPA2_PSK: return "🟢 Strong";
    case WIFI_AUTH_WPA3_PSK: return "🟢 Very Strong";
    case WIFI_AUTH_WPA2_WPA3_PSK: return "🟢 Very Strong";
    default: return "❓ Unknown";
  }
}

// Scan WiFi networks
void scanNetworks() {
  Serial.println("Starting WiFi scan...");
  scannedNetworks.clear();

  int n = WiFi.scanNetworks(false, true, false, 300);

  Serial.printf("Scan complete. Found %d networks\n", n);

  for (int i = 0; i < n; i++) {
    NetworkInfo net;
    net.ssid = WiFi.SSID(i);
    net.bssid = WiFi.BSSIDstr(i);
    net.rssi = WiFi.RSSI(i);
    net.channel = WiFi.channel(i);
    net.encryption = WiFi.encryptionType(i);
    net.hidden = (net.ssid.length() == 0);

    scannedNetworks.push_back(net);

    Serial.printf("%d: %s (%s) Ch:%d RSSI:%d %s\n",
                  i+1,
                  net.hidden ? "[Hidden Network]" : net.ssid.c_str(),
                  net.bssid.c_str(),
                  net.channel,
                  net.rssi,
                  getEncryptionType(net.encryption).c_str());
  }
}

// Get connected clients to our AP
void updateConnectedClients() {
  wifi_sta_list_t wifi_sta_list;
  tcpip_adapter_sta_list_t adapter_sta_list;

  memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
  memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));

  esp_wifi_ap_get_sta_list(&wifi_sta_list);
  tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);

  connectedClients.clear();

  for (int i = 0; i < adapter_sta_list.num; i++) {
    tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
    char ip_str[16];
    sprintf(ip_str, "%d.%d.%d.%d",
            IP2STR(&station.ip));

    char mac_str[18];
    sprintf(mac_str, "%02X:%02X:%02X:%02X:%02X:%02X",
            station.mac[0], station.mac[1], station.mac[2],
            station.mac[3], station.mac[4], station.mac[5]);

    String clientInfo = String(mac_str) + " - " + String(ip_str);
    connectedClients.push_back(clientInfo);

    Serial.printf("Connected client: %s\n", clientInfo.c_str());
  }
}

// HTML for the web interface
String getHTML() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html dir="rtl" lang="ar">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WiFi Scanner</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            margin: 0;
            padding: 20px;
            direction: rtl;
        }
        .container {
            max-width: 1200px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            box-shadow: 0 10px 40px rgba(0,0,0,0.2);
            padding: 30px;
        }
        h1 {
            color: #667eea;
            text-align: center;
            margin-bottom: 10px;
        }
        .warning {
            background: #fff3cd;
            border: 2px solid #ffc107;
            border-radius: 8px;
            padding: 15px;
            margin: 20px 0;
            text-align: center;
        }
        .warning strong {
            color: #d9534f;
        }
        .controls {
            text-align: center;
            margin: 20px 0;
        }
        button {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            padding: 12px 30px;
            font-size: 16px;
            border-radius: 25px;
            cursor: pointer;
            margin: 5px;
            transition: transform 0.2s;
        }
        button:hover {
            transform: scale(1.05);
        }
        button:active {
            transform: scale(0.95);
        }
        .section {
            margin: 30px 0;
        }
        .section h2 {
            color: #764ba2;
            border-bottom: 3px solid #667eea;
            padding-bottom: 10px;
        }
        table {
            width: 100%;
            border-collapse: collapse;
            margin: 15px 0;
        }
        th {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 12px;
            text-align: right;
        }
        td {
            padding: 10px;
            border-bottom: 1px solid #ddd;
            text-align: right;
        }
        tr:hover {
            background: #f5f5f5;
        }
        .signal-strength {
            display: inline-block;
            width: 100px;
            height: 20px;
            background: #e0e0e0;
            border-radius: 10px;
            overflow: hidden;
        }
        .signal-bar {
            height: 100%;
            border-radius: 10px;
            transition: width 0.3s;
        }
        .signal-excellent { background: #4caf50; }
        .signal-good { background: #8bc34a; }
        .signal-fair { background: #ffc107; }
        .signal-weak { background: #ff9800; }
        .signal-poor { background: #f44336; }
        .stats {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(200px, 1fr));
            gap: 15px;
            margin: 20px 0;
        }
        .stat-card {
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            padding: 20px;
            border-radius: 10px;
            text-align: center;
        }
        .stat-card h3 {
            margin: 0 0 10px 0;
            font-size: 14px;
        }
        .stat-card .value {
            font-size: 32px;
            font-weight: bold;
        }
        .info-box {
            background: #e3f2fd;
            border-left: 4px solid #2196f3;
            padding: 15px;
            margin: 15px 0;
            border-radius: 5px;
        }
        .channel-graph {
            display: flex;
            align-items: flex-end;
            height: 150px;
            margin: 20px 0;
            gap: 5px;
        }
        .channel-bar {
            flex: 1;
            background: #667eea;
            min-height: 5px;
            border-radius: 5px 5px 0 0;
            position: relative;
            transition: height 0.3s;
        }
        .channel-label {
            position: absolute;
            bottom: -20px;
            width: 100%;
            text-align: center;
            font-size: 12px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>🛡️ ESP32 WiFi Scanner & Network Tool</h1>

        <div class="warning">
            <strong>⚠️ تحذير قانوني:</strong> هذه الأداة للأغراض التعليمية والاختبارات المصرح بها فقط.
            استخدمها فقط على الشبكات التي تملكها أو لديك إذن صريح لاختبارها.
        </div>

        <div class="controls">
            <button onclick="scan()">🔍 مسح الشبكات</button>
            <button onclick="getClients()">👥 عرض الأجهزة المتصلة</button>
            <button onclick="location.reload()">🔄 تحديث الصفحة</button>
        </div>

        <div class="stats" id="stats"></div>

        <div class="section">
            <h2>📊 الشبكات المكتشفة</h2>
            <div id="channelGraph" class="channel-graph"></div>
            <div id="networks"></div>
        </div>

        <div class="section">
            <h2>👥 الأجهزة المتصلة بنقطة الوصول</h2>
            <div id="clients"></div>
        </div>

        <div class="section">
            <h2>🎓 معلومات تعليمية عن أمان الشبكات</h2>
            <div class="info-box">
                <h3>أنواع التشفير:</h3>
                <ul>
                    <li><strong>Open:</strong> بدون تشفير - أي شخص يمكنه الاتصال والتجسس على البيانات</li>
                    <li><strong>WEP:</strong> تشفير قديم وضعيف جداً - يمكن كسره في دقائق</li>
                    <li><strong>WPA:</strong> أفضل من WEP لكنه قديم وبه ثغرات معروفة</li>
                    <li><strong>WPA2:</strong> معيار قوي وآمن للاستخدام المنزلي والتجاري</li>
                    <li><strong>WPA3:</strong> أحدث وأقوى معيار تشفير متاح</li>
                </ul>
            </div>
            <div class="info-box">
                <h3>نصائح الأمان:</h3>
                <ul>
                    <li>استخدم WPA2 أو WPA3 دائماً</li>
                    <li>استخدم كلمة مرور قوية (12+ حرف، أرقام، رموز)</li>
                    <li>غير كلمة مرور الراوتر الافتراضية</li>
                    <li>قم بتحديث برنامج الراوتر بانتظام</li>
                    <li>عطل WPS إذا كان متاحاً</li>
                    <li>أخفِ اسم الشبكة (SSID) للحماية الإضافية</li>
                </ul>
            </div>
        </div>
    </div>

    <script>
        function getSignalStrength(rssi) {
            if (rssi > -50) return { class: 'signal-excellent', width: '100%', text: 'ممتاز' };
            if (rssi > -60) return { class: 'signal-good', width: '80%', text: 'جيد' };
            if (rssi > -70) return { class: 'signal-fair', width: '60%', text: 'متوسط' };
            if (rssi > -80) return { class: 'signal-weak', width: '40%', text: 'ضعيف' };
            return { class: 'signal-poor', width: '20%', text: 'ضعيف جداً' };
        }

        function scan() {
            document.getElementById('networks').innerHTML = '<p>جاري المسح... ⏳</p>';
            fetch('/scan')
                .then(response => response.json())
                .then(data => {
                    displayNetworks(data);
                    updateChannelGraph(data);
                    updateStats(data);
                });
        }

        function displayNetworks(networks) {
            if (networks.length === 0) {
                document.getElementById('networks').innerHTML = '<p>لم يتم العثور على شبكات</p>';
                return;
            }

            let html = '<table><thead><tr><th>#</th><th>اسم الشبكة</th><th>MAC Address</th><th>القناة</th><th>قوة الإشارة</th><th>التشفير</th><th>قوة الأمان</th></tr></thead><tbody>';

            networks.forEach((net, index) => {
                const signal = getSignalStrength(net.rssi);
                html += `
                    <tr>
                        <td>${index + 1}</td>
                        <td><strong>${net.ssid || '[شبكة مخفية]'}</strong></td>
                        <td><code>${net.bssid}</code></td>
                        <td>${net.channel}</td>
                        <td>
                            ${net.rssi} dBm
                            <div class="signal-strength">
                                <div class="signal-bar ${signal.class}" style="width: ${signal.width}"></div>
                            </div>
                            ${signal.text}
                        </td>
                        <td>${net.encryption}</td>
                        <td>${net.security}</td>
                    </tr>
                `;
            });

            html += '</tbody></table>';
            document.getElementById('networks').innerHTML = html;
        }

        function updateChannelGraph(networks) {
            const channels = new Array(14).fill(0);
            networks.forEach(net => {
                if (net.channel >= 1 && net.channel <= 14) {
                    channels[net.channel - 1]++;
                }
            });

            const maxCount = Math.max(...channels);
            let html = '';

            for (let i = 0; i < 14; i++) {
                const height = maxCount > 0 ? (channels[i] / maxCount) * 100 : 0;
                html += `
                    <div class="channel-bar" style="height: ${height}%">
                        <div class="channel-label">Ch${i + 1}<br>${channels[i]}</div>
                    </div>
                `;
            }

            document.getElementById('channelGraph').innerHTML = html;
        }

        function updateStats(networks) {
            const openNetworks = networks.filter(n => n.encryption === 'Open').length;
            const weakNetworks = networks.filter(n => n.encryption.includes('WEP') || n.encryption.includes('WPA-PSK')).length;
            const strongNetworks = networks.filter(n => n.encryption.includes('WPA2') || n.encryption.includes('WPA3')).length;

            const html = `
                <div class="stat-card">
                    <h3>إجمالي الشبكات</h3>
                    <div class="value">${networks.length}</div>
                </div>
                <div class="stat-card">
                    <h3>شبكات مفتوحة</h3>
                    <div class="value">${openNetworks}</div>
                </div>
                <div class="stat-card">
                    <h3>شبكات ضعيفة الحماية</h3>
                    <div class="value">${weakNetworks}</div>
                </div>
                <div class="stat-card">
                    <h3>شبكات آمنة</h3>
                    <div class="value">${strongNetworks}</div>
                </div>
            `;

            document.getElementById('stats').innerHTML = html;
        }

        function getClients() {
            document.getElementById('clients').innerHTML = '<p>جاري التحميل... ⏳</p>';
            fetch('/clients')
                .then(response => response.json())
                .then(data => {
                    if (data.length === 0) {
                        document.getElementById('clients').innerHTML = '<p>لا توجد أجهزة متصلة حالياً</p>';
                        return;
                    }

                    let html = '<table><thead><tr><th>#</th><th>MAC Address</th><th>IP Address</th></tr></thead><tbody>';
                    data.forEach((client, index) => {
                        const parts = client.split(' - ');
                        html += `<tr><td>${index + 1}</td><td><code>${parts[0]}</code></td><td><code>${parts[1] || 'N/A'}</code></td></tr>`;
                    });
                    html += '</tbody></table>';
                    document.getElementById('clients').innerHTML = html;
                });
        }

        // Auto-scan on page load
        window.onload = function() {
            scan();
            getClients();
        };

        // Auto-refresh every 30 seconds
        setInterval(() => {
            scan();
            getClients();
        }, 30000);
    </script>
</body>
</html>
)rawliteral";

  return html;
}

// Handle root page
void handleRoot() {
  server.send(200, "text/html", getHTML());
}

// Handle scan request
void handleScan() {
  scanNetworks();

  String json = "[";
  for (size_t i = 0; i < scannedNetworks.size(); i++) {
    if (i > 0) json += ",";
    json += "{";
    json += "\"ssid\":\"" + scannedNetworks[i].ssid + "\",";
    json += "\"bssid\":\"" + scannedNetworks[i].bssid + "\",";
    json += "\"rssi\":" + String(scannedNetworks[i].rssi) + ",";
    json += "\"channel\":" + String(scannedNetworks[i].channel) + ",";
    json += "\"encryption\":\"" + getEncryptionType(scannedNetworks[i].encryption) + "\",";
    json += "\"security\":\"" + getSecurityStrength(scannedNetworks[i].encryption) + "\"";
    json += "}";
  }
  json += "]";

  server.send(200, "application/json", json);
}

// Handle clients request
void handleClients() {
  updateConnectedClients();

  String json = "[";
  for (size_t i = 0; i < connectedClients.size(); i++) {
    if (i > 0) json += ",";
    json += "\"" + connectedClients[i] + "\"";
  }
  json += "]";

  server.send(200, "application/json", json);
}

void setup() {
  Serial.begin(115200);
  delay(1000);

  Serial.println("\n\n=================================");
  Serial.println("ESP32 WiFi Scanner & Network Tool");
  Serial.println("=================================\n");

  // Set WiFi mode to AP + STA
  WiFi.mode(WIFI_AP_STA);

  // Start Access Point
  Serial.println("Starting Access Point...");
  WiFi.softAP(ap_ssid, ap_password);

  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
  Serial.printf("AP SSID: %s\n", ap_ssid);
  Serial.printf("AP Password: %s\n\n", ap_password);

  // Setup web server
  server.on("/", handleRoot);
  server.on("/scan", handleScan);
  server.on("/clients", handleClients);

  server.begin();
  Serial.println("Web server started!");
  Serial.println("\nConnect to the AP and visit: http://192.168.4.1");
  Serial.println("=================================\n");

  // Initial scan
  scanNetworks();
}

void loop() {
  server.handleClient();

  // Periodic scan every 60 seconds
  static unsigned long lastScan = 0;
  if (millis() - lastScan > 60000) {
    scanNetworks();
    updateConnectedClients();
    lastScan = millis();
  }
}
