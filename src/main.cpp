// Usar PlatformIO
#include <Arduino.h>

#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <time.h>
#include "secrets.h"

// Configuración de la pantalla OLED
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_MOSI   23
#define OLED_CLK    18
#define OLED_DC     16
#define OLED_CS     5
#define OLED_RESET  17

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT,
  OLED_MOSI, OLED_CLK, OLED_DC, OLED_RESET, OLED_CS);

// Configuración WiFi
const char* ssid = SECRET_WIFI_NAME;
const char* password = SECRET_WIFI_PASSWORD;

// Servidor web en puerto 80
WebServer server(80);

// Configuración de ubicaciones
// Bogotá
const float bogota_lat = 4.6097;
const float bogota_lon = -74.0817;

// El Banco, Magdalena
const float banco_lat = 9.0167;
const float banco_lon = -73.9833;

// Variables globales para ubicación personalizada
float custom_lat = 0.0;
float custom_lon = 0.0;
String custom_city = "Personalizada";

// Variables globales
float temperature = 0.0;
unsigned long lastUpdate = 0;
const unsigned long updateInterval = 600000; // 10 minutos en milisegundos

// Modo de ubicación: 0=Bogotá, 1=El Banco, 2=Personalizada
int locationMode = 0;

// Declaración de funciones
void getWeatherData();
void displayTemperature();
void updateDisplay();
void handleRoot();
void handleSetLocation();
void handleSetCustomLocation();
void handleGetStatus();

void setup() {
  Serial.begin(115200);
  
  // Inicializar la pantalla OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC)) {
    Serial.println(F("Error al inicializar SSD1306"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Iniciando...");
  display.display();
  
  // Conectar a WiFi
  WiFi.begin(ssid, password);
  display.println("Conectando WiFi...");
  display.display();
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Conectando a WiFi...");
  }
  
  Serial.println("WiFi conectado!");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("Puedes acceder desde: http://");
  Serial.println(WiFi.localIP());
  
  display.println("WiFi conectado!");
  display.print("IP: ");
  display.println(WiFi.localIP());
  display.display();
  delay(5000);
  
  // Configurar tiempo (NTP)
  configTime(-5 * 3600, 0, "pool.ntp.org", "time.nist.gov");
  
  // Configurar rutas del servidor web
  server.on("/", handleRoot);
  server.on("/setlocation", handleSetLocation);
  server.on("/setcustom", handleSetCustomLocation);
  server.on("/status", handleGetStatus);
  
  // Iniciar servidor web
  server.begin();
  Serial.println("Servidor web iniciado en puerto 80");
  Serial.print("Accede desde tu telefono: http://");
  Serial.println(WiFi.localIP());
  
  // Obtener datos iniciales
  Serial.println("Obteniendo datos iniciales...");
  getWeatherData();
  updateDisplay();
}

void loop() {
  // Manejar clientes web
  server.handleClient();
  
  // Debug: mostrar cuando alguien se conecta
  static unsigned long lastDebug = 0;
  if (millis() - lastDebug > 30000) { // cada 30 segundos
    Serial.print("Servidor activo en: http://");
    Serial.println(WiFi.localIP());
    Serial.print("Ubicacion actual: ");
    if (locationMode == 0) Serial.println("Bogota");
    else if (locationMode == 1) Serial.println("El Banco");
    else Serial.println(custom_city);
    Serial.print("Temperatura actual: ");
    Serial.print(temperature);
    Serial.println("°C");
    lastDebug = millis();
  }
  
  // Actualizar datos meteorológicos cada 10 minutos
  if (millis() - lastUpdate >= updateInterval) {
    Serial.println("Actualizacion automatica de datos...");
    getWeatherData();
    updateDisplay();
    lastUpdate = millis();
  }
  
  delay(100);
}

void getWeatherData() {
  Serial.println("=== Obteniendo datos meteorologicos ===");
  
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    
    // Seleccionar coordenadas según la ubicación
    float lat, lon;
    String cityName;
    if (locationMode == 0) {
      lat = bogota_lat;
      lon = bogota_lon;
      cityName = "Bogota";
    } else if (locationMode == 1) {
      lat = banco_lat;
      lon = banco_lon;
      cityName = "El Banco";
    } else {
      lat = custom_lat;
      lon = custom_lon;
      cityName = custom_city;
    }
    
    Serial.print("Ubicacion seleccionada: ");
    Serial.println(cityName);
    Serial.print("Coordenadas: ");
    Serial.print(lat, 6);
    Serial.print(", ");
    Serial.println(lon, 6);
    
    // Construir URL de la API solo con temperatura
    String url = "https://api.open-meteo.com/v1/forecast?latitude=" + 
                 String(lat, 6) + "&longitude=" + String(lon, 6) + 
                 "&hourly=temperature_2m";
    
    Serial.print("URL API: ");
    Serial.println(url);
    
    http.begin(url);
    int httpResponseCode = http.GET();
    
    Serial.print("Codigo de respuesta HTTP: ");
    Serial.println(httpResponseCode);
    
    if (httpResponseCode > 0) {
      String response = http.getString();
      Serial.print("Respuesta recibida (primeros 200 chars): ");
      Serial.println(response.substring(0, 200));
      
      // Parsear JSON
      DynamicJsonDocument doc(8192);
      deserializeJson(doc, response);
      
      // Obtener hora actual + 5 (ajuste de zona horaria)
      struct tm timeinfo;
      if (getLocalTime(&timeinfo)) {
        int currentHour = timeinfo.tm_hour + 5;
        if (currentHour >= 24) currentHour -= 24;
        
        Serial.print("Hora actual (ajustada): ");
        Serial.println(currentHour);
        
        // Extraer temperatura del array
        JsonArray tempArray = doc["hourly"]["temperature_2m"];
        
        if (currentHour < tempArray.size()) {
          temperature = tempArray[currentHour];
          Serial.print("Temperatura obtenida para ");
          Serial.print(cityName);
          Serial.print(": ");
          Serial.print(temperature);
          Serial.println("°C");
        } else {
          Serial.print("Error: hora fuera de rango. Array size: ");
          Serial.println(tempArray.size());
        }
      } else {
        Serial.println("Error obteniendo tiempo local");
      }
    } else {
      Serial.print("Error en HTTP: ");
      Serial.println(httpResponseCode);
    }
    
    http.end();
  } else {
    Serial.println("WiFi desconectado");
  }
  
  Serial.println("=== Fin obtencion datos ===");
}

void updateDisplay() {
  displayTemperature();
}

void displayTemperature() {
  display.clearDisplay();
  
  // Título con ubicación
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  if (locationMode == 0) {
    display.println("Temperatura Bogota");
  } else if (locationMode == 1) {
    display.println("Temperatura El Banco");
  } else {
    display.print("Temp ");
    display.println(custom_city.substring(0, 10));
  }
  
  // Línea separadora
  display.drawLine(0, 12, SCREEN_WIDTH-1, 12, SSD1306_WHITE);
  
  // Temperatura grande
  display.setTextSize(3);
  display.setCursor(10, 25);
  display.print(temperature, 1);
  
  // Símbolo de grados (antes de la C)
  display.drawCircle(85, 27, 3, SSD1306_WHITE);
  
  // Letra C después del símbolo de grados
  display.setCursor(95, 25);
  display.print("C");
  
  // Información adicional
  display.setTextSize(1);
  display.setCursor(0, 55);
  if (locationMode == 0) {
    display.print("Bogota - ");
  } else if (locationMode == 1) {
    display.print("Magdalena - ");
  } else {
    display.print(custom_city.substring(0, 8));
    display.print(" - ");
  }
  
  // Mostrar tiempo de actualización
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    display.printf("%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min);
  }
  
  display.display();
}

// Página web principal
void handleRoot() {
  Serial.println(">>> Cliente conectado a la pagina principal <<<");
  
  String html = "<!DOCTYPE html>";
  html += "<html><head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1'>";
  html += "<title>Control ESP32 Weather</title>";
  html += "<style>";
  html += "body { font-family: Arial; margin: 20px; background: #f0f0f0; }";
  html += ".container { max-width: 400px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 2px 10px rgba(0,0,0,0.1); }";
  html += ".button { width: 100%; padding: 15px; margin: 10px 0; font-size: 18px; border: none; border-radius: 5px; cursor: pointer; }";
  html += ".temp-btn { background: #ff6b6b; color: white; }";
  html += ".humidity-btn { background: #4ecdc4; color: white; }";
  html += ".capital-btn { background: #17a2b8; color: white; }";
  html += ".status { background: #f8f9fa; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".current { background: #28a745; color: white; }";
  html += "select { width: 100%; padding: 15px; margin: 10px 0; font-size: 16px; border: 1px solid #ccc; border-radius: 5px; }";
  html += "</style></head><body>";
  html += "<div class='container'>";
  html += "<h1>Termometro Mundial</h1>";
  html += "<div class='status'>";
  html += "<strong>Estado Actual:</strong><br>";
  html += "Temperatura: " + String(temperature, 1) + "C<br>";
  String currentLocation;
  if (locationMode == 0) currentLocation = "Bogota";
  else if (locationMode == 1) currentLocation = "El Banco, Magdalena";
  else currentLocation = custom_city;
  html += "Ubicacion: " + currentLocation;
  html += "</div>";
  
  html += "<h3>Ubicaciones Fijas:</h3>";
  html += "<button class='button temp-btn" + String(locationMode == 0 ? " current" : "") + "' onclick='setLocation(0)'>Bogota</button>";
  html += "<button class='button humidity-btn" + String(locationMode == 1 ? " current" : "") + "' onclick='setLocation(1)'>El Banco</button>";
  
  html += "<h3>Capitales del Mundo:</h3>";
  html += "<select id='capitalSelect'>";
  html += "<option value=''>Selecciona una capital...</option>";
  html += "<option value='Madrid,40.4168,-3.7038'>Madrid, Espana</option>";
  html += "<option value='Paris,48.8566,2.3522'>Paris, Francia</option>";
  html += "<option value='Londres,51.5074,-0.1278'>Londres, Reino Unido</option>";
  html += "<option value='Roma,41.9028,12.4964'>Roma, Italia</option>";
  html += "<option value='Berlin,52.5200,13.4050'>Berlin, Alemania</option>";
  html += "<option value='Tokyo,35.6762,139.6503'>Tokyo, Japon</option>";
  html += "<option value='Beijing,39.9042,116.4074'>Beijing, China</option>";
  html += "<option value='Nueva Delhi,28.7041,77.1025'>Nueva Delhi, India</option>";
  html += "<option value='Washington,38.9072,-77.0369'>Washington DC, EE.UU.</option>";
  html += "<option value='Ottawa,45.4215,-75.6972'>Ottawa, Canada</option>";
  html += "<option value='Ciudad de Mexico,19.4326,-99.1332'>Ciudad de Mexico</option>";
  html += "<option value='Buenos Aires,-34.6037,-58.3816'>Buenos Aires, Argentina</option>";
  html += "<option value='Lima,-12.0464,-77.0428'>Lima, Peru</option>";
  html += "<option value='Santiago,-33.4489,-70.6693'>Santiago, Chile</option>";
  html += "<option value='Caracas,10.4806,-66.9036'>Caracas, Venezuela</option>";
  html += "<option value='Brasilia,-15.8267,-47.9218'>Brasilia, Brasil</option>";
  html += "<option value='Bangkok,13.7563,100.5018'>Bangkok, Tailandia</option>";
  html += "<option value='Singapur,1.3521,103.8198'>Singapur</option>";
  html += "<option value='Sidney,-33.8688,151.2093'>Sidney, Australia</option>";
  html += "</select>";
  html += "<button class='button capital-btn' onclick='setCapital()'>Aplicar Capital</button>";
  
  html += "<button class='button' style='background: #6c757d; color: white; margin-top: 20px;' onclick='updateData()'>Actualizar Datos</button>";
  html += "</div>";
  html += "<script>";
  html += "function setLocation(loc) {";
  html += "  console.log('Cambiando a ubicacion fija: ' + loc);";
  html += "  fetch('/setlocation?location=' + loc)";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      console.log('Respuesta del servidor: ' + data);";
  html += "      setTimeout(() => { location.reload(); }, 2000);";
  html += "    })";
  html += "    .catch(error => console.error('Error:', error));";
  html += "}";
  html += "function setCapital() {";
  html += "  var select = document.getElementById('capitalSelect');";
  html += "  var value = select.value;";
  html += "  if (value === '') {";
  html += "    alert('Por favor selecciona una capital');";
  html += "    return;";
  html += "  }";
  html += "  var parts = value.split(',');";
  html += "  var city = parts[0];";
  html += "  var lat = parts[1];";
  html += "  var lon = parts[2];";
  html += "  console.log('Cambiando a capital: ' + city);";
  html += "  fetch('/setcustom?city=' + encodeURIComponent(city) + '&lat=' + lat + '&lon=' + lon)";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      console.log('Respuesta del servidor: ' + data);";
  html += "      setTimeout(() => { location.reload(); }, 2000);";
  html += "    })";
  html += "    .catch(error => console.error('Error:', error));";
  html += "}";
  html += "function updateData() {";
  html += "  console.log('Actualizando datos...');";
  html += "  location.reload();";
  html += "}";
  html += "</script></body></html>";
  
  server.send(200, "text/html; charset=UTF-8", html);
}

// Cambiar ubicación fija (Bogotá o El Banco)
void handleSetLocation() {
  Serial.println(">>> Solicitud de cambio de ubicacion fija <<<");
  
  if (server.hasArg("location")) {
    int newLocation = server.arg("location").toInt();
    Serial.print("Nueva ubicacion solicitada: ");
    Serial.println(newLocation);
    
    if (newLocation >= 0 && newLocation <= 1) {
      locationMode = newLocation;
      Serial.println("Obteniendo datos de la nueva ubicacion...");
      getWeatherData();
      updateDisplay();
      Serial.println(">>> Ubicacion cambiada exitosamente a: " + String(locationMode == 0 ? "Bogota" : "El Banco") + " <<<");
    } else {
      Serial.println("Error: ubicacion invalida");
    }
  } else {
    Serial.println("Error: parametro location no encontrado");
  }
  server.send(200, "text/plain", "OK");
}

// Cambiar a ubicación personalizada (capital del mundo)
void handleSetCustomLocation() {
  Serial.println(">>> Solicitud de cambio a capital personalizada <<<");
  
  if (server.hasArg("city") && server.hasArg("lat") && server.hasArg("lon")) {
    custom_city = server.arg("city");
    custom_lat = server.arg("lat").toFloat();
    custom_lon = server.arg("lon").toFloat();
    locationMode = 2; // Modo personalizado
    
    Serial.print("Ciudad: ");
    Serial.println(custom_city);
    Serial.print("Coordenadas: ");
    Serial.print(custom_lat, 6);
    Serial.print(", ");
    Serial.println(custom_lon, 6);
    
    Serial.println("Obteniendo datos de la nueva capital...");
    getWeatherData();
    updateDisplay();
    Serial.println(">>> Capital cambiada exitosamente a: " + custom_city + " <<<");
  } else {
    Serial.println("Error: faltan parametros para ubicacion personalizada");
  }
  server.send(200, "text/plain", "OK");
}

// Obtener estado actual
void handleGetStatus() {
  String currentLocation;
  if (locationMode == 0) currentLocation = "Bogota";
  else if (locationMode == 1) currentLocation = "El Banco";
  else currentLocation = custom_city;
  
  String json = "{";
  json += "\"temperature\":" + String(temperature, 1) + ",";
  json += "\"location\":\"" + currentLocation + "\",";
  json += "\"locationMode\":" + String(locationMode) + ",";
  json += "\"ip\":\"" + WiFi.localIP().toString() + "\"";
  json += "}";
  
  server.send(200, "application/json", json);
}