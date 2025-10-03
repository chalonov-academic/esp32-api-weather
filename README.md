# Termómetro Mundial ESP32

Termómetro inteligente con pantalla OLED que muestra la temperatura actual de múltiples ciudades alrededor del mundo, controlable desde cualquier dispositivo móvil.

## 📋 Características

- 🌍 **Múltiples ubicaciones**: Cambia entre Bogotá, El Banco (Magdalena) y más de 20 capitales mundiales
- 📱 **Control remoto**: Interfaz web responsive accesible desde cualquier navegador
- 🖥️ **Pantalla OLED**: Display SSD1306 de 128x64 píxeles
- 🌐 **API meteorológica**: Datos en tiempo real de Open-Meteo
- 🔄 **Actualización automática**: Datos actualizados cada 10 minutos
- ⚡ **WiFi integrado**: Conexión inalámbrica para acceso remoto

## 🛠️ Hardware Requerido

- ESP32 (cualquier modelo compatible)
- Pantalla OLED SSD1306 (128x64, SPI)
- Cables de conexión

### 📌 Pinout Pantalla OLED

```
OLED MOSI   → GPIO 23
OLED CLK    → GPIO 18
OLED DC     → GPIO 16
OLED CS     → GPIO 5
OLED RESET  → GPIO 17
```

## 📦 Dependencias

Agregar en `platformio.ini`:

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200

lib_deps = 
    adafruit/Adafruit SSD1306@^2.5.7
    adafruit/Adafruit GFX Library@^1.11.9
    bblanchon/ArduinoJson@^6.21.3
```

## 🚀 Instalación

### 1. Clonar el repositorio

```bash
git clone https://github.com/chalonov-academic/esp32-api-weather.git
cd esp32-api-weather
```

### 2. Configurar credenciales WiFi

Crear archivo `src/secrets.h`:

```cpp
#define SECRET_WIFI_NAME "TU_NOMBRE_WIFI"
#define SECRET_WIFI_PASSWORD "TU_PASSWORD_WIFI"
```

**Importante:** Agregar `secrets.h` a `.gitignore` para no subir credenciales.

### 3. Compilar y cargar

Con PlatformIO:

Con PlatformIO IDE (VSCode):
- Presiona el botón "Upload" (→)
- Abre el "Serial Monitor"

### 4. Obtener la IP

Al iniciar, la ESP32 mostrará su IP en:
- Monitor Serial
- Pantalla OLED (5 segundos)

Ejemplo: `192.168.1.100`

## 📱 Uso

### Acceder a la interfaz web

1. Conecta tu teléfono o computadora a la **misma red WiFi**
2. Abre el navegador
3. Ve a la IP de la ESP32: `http://192.168.1.100`

### Cambiar ubicaciones

**Ubicaciones fijas:**
- Botón **"Bogotá"**: Temperatura de Bogotá, Colombia
- Botón **"El Banco"**: Temperatura de El Banco, Magdalena

**Capitales del mundo:**
1. Selecciona una capital de la lista desplegable
2. Presiona **"Aplicar Capital"**
3. Espera 2 segundos mientras se actualiza

**Actualizar datos:**
- Botón **"Actualizar Datos"**: Recarga manualmente los datos meteorológicos

## 🌍 Ciudades Disponibles

### Ubicaciones fijas
- Bogotá, Colombia
- El Banco, Magdalena

### Capitales disponibles
- **Europa**: Madrid, París, Londres, Roma, Berlín
- **América**: Washington DC, Ottawa, Ciudad de México, Buenos Aires, Lima, Santiago, Caracas, Brasilia
- **Asia**: Tokyo, Beijing, Nueva Delhi, Bangkok, Singapur
- **Oceanía**: Sidney

## 🔧 API Endpoints

La ESP32 expone los siguientes endpoints:

- `GET /` - Interfaz web principal
- `GET /setlocation?location=X` - Cambia a ubicación fija (0=Bogotá, 1=El Banco)
- `GET /setcustom?city=X&lat=Y&lon=Z` - Cambia a ubicación personalizada
- `GET /status` - Devuelve JSON con estado actual

### Ejemplo de respuesta `/status`:

```json
{
  "temperature": 16.5,
  "location": "Bogota",
  "locationMode": 0,
  "ip": "192.168.1.100"
}
```

## 📊 Pantalla OLED

La pantalla muestra:
- **Línea 1**: Nombre de la ciudad actual
- **Centro**: Temperatura grande (ej: 16.5°C)
- **Línea inferior**: Ciudad y hora de última actualización

## 🐛 Solución de Problemas

### La ESP32 no se conecta al WiFi
- Verifica credenciales en `secrets.h`
- Asegúrate de usar WiFi 2.4GHz (ESP32 no soporta 5GHz)
- Revisa el monitor serial para ver mensajes de error

### No puedo acceder a la página web
- Verifica que estés en la misma red WiFi
- Confirma la IP en el monitor serial
- Intenta hacer ping a la IP desde tu computadora
- Desactiva temporalmente el firewall

### La pantalla no muestra nada
- Verifica las conexiones del pinout
- Asegúrate de usar el protocolo SPI (no I2C)
- Revisa que el modelo sea SSD1306 128x64

### La temperatura no se actualiza
- Verifica conexión a internet del router
- Revisa el monitor serial para ver errores HTTP
- La API puede tener límites de tasa

## 📝 Estructura del Proyecto

```
termometro-mundial-esp32/
├── src/
│   ├── main.cpp          # Código principal
│   └── secrets.h         # Credenciales WiFi (no incluir en git)
├── platformio.ini        # Configuración PlatformIO
├── .gitignore           
└── README.md
```

## 📄 Licencia

Este proyecto es de código abierto y está disponible bajo la Licencia MIT.

## 🙏 Agradecimientos

- [Open-Meteo](https://open-meteo.com/) - API meteorológica gratuita
- [Adafruit](https://www.adafruit.com/) - Librerías para pantallas OLED
- [PlatformIO](https://platformio.org/) - Plataforma de desarrollo

---
