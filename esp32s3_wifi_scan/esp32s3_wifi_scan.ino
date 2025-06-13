#include <esp_netif.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <WiFiClientSecure.h>
#include <esp_wifi.h> // 添加WiFi控制功能
#include <lwip/dns.h> // 添加DNS功能

// AP热点设置
String ap_ssid = "ESP32-S3-Config";
String ap_password = "";
bool ap_open = true; // true=无密码，false=有密码

WebServer server(80);
String scanResult = "";
String message = "";

Preferences preferences;
#define MAX_WIFI_HISTORY 5
String wifi_ssids[MAX_WIFI_HISTORY];
String wifi_pwds[MAX_WIFI_HISTORY];
int wifi_count = 0;

// DeepSeek 聊天相关变量
String deepseekApiKey = "";
String chatHistory = "";
String currentInput = "";

// 对话上下文管理
#define MAX_CONTEXT 5  // 最大上下文轮数
String context[MAX_CONTEXT]; // 存储对话上下文
int contextIndex = 0;

// 对话模式
#define MODE_LONG 0
#define MODE_MULTI 1
int chatMode = MODE_LONG; // 默认长文模式

// 主页HTML模板
String htmlPage(String body) {
  return R"(
<html>
<head>
  <meta charset='utf-8'>
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32-S3配置</title>
  <style>
    :root {
      --primary: #2a5d9f;
      --primary-dark: #17406a;
      --error: #d32f2f;
      --success: #388e3c;
      --gray: #f4f6fa;
      --text: #333;
    }
    
    * {
      box-sizing: border-box;
      margin: 0;
      padding: 0;
    }
    
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: var(--gray);
      color: var(--text);
      line-height: 1.6;
      padding: 16px;
    }
    
    .container {
      max-width: 500px;
      margin: 20px auto;
      background: #fff;
      padding: 24px;
      border-radius: 12px;
      box-shadow: 0 4px 12px rgba(0,0,0,0.08);
    }
    
    h1, h2, h3 {
      color: var(--primary);
      margin-bottom: 16px;
    }
    
    h1 {
      font-size: 1.8rem;
      text-align: center;
      padding-bottom: 12px;
      border-bottom: 1px solid #eee;
    }
    
    h2 {
      font-size: 1.4rem;
      margin-top: 24px;
    }
    
    h3 {
      font-size: 1.2rem;
      color: #444;
    }
    
    .message {
      padding: 12px;
      border-radius: 6px;
      margin: 16px 0;
      text-align: center;
    }
    
    .error {
      background: #ffebee;
      color: var(--error);
      border: 1px solid #ffcdd2;
    }
    
    .success {
      background: #e8f5e9;
      color: var(--success);
      border: 1px solid #c8e6c9;
    }
    
    .info {
      background: #e3f2fd;
      color: #1976d2;
      border: 1px solid #bbdefb;
    }
    
    ul {
      padding: 0;
      margin: 16px 0;
    }
    
    li {
      list-style: none;
      margin: 12px 0;
      padding: 0;
    }
    
    button, input, .btn, textarea {
      padding: 12px 16px;
      border-radius: 6px;
      border: 1px solid #ddd;
      font-size: 1rem;
      width: 100%;
      margin: 8px 0;
      transition: all 0.2s;
    }
    
    input, textarea {
      background: #fafafa;
    }
    
    input:focus, textarea:focus {
      outline: none;
      border-color: var(--primary);
      box-shadow: 0 0 0 2px rgba(42, 93, 159, 0.2);
    }
    
    button, .btn {
      background: var(--primary);
      color: white;
      border: none;
      font-weight: 500;
      cursor: pointer;
      text-align: center;
      text-decoration: none;
      display: block;
    }
    
    button:hover, .btn:hover {
      background: var(--primary-dark);
      transform: translateY(-2px);
      box-shadow: 0 4px 8px rgba(0,0,0,0.1);
    }
    
    .wifi-item {
      display: flex;
      align-items: center;
      background: #f9f9f9;
      border-radius: 6px;
      padding: 12px;
      border: 1px solid #eee;
    }
    
    .wifi-icon {
      margin-right: 12px;
      font-size: 1.5rem;
    }
    
    .wifi-details {
      flex-grow: 1;
    }
    
    .wifi-ssid {
      font-weight: 600;
    }
    
    .wifi-meta {
      font-size: 0.9rem;
      color: #666;
      margin-top: 4px;
    }
    
    .footer {
      text-align: center;
      margin-top: 24px;
      padding-top: 16px;
      border-top: 1px solid #eee;
      color: #777;
      font-size: 0.9rem;
    }
    
    .loading {
      display: inline-block;
      width: 20px;
      height: 20px;
      border: 3px solid rgba(255,255,255,0.3);
      border-radius: 50%;
      border-top-color: #fff;
      animation: spin 1s ease-in-out infinite;
      margin-right: 8px;
      vertical-align: middle;
    }
    
    @keyframes spin {
      to { transform: rotate(360deg); }
    }
    
    .hidden {
      display: none;
    }
    
    .history-item {
      display: flex;
      justify-content: space-between;
      align-items: center;
      padding: 10px;
      background: #f5f9ff;
      border-radius: 6px;
      margin-bottom: 8px;
    }
    
    .delete-btn {
      background: #f44336;
      width: auto;
      padding: 6px 12px;
      font-size: 0.9rem;
    }
    
    /* 聊天样式 */
    .chat-container {
      border: 1px solid #ddd;
      border-radius: 8px;
      padding: 12px;
      margin: 16px 0;
      max-height: 300px;
      overflow-y: auto;
      background: #f9f9f9;
    }
    
    .chat-message {
      margin-bottom: 10px;
      padding: 8px 12px;
      border-radius: 12px;
    }
    
    .user-message {
      background: #e3f2fd;
      text-align: left;
      margin-left: 10%;
      border-bottom-right-radius: 0;
    }
    
    .ai-message {
      background: #e8f5e9;
      text-align: left;
      margin-right: 10%;
      border-bottom-left-radius: 0;
    }
    
    .timestamp {
      font-size: 0.7rem;
      color: #777;
      margin-top: 4px;
      text-align: right;
    }
    
    .clear-btn {
      background: #f44336;
      margin-top: 10px;
    }
    
    .api-status {
      font-size: 0.9rem;
      color: #666;
      margin-top: 5px;
    }
    
    .context-info {
      font-size: 0.8rem;
      color: #666;
      margin-top: 5px;
      text-align: center;
    }
  </style>
</head>
<body>
<div class="container">
  <h1>ESP32-S3 配置面板</h1>
  <div id="status" class="message info">
    <div class="loading"></div> 正在加载...
  </div>
  <div id="content">
  )" + body + R"(
  </div>
  <div class="footer">
    ESP32-S3 | 固件版本 2.5 | 2025
  </div>
</div>
<script>
  window.onload = function() {
    document.getElementById('status').classList.add('hidden');
    document.getElementById('content').classList.remove('hidden');
  };
  
  document.querySelectorAll('form').forEach(form => {
    form.addEventListener('submit', function() {
      document.getElementById('status').classList.remove('hidden');
      document.getElementById('status').textContent = "处理中，请稍候...";
    });
  });
</script>
</body>
</html>
)";
}

// 扫描WiFi并生成可选列表
String scanWiFi() {
  int n = WiFi.scanNetworks();
  if (n == 0) return "<p class='message info'>未发现WiFi网络</p>";
  
  String result = "<h2>可用的WiFi网络</h2><ul>";
  for (int i = 0; i < n; ++i) {
    String ssid = WiFi.SSID(i);
    int rssi = WiFi.RSSI(i);
    String security = (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "开放网络" : "加密网络";
    
    String signalIcon = "";
    if (rssi >= -50) signalIcon = "📶";
    else if (rssi >= -70) signalIcon = "📶";
    else signalIcon = "📶";
    
    result += "<li class='wifi-item'>";
    result += "<div class='wifi-icon'>" + signalIcon + "</div>";
    result += "<div class='wifi-details'>";
    result += "<div class='wifi-ssid'>" + ssid + "</div>";
    result += "<div class='wifi-meta'>" + security + " | 信号: " + String(rssi) + "dBm</div>";
    result += "</div>";
    result += "<form style='display:inline;' action='/select' method='get'>";
    result += "<input type='hidden' name='ssid' value='" + ssid + "'>";
    result += "<button type='submit'>选择</button>";
    result += "</form>";
    result += "</li>";
  }
  result += "</ul>";
  return result;
}

// 保存历史WiFi
void saveWiFiHistory(String ssid, String pwd) {
  for (int i = 0; i < wifi_count; ++i) {
    if (wifi_ssids[i] == ssid) {
      wifi_pwds[i] = pwd;
      goto save;
    }
  }
  
  if (wifi_count < MAX_WIFI_HISTORY) {
    wifi_ssids[wifi_count] = ssid;
    wifi_pwds[wifi_count] = pwd;
    wifi_count++;
  } else {
    for (int i = 1; i < MAX_WIFI_HISTORY; ++i) {
      wifi_ssids[i-1] = wifi_ssids[i];
      wifi_pwds[i-1] = wifi_pwds[i];
    }
    wifi_ssids[MAX_WIFI_HISTORY-1] = ssid;
    wifi_pwds[MAX_WIFI_HISTORY-1] = pwd;
  }

save:
  preferences.begin("wifi", false);
  preferences.putUInt("count", wifi_count);
  for (int i = 0; i < wifi_count; ++i) {
    preferences.putString(("ssid"+String(i)).c_str(), wifi_ssids[i]);
    preferences.putString(("pwd"+String(i)).c_str(), wifi_pwds[i]);
  }
  preferences.end();
}

// 删除历史WiFi
void deleteWiFiHistory(int index) {
  if (index < 0 || index >= wifi_count) return;
  
  for (int i = index; i < wifi_count - 1; ++i) {
    wifi_ssids[i] = wifi_ssids[i+1];
    wifi_pwds[i] = wifi_pwds[i+1];
  }
  wifi_count--;
  
  preferences.begin("wifi", false);
  preferences.putUInt("count", wifi_count);
  for (int i = 0; i < wifi_count; ++i) {
    preferences.putString(("ssid"+String(i)).c_str(), wifi_ssids[i]);
    preferences.putString(("pwd"+String(i)).c_str(), wifi_pwds[i]);
  }
  for (int i = wifi_count; i < MAX_WIFI_HISTORY; ++i) {
    preferences.remove(("ssid"+String(i)).c_str());
    preferences.remove(("pwd"+String(i)).c_str());
  }
  preferences.end();
}

// 读取历史WiFi
void loadWiFiHistory() {
  preferences.begin("wifi", true);
  wifi_count = preferences.getUInt("count", 0);
  for (int i = 0; i < wifi_count && i < MAX_WIFI_HISTORY; ++i) {
    wifi_ssids[i] = preferences.getString(("ssid"+String(i)).c_str(), "");
    wifi_pwds[i] = preferences.getString(("pwd"+String(i)).c_str(), "");
  }
  preferences.end();
}

// 读取DeepSeek API密钥
void loadDeepSeekKey() {
  preferences.begin("deepseek", true);
  deepseekApiKey = preferences.getString("apikey", "");
  preferences.end();
}

// 保存DeepSeek API密钥
void saveDeepSeekKey(String key) {
  deepseekApiKey = key;
  preferences.begin("deepseek", false);
  preferences.putString("apikey", deepseekApiKey);
  preferences.end();
}

// 保存聊天历史
void saveChatHistory() {
  preferences.begin("chat", false);
  preferences.putString("history", chatHistory);
  preferences.end();
}

// 加载聊天历史
void loadChatHistory() {
  preferences.begin("chat", true);
  chatHistory = preferences.getString("history", "");
  preferences.end();
}

// 清除聊天历史
void clearChatHistory() {
  chatHistory = "";
  // 同时清除上下文
  for (int i = 0; i < MAX_CONTEXT; i++) {
    context[i] = "";
  }
  contextIndex = 0;
  saveChatHistory();
}

// 添加消息到上下文
void addToContext(String role, String content) {
  String msg = "{\"role\":\"" + role + "\",\"content\":\"" + content + "\"}";
  
  // 如果上下文已满，移除最旧的一条
  if (contextIndex >= MAX_CONTEXT) {
    for (int i = 0; i < MAX_CONTEXT - 1; i++) {
      context[i] = context[i + 1];
    }
    contextIndex = MAX_CONTEXT - 1;
  }
  
  context[contextIndex] = msg;
  contextIndex++;
}

// 自动连接历史WiFi
bool autoConnectWiFi() {
  for (int i = 0; i < wifi_count; ++i) {
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP(ap_ssid, ap_password);
    WiFi.begin(wifi_ssids[i].c_str(), wifi_pwds[i].c_str());
    
    int t = 0;
    while (WiFi.status() != WL_CONNECTED && t < 20) {
      delay(500);
      t++;
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.printf("自动连接到历史WiFi: %s\n", wifi_ssids[i].c_str());
      return true;
    }
  }
  return false;
}

// 在主页显示历史WiFi
String historyWiFiHtml() {
  if (wifi_count == 0) return "";
  
  String html = "<h2>已保存的WiFi</h2><ul>";
  for (int i = 0; i < wifi_count; ++i) {
    html += "<li class='history-item'>";
    html += "<div>" + wifi_ssids[i] + "</div>";
    html += "<form style='display:inline;' action='/delete' method='post'>";
    html += "<input type='hidden' name='index' value='" + String(i) + "'>";
    html += "<button class='delete-btn' type='submit'>删除</button>";
    html += "</form>";
    html += "</li>";
  }
  html += "</ul>";
  return html;
}

// 保存AP设置
void saveAPConfig() {
  preferences.begin("ap", false);
  preferences.putString("ssid", ap_ssid);
  preferences.putString("pwd", ap_password);
  preferences.putBool("open", ap_open);
  preferences.end();
}

// 读取AP设置
void loadAPConfig() {
  preferences.begin("ap", true);
  ap_ssid = preferences.getString("ssid", "ESP32-S3-Config");
  ap_password = preferences.getString("pwd", "");
  ap_open = preferences.getBool("open", true);
  preferences.end();
}

// 热点设置表单
String apConfigForm() {
  String html = "<h2>热点设置</h2>";
  html += "<form action='/apconfig' method='post'>";
  html += "<label>热点名称(SSID)：</label>";
  html += "<input type='text' name='ssid' value='" + ap_ssid + "' maxlength='32' required><br>";
  html += "<label>密码(8-63字符，可留空为无密码)：</label>";
  html += "<input type='password' name='password' value='" + ap_password + "' maxlength='63' minlength='8' ";
  if (ap_open) html += "disabled";
  html += "><br>";
  html += "<label><input type='checkbox' name='open' value='1' ";
  if (ap_open) html += "checked";
  html += "> 无密码(开放网络)</label><br>";
  html += "<button type='submit'>保存并重启热点</button>";
  html += "</form>";
  html += "<div class='api-status'>当前热点：<b>" + ap_ssid + "</b> | " + (ap_open ? "无密码" : ("密码：" + ap_password)) + "</div>";
  return html;
}

// 处理热点设置
void handleAPConfig() {
  String ssid = server.arg("ssid");
  String pwd = server.arg("password");
  bool open = server.hasArg("open");
  if (ssid.length() < 1 || ssid.length() > 32) {
    message = "SSID长度需1-32字符";
  } else if (!open && (pwd.length() < 8 || pwd.length() > 63)) {
    message = "密码需8-63字符，或选择无密码";
  } else {
    ap_ssid = ssid;
    ap_password = pwd;
    ap_open = open;
    saveAPConfig();
    WiFi.softAPdisconnect(true);
    delay(200);
    if (ap_open) {
      WiFi.softAP(ap_ssid.c_str());
    } else {
      WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
    }
    message = "热点设置已保存并重启";
  }
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// 主页
void handleRoot() {
  String body = "<h2>设备配置</h2>";
  
  if (message != "") {
    String msgClass = (message.indexOf("成功") != -1) ? "success" : "error";
    body += "<div class='message " + msgClass + "'>" + message + "</div>";
    message = "";
  }
  
  body += scanWiFi();
  body += historyWiFiHtml();
  
  body += "<div style='margin-top:24px;'>";
  body += "<a class='btn' href='/chat'>DeepSeek聊天</a>";
  body += "</div>";
  
  body += "<form action='/scan' method='post'>";
  body += "<button type='submit'>扫描WiFi网络</button>";
  body += "</form>";
  
  body += apConfigForm();
  
  server.send(200, "text/html", htmlPage(body));
}

// 处理扫描请求
void handleScan() {
  scanResult = scanWiFi();
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// 选择WiFi，显示密码输入表单
void handleSelect() {
  String ssid = server.arg("ssid");
  String body = "<h2>连接WiFi</h2>";
  body += "<form action='/connect' method='post'>";
  body += "<input type='hidden' name='ssid' value='" + ssid + "'>";
  body += "<label>WiFi名称：<b>" + ssid + "</b></label><br><br>";
  body += "<label>密码：</label><input type='password' name='password' placeholder='输入密码' required><br><br>";
  body += "<button type='submit'>连接</button>";
  body += "<a class='btn' href='/' style='background:#757575;margin-top:12px;text-align:center;'>返回</a>";
  body += "</form>";
  server.send(200, "text/html", htmlPage(body));
}

// 连接指定WiFi
void handleConnect() {
  String ssid = server.arg("ssid");
  String pwd = server.arg("password");
  
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP(ap_ssid, ap_password);
  WiFi.disconnect(true);
  delay(100);
  
  WiFi.begin(ssid.c_str(), pwd.c_str());
  
  int t = 0;
  while (WiFi.status() != WL_CONNECTED && t < 20) {
    delay(500);
    t++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    saveWiFiHistory(ssid, pwd);
    String ip = WiFi.localIP().toString();
    String apip = WiFi.softAPIP().toString();
    message = "连接成功!<br><br>";
    message += "<b>设备IP:</b> " + ip + "<br>";
    message += "<b>热点IP:</b> " + apip + "<br><br>";
    message += "您现在可以通过设备IP或热点访问配置页面";
  } else {
    message = "连接失败，请检查：<br>- 密码是否正确<br>- 信号强度是否足够";
    WiFi.mode(WIFI_AP);
    WiFi.softAP(ap_ssid, ap_password);
  }
  
  scanResult = "";
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// 删除历史WiFi
void handleDelete() {
  int index = server.arg("index").toInt();
  
  if (index >= 0 && index < wifi_count) {
    String ssid = wifi_ssids[index];
    deleteWiFiHistory(index);
    message = "已删除: " + ssid;
  } else {
    message = "删除失败: 无效的索引";
  }
  
  server.sendHeader("Location", "/", true);
  server.send(302, "text/plain", "");
}

// DeepSeek聊天页面
void handleChatForm() {
  loadDeepSeekKey(); // 确保加载最新密钥
  loadChatHistory(); // 加载聊天记录
  
  String body = "<h2>DeepSeek聊天</h2>";
  
  // API密钥设置表单
  body += "<h3>API密钥设置</h3>";
  body += "<form action='/savekey' method='post'>";
  body += "<input type='password' name='apikey' value='" + deepseekApiKey + "' placeholder='输入DeepSeek API密钥' required>";
  body += "<button type='submit'>保存密钥</button>";
  body += "</form>";
  
  // 显示API状态
  body += "<div class='api-status'>";
  body += (deepseekApiKey != "") ? "API密钥已保存" : "未设置API密钥";
  body += "</div>";
  
  // 上下文信息
  body += "<div class='context-info'>";
  body += "当前上下文: " + String(contextIndex) + "/" + String(MAX_CONTEXT) + " 轮对话";
  body += "</div>";
  
  // 聊天界面
  body += "<h3>聊天对话</h3>";
  body += "<div class='chat-container'>";
  body += (chatHistory != "") ? chatHistory : "<p>暂无聊天记录，请发送消息开始对话</p>";
  body += "</div>";
  
  // 输入表单
  body += "<form action='/chat' method='post'>";
  body += "<textarea name='message' rows='2' placeholder='输入消息...' required></textarea>";
  body += "<button type='submit'>发送</button>";
  body += "</form>";
  
  // 操作按钮
  body += "<div style='display:flex; gap:10px; margin-top:10px;'>";
  body += "<form action='/export' method='get' style='flex:1;'>";
  body += "<button type='submit' style='background:#4CAF50;'>导出聊天记录</button>";
  body += "</form>";
  
  body += "<form action='/clearchat' method='post' style='flex:1;'>";
  body += "<button type='submit' class='clear-btn'>清空聊天</button>";
  body += "</form>";
  body += "</div>";
  
  // 聊天模式切换
  body += "<div style='margin:10px 0;'>";
  body += "<form action='/switchmode' method='post' style='display:inline;'>";
  body += "<input type='hidden' name='mode' value='long'>";
  body += "<button type='submit'";
  if (chatMode == MODE_LONG) body += " style='background:#2a5d9f;'";
  body += ">长文模式(3轮)</button></form> ";
  body += "<form action='/switchmode' method='post' style='display:inline;'>";
  body += "<input type='hidden' name='mode' value='multi'>";
  body += "<button type='submit'";
  if (chatMode == MODE_MULTI) body += " style='background:#2a5d9f;'";
  body += ">多轮模式(10轮,短回复)</button></form>";
  body += "</div>";
  body += "<div class='api-status'>当前模式: ";
  body += (chatMode == MODE_LONG) ? "长文模式(支持3轮长回复)" : "多轮模式(支持10轮,每次不超30字)";
  body += "</div>";
  
  // 返回按钮
  body += "<a class='btn' href='/' style='background:#757575;margin-top:16px;text-align:center;'>返回主页</a>";
  
  server.send(200, "text/html", htmlPage(body));
}

// 保存API密钥
void handleSaveKey() {
  String newKey = server.arg("apikey");
  if (newKey.length() > 10) {
    saveDeepSeekKey(newKey);
    message = "API密钥已保存";
  } else {
    message = "错误: 无效的API密钥";
  }
  server.sendHeader("Location", "/chat", true);
  server.send(302, "text/plain", "");
}

// 处理聊天请求
void handleChatQuery() {
  if (deepseekApiKey == "") {
    message = "错误: 请先设置API密钥";
    server.sendHeader("Location", "/chat", true);
    server.send(302, "text/plain", "");
    return;
  }
  
  currentInput = server.arg("message");
  currentInput.trim();
  if (currentInput.length() == 0) {
    message = "消息不能为空，请输入内容。";
    server.sendHeader("Location", "/chat", true);
    server.send(302, "text/plain", "");
    return;
  }
  
  // 添加用户消息到历史
  String timestamp = "时间: " + String(millis() / 1000);
  chatHistory += "<div class='chat-message user-message'>";
  chatHistory += "<div><strong>你:</strong> " + currentInput + "</div>";
  chatHistory += "<div class='timestamp'>" + timestamp + "</div>";
  chatHistory += "</div>";
  
  saveChatHistory(); // 保存聊天记录
  
  // 添加用户消息到上下文
  addToContext("user", currentInput);
  
  // 调用DeepSeek API
  HTTPClient http;
  WiFiClientSecure client;  // 使用安全客户端
  client.setInsecure();     // 跳过证书验证
  
  String url = "https://api.deepseek.com/chat/completions";
  
  if (!http.begin(client, url)) {
    chatHistory += "<div class='chat-message ai-message error'>";
    chatHistory += "<strong>连接错误:</strong> 无法连接到API服务器";
    chatHistory += "</div>";
    saveChatHistory();
    server.sendHeader("Location", "/chat", true);
    server.send(302, "text/plain", "");
    return;
  }
  
  http.addHeader("Content-Type", "application/json");
  http.addHeader("Authorization", "Bearer " + deepseekApiKey);
  http.addHeader("Accept", "application/json");
  
  // 构建包含上下文的请求体，只保留最近2轮
  int maxRounds = (chatMode == MODE_LONG) ? 3 : 10;
  String systemPrompt = "";
  if (chatMode == MODE_MULTI) systemPrompt = "请每次回答不超过30字，简明扼要。";
  String requestBody = "{\"model\":\"deepseek-chat\",\"messages\":[";
  if (systemPrompt != "") {
    requestBody += "{\"role\":\"system\",\"content\":\"" + systemPrompt + "\"},";
  }
  int startIdx = contextIndex > maxRounds ? contextIndex - maxRounds : 0;
  bool first = true;
  for (int i = startIdx; i < contextIndex; i++) {
    // 只拼接content非空的消息
    int contentPos = context[i].indexOf("\"content\":\"");
    if (contentPos != -1) {
      int contentEnd = context[i].indexOf("\"", contentPos + 11);
      String contentStr = context[i].substring(contentPos + 11, contentEnd);
      if (contentStr.length() > 0) {
        if (!first) requestBody += ",";
        requestBody += context[i];
        first = false;
      }
    }
  }
  requestBody += "],\"temperature\":0.7,\"max_tokens\":1024,\"stream\":false}";
  Serial.print("请求体长度: ");
  Serial.println(requestBody.length());
  Serial.println("发送请求体: " + requestBody);
  int httpCode = http.POST(requestBody);
  String response = "";
  
  if (httpCode == HTTP_CODE_OK) {
    WiFiClient* stream = http.getStreamPtr();
    int totalLen = http.getSize();
    Serial.print("API响应Content-Length: ");
    Serial.println(totalLen);
    int len = totalLen;
    char buff[129];
    while (http.connected() && (len > 0 || len == -1)) {
      size_t size = stream->available();
      if (size) {
        int c = stream->readBytes(buff, ((size > sizeof(buff) - 1) ? sizeof(buff) - 1 : size));
        if (c > 0) {
          response.concat((const char*)buff, c); // 只拼接实际读取的内容
        }
        if (len > 0) len -= c;
      }
      delay(1);
    }
    Serial.print("API响应实际长度: ");
    Serial.println(response.length());
    Serial.print("API响应前100字符: ");
    Serial.println(response.substring(0, 100));
    Serial.print("API响应后100字符: ");
    Serial.println(response.substring(response.length() - 100));
    // 打印完整响应内容
    Serial.println("API完整响应:");
    Serial.println(response);
    response.trim();
    // 去除chunked编码的头，只保留JSON
    int jsonStart = response.indexOf('{');
    int jsonEnd = response.lastIndexOf('}');
    if (jsonStart >= 0 && jsonEnd > jsonStart) {
      response = response.substring(jsonStart, jsonEnd + 1);
    }
    DynamicJsonDocument resDoc(16384); // 增大缓冲区以处理长回复
    DeserializationError error = deserializeJson(resDoc, response);
    if (!error) {
      if (resDoc.containsKey("choices") && resDoc["choices"].size() > 0) {
        String aiResponse = resDoc["choices"][0]["message"]["content"].as<String>();
        Serial.print("AI回复内容: ");
        Serial.println(aiResponse);

        // 添加AI回复到历史
        chatHistory += "<div class='chat-message ai-message'>";
        chatHistory += "<div><strong>AI:</strong> " + aiResponse + "</div>";
        chatHistory += "<div class='timestamp'>" + timestamp + "</div>";
        chatHistory += "</div>";

        // 添加AI回复到上下文
        addToContext("assistant", aiResponse);
        saveChatHistory();
      } else {
        chatHistory += "<div class='chat-message ai-message error'>";
        chatHistory += "<strong>错误:</strong> 无AI回复</div>";
        Serial.println("choices为空或不存在");
        saveChatHistory();
      }
    } else {
      chatHistory += "<div class='chat-message ai-message error'>";
      chatHistory += "<strong>解析错误:</strong> " + String(error.c_str());
      chatHistory += "</div>";
      Serial.print("JSON解析失败，错误: ");
      Serial.println(error.c_str());
      Serial.println("原始响应内容:");
      Serial.println(response);
      saveChatHistory();
    }
    Serial.print("httpCode: ");
    Serial.println(httpCode);
    Serial.print("chatHistory: ");
    Serial.println(chatHistory);
  } else {
    String errorMsg = http.errorToString(httpCode);
    String apiResp = http.getString();
    Serial.print("API错误响应内容: ");
    Serial.println(apiResp);
    chatHistory += "<div class='chat-message ai-message error'>";
    if (httpCode == 400) {
      chatHistory += "<strong>API错误:</strong> 请求内容过长或格式不合法，请缩短对话或内容。";
    } else {
      chatHistory += "<strong>API错误:</strong> " + String(httpCode) + " - " + errorMsg;
    }
    chatHistory += "</div>";
    saveChatHistory();
  }
  
  http.end();
  saveChatHistory(); // 保存更新后的聊天记录
  server.sendHeader("Location", "/chat", true);
  server.send(302, "text/plain", "");
}

// 导出聊天记录
void handleExportChat() {
  if (chatHistory.length() == 0) {
    server.send(200, "text/plain", "没有聊天记录可导出");
    return;
  }
  
  // 创建纯文本聊天记录
  String txtContent = "=== ESP32-S3 DeepSeek聊天记录 ===\n\n";
  txtContent += "导出时间: " + String(millis() / 1000) + "\n\n";
  
  // 转换HTML为纯文本
  String plainHistory = chatHistory;
  plainHistory.replace("</div>", "\n");
  plainHistory.replace("<div class='chat-message user-message'>", "[用户] ");
  plainHistory.replace("<div class='chat-message ai-message'>", "[AI] ");
  plainHistory.replace("<div class='chat-message ai-message error'>", "[错误] ");
  plainHistory.replace("<strong>你:</strong> ", "");
  plainHistory.replace("<strong>AI:</strong> ", "");
  plainHistory.replace("<strong>错误:</strong> ", "");
  plainHistory.replace("<div>", "");
  plainHistory.replace("</div>", "");
  plainHistory.replace("<div class='timestamp'>", "时间: ");
  
  // 移除HTML标签
  while (plainHistory.indexOf('<') != -1) {
    int start = plainHistory.indexOf('<');
    int end = plainHistory.indexOf('>', start);
    if (end != -1) {
      plainHistory = plainHistory.substring(0, start) + plainHistory.substring(end + 1);
    } else {
      break;
    }
  }
  
  txtContent += plainHistory;
  
  // 发送为下载文件
  server.sendHeader("Content-Type", "text/plain");
  server.sendHeader("Content-Disposition", "attachment; filename=deepseek_chatlog.txt");
  server.send(200, "text/plain", txtContent);
}

// 清除聊天记录
void handleClearChat() {
  clearChatHistory();
  server.sendHeader("Location", "/chat", true);
  server.send(302, "text/plain", "");
}

// 聊天模式切换
void handleSwitchMode() {
  String mode = server.arg("mode");
  if (mode == "long") chatMode = MODE_LONG;
  else if (mode == "multi") chatMode = MODE_MULTI;
  server.sendHeader("Location", "/chat", true);
  server.send(302, "text/plain", "");
}

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n=== ESP32-S3 WiFi配置 ===");
  
  // 初始化上下文数组
  for (int i = 0; i < MAX_CONTEXT; i++) {
    context[i] = "";
  }
  
  // 初始化存储
  preferences.begin("wifi", false);
  preferences.end();
  
  // 加载历史WiFi
  loadWiFiHistory();
  
  // 加载API密钥
  loadDeepSeekKey();
  
  // 加载聊天历史
  loadChatHistory();
  
  // 加载AP设置
  loadAPConfig();
  
  // 启动AP模式
  if (ap_open) {
    WiFi.softAP(ap_ssid.c_str());
  } else {
    WiFi.softAP(ap_ssid.c_str(), ap_password.c_str());
  }
  
  // 尝试自动连接
  bool connected = autoConnectWiFi();
  
  if (connected) {
    Serial.println("已连接到WiFi");
    Serial.print("设备IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("AP模式已启动");
    Serial.print("热点名称: ");
    Serial.println(ap_ssid);
    Serial.print("热点密码: ");
    Serial.println(ap_password);
    Serial.print("AP IP地址: ");
    Serial.println(WiFi.softAPIP());
  }

  // 设置路由
  server.on("/", HTTP_GET, handleRoot);
  server.on("/scan", HTTP_POST, handleScan);
  server.on("/select", HTTP_GET, handleSelect);
  server.on("/connect", HTTP_POST, handleConnect);
  server.on("/delete", HTTP_POST, handleDelete);
  server.on("/chat", HTTP_GET, handleChatForm);
  server.on("/chat", HTTP_POST, handleChatQuery);
  server.on("/savekey", HTTP_POST, handleSaveKey);
  server.on("/export", HTTP_GET, handleExportChat);
  server.on("/clearchat", HTTP_POST, handleClearChat);
  server.on("/switchmode", HTTP_POST, handleSwitchMode);
  server.on("/apconfig", HTTP_POST, handleAPConfig);
  
  // 启动服务器
  server.begin();
  Serial.println("HTTP服务器已启动");
}

void loop() {
  server.handleClient();
  
  delay(2);
}
