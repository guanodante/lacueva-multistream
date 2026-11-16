# 🎮 La Cueva MultiStream — OBS Studio Plugin

Plugin de alto rendimiento para **OBS Studio** (v28.0 a v31.0+) que permite transmitir en vivo simultáneamente a **TikTok, YouTube, Instagram, Facebook y X (Twitter)** con configuración automática y consumo ultrabajo de recursos.

---

## ⚡ ¿Por qué consume el mínimo de recursos?

En las soluciones tradicionales, cada plataforma adicional requiere un nuevo codificador de video en tu GPU o CPU, lo cual satura rápidamente tu máquina:

```
❌ Método Tradicional (Ineficiente):
OBS Canvas ──► Encoder 1 (NVENC) ──► YouTube
           ──► Encoder 2 (NVENC) ──► Facebook   (¡Límite de sesiones NVENC alcanzado!)
           ──► Encoder 3 (x264)  ──► TikTok     (¡CPU al 95%!)
           ──► Encoder 4 (x264)  ──► Instagram
           ──► Encoder 5 (x264)  ──► X

✅ La Cueva MultiStream (Ultra Optimizado):
OBS Canvas ──► [ 1 ÚNICO ENCODER COMPARTIDO ] ──┬──► Socket RTMP 1 ──► YouTube
                                                ├──► Socket RTMP 2 ──► Facebook
                                                ├──► Socket RTMP 3 ──► TikTok
                                                ├──► Socket RTMP 4 ──► Instagram
                                                └──► Socket RTMP 5 ──► X
```

Al reutilizar el mismo flujo de paquetes codificados (`encoder_packet`), tu tarjeta gráfica o procesador **solo codifica una vez**, consumiendo prácticamente lo mismo que si transmitieras a un solo destino. El único recurso adicional que se utiliza es el ancho de banda de tu conexión a internet.

---

## 🌐 Configuración Automática por Plataforma

El plugin precarga las URLs de servidor, protocolos y parámetros óptimos de transmisión:

| Plataforma | Servidor RTMP / RTMPS | Resolución | Bitrate Video | FPS | Orientación | RTMPS |
|:---:|:---:|:---:|:---:|:---:|:---:|:---:|
| 🔴 **YouTube Live** | `rtmp://a.rtmp.youtube.com/live2` *(Automático)* | 1080p | 4,500 kbps | 30/60 | 🖥️ Horizontal (16:9) | No |
| 🔵 **Facebook Live** | `rtmps://live-api-s.facebook.com:443/rtmp/` *(Automático)* | 1080p | 4,500 kbps | 30 | 🖥️ Horizontal (16:9) | **Obligatorio** |
| 🎵 **TikTok Live** | Ingesta RTMP de TikTok Live Studio | 1080×1920 | 4,000 kbps | 30 | 📱 Vertical (9:16) | No |
| 📸 **Instagram Live** | Instagram Live Producer (`instagram.com`) | 720×1280 | 3,500 kbps | 30 | 📱 Vertical (9:16) | **Obligatorio** |
| 🐦 **X (Twitter)** | X Media Studio / Producer | 1080p | 4,000 kbps | 30 | 🖥️ Horizontal (16:9) | No |

---

## 🔑 Dónde obtener la Clave de Transmisión (Stream Key)

1. **YouTube**: [YouTube Studio](https://studio.youtube.com) ➔ Crear ➔ Transmitir en vivo ➔ Copiar "Clave de transmisión".
2. **Facebook**: [Facebook Live Producer](https://facebook.com/live/producer) ➔ Usar clave de stream persistente.
3. **TikTok**: Abre *TikTok Live Studio* o el panel de creador en TikTok. Copia la URL de servidor y la clave temporal generada.
4. **Instagram**: Entra a [instagram.com](https://instagram.com) desde tu PC con cuenta profesional/creador ➔ Crear ➔ Video en vivo ➔ Activa "Live Producer" y copia URL y Stream Key.
5. **X (Twitter)**: Entra a [studio.x.com](https://studio.x.com) ➔ Productor ➔ Fuentes ➔ Crear fuente RTMP.

---

## 🛠️ Compilación e Instalación

### Requisitos Previos
- **Windows 10/11 (64-bit)**
- **CMake 3.16 o superior**
- **Visual Studio 2022 (MSVC v143)** con soporte C++17
- **Qt 6.5+** (Widgets y Core)
- Dependencias de **OBS Studio**

### Pasos de Compilación
```bash
git clone https://github.com/lacuevadelldolor/lacueva-multistream.git
cd lacueva-multistream

# Configurar con CMake
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release

# Compilar DLL
cmake --build build --config Release
```

### Instalación Manual
Copia los binarios generados a la carpeta de plugins de OBS Studio:
- `build/Release/lacueva-multistream.dll` ➔ `C:\Program Files\obs-studio\obs-plugins\64bit\`
- Carpeta `data/locale/` ➔ `C:\Program Files\obs-studio\data\obs-plugins\lacueva-multistream\locale\`

---

## 🎮 Interfaz Gráfica (Dock)

El plugin añade un panel anclable en OBS Studio denominado **"🎮 La Cueva MultiStream"**:
- **Tarjetas por plataforma**: Activa o desactiva destinos con una casilla.
- **Botón "▶ INICIAR TODAS"**: Arranca la transmisión en simultáneo en todas las plataformas habilitadas con un solo clic.
- **Botón "⏹ DETENER TODAS"**: Cierra limpiamente todos los sockets.
- **Monitor de Recursos en Vivo**: Muestra uso de CPU, RAM y velocidad de subida en tiempo real con alertas preventivas.
- **Botón "Probar"**: Realiza un handshake de verificación antes de salir en vivo.

---

## 📄 Licencia

GPLv2 — Conforme a las licencias de código abierto del proyecto OBS Studio.

Desarrollado con dedicación para **La Cueva del Dolor** 🎮🔥.
