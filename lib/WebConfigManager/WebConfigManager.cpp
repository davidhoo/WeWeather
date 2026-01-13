#include "WebConfigManager.h"
#include "../LogManager/LogManager.h"

// 将HTML模板存储在PROGMEM中以节省RAM（仅包含英文部分）
const char HTML_HEAD[] PROGMEM = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\"><meta name=\"viewport\" content=\"width=device-width,initial-scale=1.0\"><title>WeWeather</title><style>body{font-family:Arial;margin:20px;background:#f5f5f5}.container{max-width:400px;margin:0 auto;background:white;padding:20px;border-radius:8px;box-shadow:0 2px 8px rgba(0,0,0,0.1)}h1{text-align:center;color:#333;margin-bottom:20px}.form-group{margin-bottom:15px}label{display:block;margin-bottom:5px;font-weight:bold;color:#555}input{width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;font-size:14px;box-sizing:border-box}input:focus{border-color:#4CAF50;outline:none}.btn-group{text-align:center;margin-top:20px}button{background:#4CAF50;color:white;padding:10px 20px;border:none;border-radius:4px;cursor:pointer;font-size:14px;margin:0 5px}button:hover{background:#45a049}.exit-btn{background:#f44336}.exit-btn:hover{background:#da190b}.info{background:#e7f3ff;border:1px solid #b3d9ff;padding:10px;border-radius:4px;margin-bottom:15px;font-size:13px}</style></head><body><div class=\"container\">";

const char HTML_FOOT[] PROGMEM = "</div></body></html>";

// 包含中文的模板使用String对象动态构建
String getConfigForm() {
    return String("<h1>WeWeather 配置</h1>") +
           "<div class=\"info\"><strong>说明：</strong>配置完成后点击保存，设备将重启并应用新配置。</div>" +
           "<form method=\"POST\" action=\"/save\">" +
           "<div class=\"form-group\"><label>WiFi名称:</label><input type=\"text\" name=\"ssid\" value=\"%s\" placeholder=\"请输入WiFi名称\"></div>" +
           "<div class=\"form-group\"><label>WiFi密码:</label><input type=\"text\" name=\"password\" value=\"%s\" placeholder=\"请输入WiFi密码\"></div>" +
           "<div class=\"form-group\"><label>城市代码:</label><input type=\"text\" name=\"citycode\" value=\"%s\" placeholder=\"例如：110108\"></div>" +
           "<div class=\"form-group\"><label>API Key:</label><input type=\"text\" name=\"apikey\" value=\"%s\" placeholder=\"请输入高德地图API密钥\"></div>" +
           "<div class=\"form-group\"><label>MAC地址:</label><input type=\"text\" name=\"mac\" value=\"%s\" placeholder=\"例如：AA:BB:CC:DD:EE:FF\"></div>" +
           "<div class=\"btn-group\"><button type=\"submit\">保存配置</button><button type=\"button\" class=\"exit-btn\" onclick=\"location.href='/exit'\">退出配置</button></div>" +
           "</form>";
}

String getSuccessPage() {
    return String("<h1 style=\"color:#4CAF50\">✓ 配置保存成功</h1>") +
           "<p>配置已保存，设备将在 <span id=\"countdown\" style=\"color:#f44336;font-weight:bold\">3</span> 秒后重启。</p>" +
           "<script>let c=3;setInterval(()=>{document.getElementById('countdown').textContent=--c;if(c<=0)document.body.innerHTML='<div class=\"container\"><h1>设备重启中...</h1></div>';},1000);</script>";
}

String getErrorPage() {
    return String("<h1 style=\"color:#f44336\">✗ 配置保存失败</h1>") +
           "<p>配置保存过程中出现错误，请重试。</p>" +
           "<div class=\"btn-group\"><button onclick=\"location.href='/config'\">重新配置</button><button class=\"exit-btn\" onclick=\"location.href='/exit'\">退出配置</button></div>";
}

String getExitPage() {
    return String("<h1 style=\"color:#f44336\">正在退出配置模式</h1>") +
           "<p>设备将在 <span id=\"countdown\" style=\"color:#f44336;font-weight:bold\">3</span> 秒后重启</p>" +
           "<p>感谢使用 WeWeather！</p>" +
           "<script>let c=3;setInterval(()=>{document.getElementById('countdown').textContent=--c;if(c<=0)document.body.innerHTML='<div class=\"container\"><h1>设备重启中...</h1></div>';},1000);</script>";
}

/**
 * @brief 构造函数
 * @param configMgr 配置管理器指针
 */
WebConfigManager::WebConfigManager(ConfigManager<ConfigData>* configMgr)
    : configManager(configMgr), webServer(nullptr), isConfigMode(false) {
}

/**
 * @brief 析构函数
 */
WebConfigManager::~WebConfigManager() {
    if (webServer) {
        delete webServer;
        webServer = nullptr;
    }
}

/**
 * @brief 启动Web服务器
 * @param port 服务器端口，默认80
 * @return true 如果启动成功，false 如果失败
 */
bool WebConfigManager::startWebServer(int port) {
    LOG_INFO_F("Starting web server on port %d...", port);
    
    if (webServer) {
        delete webServer;
    }
    
    webServer = new ESP8266WebServer(port);
    
    // 设置Web路由
    setupWebRoutes();
    
    // 启动服务器
    webServer->begin();
    
    LOG_INFO("Web server started successfully");
    return true;
}

/**
 * @brief 停止Web服务器
 */
void WebConfigManager::stopWebServer() {
    if (webServer) {
        LOG_INFO("Stopping web server...");
        webServer->stop();
        delete webServer;
        webServer = nullptr;
        LOG_INFO("Web server stopped");
    }
}

/**
 * @brief 启动Web配置服务
 * 启动Web服务器，准备接收配置请求
 * @return true 如果启动成功，false 如果失败
 */
bool WebConfigManager::startConfigService() {
    LOG_INFO("Starting web configuration service...");
    
    isConfigMode = true;
    
    // 启动Web服务器，AP由main.cpp管理
    if (!startWebServer()) {
        LOG_ERROR("Failed to start web server");
        return false;
    }
    
    LOG_INFO("Web configuration service started successfully");
    LOG_INFO_F("Open browser and go to: http://%s", WiFi.softAPIP().toString().c_str());
    
    return true;
}

/**
 * @brief 处理Web请求
 * 处理客户端的Web请求，需要在主循环中调用
 */
void WebConfigManager::handleClient() {
    if (webServer && isConfigMode) {
        webServer->handleClient();
    }
}

/**
 * @brief 设置Web路由
 */
void WebConfigManager::setupWebRoutes() {
    if (!webServer) return;
    
    // 根路径 - 重定向到配置页面
    webServer->on("/", [this]() { handleRoot(); });
    
    // 配置页面
    webServer->on("/config", [this]() { handleConfig(); });
    
    // 保存配置
    webServer->on("/save", HTTP_POST, [this]() { handleSave(); });
    
    // 退出配置模式
    webServer->on("/exit", [this]() { handleExit(); });
    
    // 404处理
    webServer->onNotFound([this]() { handleNotFound(); });
}

/**
 * @brief 处理根路径请求
 */
void WebConfigManager::handleRoot() {
    LOG_INFO("Handling root request");
    webServer->sendHeader("Location", "/config");
    webServer->send(302, "text/plain", "");
}

/**
 * @brief 处理配置页面请求
 */
void WebConfigManager::handleConfig() {
    LOG_INFO("Handling config page request");
    String html = generateConfigPage();
    webServer->sendHeader("Content-Type", "text/html; charset=UTF-8");
    webServer->send(200, "text/html", html);
}

/**
 * @brief 处理保存配置请求
 */
void WebConfigManager::handleSave() {
    LOG_INFO("Handling save config request");
    
    // 读取现有配置
    ConfigData config;
    if (!configManager->read(config)) {
        // 如果读取失败，初始化为空配置
        memset(&config, 0, sizeof(config));
    }
    
    // 获取表单数据并更新配置
    if (webServer->hasArg("ssid")) {
        String ssid = webServer->arg("ssid");
        strncpy(config.wifiSSID, ssid.c_str(), sizeof(config.wifiSSID) - 1);
        config.wifiSSID[sizeof(config.wifiSSID) - 1] = '\0';
    }
    
    if (webServer->hasArg("password")) {
        String password = webServer->arg("password");
        strncpy(config.wifiPassword, password.c_str(), sizeof(config.wifiPassword) - 1);
        config.wifiPassword[sizeof(config.wifiPassword) - 1] = '\0';
    }
    
    if (webServer->hasArg("citycode")) {
        String citycode = webServer->arg("citycode");
        strncpy(config.cityCode, citycode.c_str(), sizeof(config.cityCode) - 1);
        config.cityCode[sizeof(config.cityCode) - 1] = '\0';
    }
    
    if (webServer->hasArg("apikey")) {
        String apikey = webServer->arg("apikey");
        strncpy(config.amapApiKey, apikey.c_str(), sizeof(config.amapApiKey) - 1);
        config.amapApiKey[sizeof(config.amapApiKey) - 1] = '\0';
    }
    
    if (webServer->hasArg("mac")) {
        String mac = webServer->arg("mac");
        strncpy(config.macAddress, mac.c_str(), sizeof(config.macAddress) - 1);
        config.macAddress[sizeof(config.macAddress) - 1] = '\0';
    }
    
    // 保存配置
    bool success = configManager->write(config);
    
    if (success) {
        LOG_INFO("Configuration saved successfully");
        // 发送成功页面，然后自动退出配置模式
        String html = generateSuccessPage();
        webServer->sendHeader("Content-Type", "text/html; charset=UTF-8");
        webServer->send(200, "text/html", html);
        
        // 延迟3秒后自动退出配置模式，与串口模式保持一致
        delay(3000);
        exitConfigMode();
    } else {
        LOG_ERROR("Failed to save configuration");
        // 发送错误页面
        String html = generateErrorPage();
        webServer->sendHeader("Content-Type", "text/html; charset=UTF-8");
        webServer->send(500, "text/html", html);
    }
}

/**
 * @brief 处理退出配置请求
 */
void WebConfigManager::handleExit() {
    LOG_INFO("Handling exit request");
    String html = generateExitPage();
    webServer->sendHeader("Content-Type", "text/html; charset=UTF-8");
    webServer->send(200, "text/html", html);
    
    // 延迟退出配置模式
    delay(2000);
    exitConfigMode();
}

/**
 * @brief 处理404请求
 */
void WebConfigManager::handleNotFound() {
    LOG_INFO("Handling 404 request");
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += webServer->uri();
    message += "\nMethod: ";
    message += (webServer->method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += webServer->args();
    message += "\n";
    
    for (uint8_t i = 0; i < webServer->args(); i++) {
        message += " " + webServer->argName(i) + ": " + webServer->arg(i) + "\n";
    }
    
    webServer->send(404, "text/plain", message);
}

/**
 * @brief 生成配置页面HTML
 */
String WebConfigManager::generateConfigPage() {
    // 读取当前配置
    ConfigData config;
    bool configValid = configManager->read(config);
    
    // 使用栈上的缓冲区，避免动态分配
    char buffer[1200];  // 足够容纳整个页面
    
    // 准备配置值，如果无效则使用空字符串
    const char* ssid = configValid ? config.wifiSSID : "";
    const char* password = configValid ? config.wifiPassword : "";
    const char* citycode = configValid ? config.cityCode : "";
    const char* apikey = configValid ? config.amapApiKey : "";
    const char* mac = configValid ? config.macAddress : "";
    
    // 使用sprintf格式化页面内容
    String formTemplate = getConfigForm();
    snprintf(buffer, sizeof(buffer), formTemplate.c_str(), ssid, password, citycode, apikey, mac);
    
    String html;
    html.reserve(1400);  // 预分配内存
    html += FPSTR(HTML_HEAD);
    html += buffer;
    html += FPSTR(HTML_FOOT);
    
    return html;
}

/**
 * @brief 生成成功页面HTML
 */
String WebConfigManager::generateSuccessPage() {
    String html;
    html.reserve(800);
    html += FPSTR(HTML_HEAD);
    html += getSuccessPage();
    html += FPSTR(HTML_FOOT);
    return html;
}

/**
 * @brief 生成错误页面HTML
 */
String WebConfigManager::generateErrorPage() {
    String html;
    html.reserve(600);
    html += FPSTR(HTML_HEAD);
    html += getErrorPage();
    html += FPSTR(HTML_FOOT);
    return html;
}

/**
 * @brief 生成退出页面HTML
 */
String WebConfigManager::generateExitPage() {
    String html;
    html.reserve(700);
    html += FPSTR(HTML_HEAD);
    html += getExitPage();
    html += FPSTR(HTML_FOOT);
    return html;
}

/**
 * @brief 退出配置模式
 * 停止Web服务器，重启系统以应用新配置
 */
void WebConfigManager::exitConfigMode() {
    LOG_INFO("Exiting configuration mode...");
    
    // 停止Web服务器，AP由main.cpp管理
    stopWebServer();
    
    LOG_INFO("System will restart in 3 seconds...");
    
    for (int i = 3; i > 0; i--) {
        LOG_INFO_F("%d...", i);
        delay(1000);
    }
    
    LOG_INFO("Restarting...");
    
    isConfigMode = false;
    ESP.restart();
}

/**
 * @brief 检查是否处于配置模式
 * @return true 如果处于配置模式，false 否则
 */
bool WebConfigManager::isInConfigMode() const {
    return isConfigMode;
}

/**
 * @brief 设置配置模式状态
 * @param enabled 是否启用配置模式
 */
void WebConfigManager::setConfigMode(bool enabled) {
    isConfigMode = enabled;
}