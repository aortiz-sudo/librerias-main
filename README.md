# Sistema de Despacho de Combustible

Firmware del sistema de control y despacho de combustible basado en **ESP32** (controlador principal) y dos microcontroladores **ATmega328P/PB** (controladores de pantalla e impresora). El proyecto gestiona el despacho de combustible, la autenticación de usuarios, la impresión de tickets, el registro de eventos y la comunicación con un servidor remoto mediante HTTP y Socket.IO, con soporte de actualización de firmware por aire (OTA).

> Proyecto desarrollado con [PlatformIO](https://platformio.org/) sobre el framework Arduino.

---

## Tabla de contenido

- [Arquitectura](#arquitectura)
- [Características](#características)
- [Periféricos](#periféricos)
- [Estructura del proyecto](#estructura-del-proyecto)
- [Requisitos](#requisitos)
- [Compilación y carga](#compilación-y-carga)
- [Configuración (tarjeta SD)](#configuración-tarjeta-sd)
- [Comunicación I2C entre microcontroladores](#comunicación-i2c-entre-microcontroladores)
- [Actualización de firmware (OTA)](#actualización-de-firmware-ota)
- [Documentación (Doxygen)](#documentación-doxygen)
- [Dependencias](#dependencias)

---

## Arquitectura

El sistema se compone de **tres microcontroladores** que se comunican por un bus **I2C**, donde el ESP32 actúa como maestro y los dos ATmega como esclavos:

``` 
                          ┌─────────────────────────┐
                          │     Servidor remoto     │
                          │    (HTTP + Socket.IO)   │
                          └────────────┬────────────┘
                                       │ Ethernet (W5500) + TLS
                                       │
              ┌────────────────────────┴────────────────────────┐
              │                  ESP32 (maestro)                │
              │    Red, lógica de pantalla, RFID, sensor LLS,   │
              │    satelital, tarjeta SD, RTC, teclado, OTA     │
              └────────┬──────────────────────────────┬─────────┘
                       │ I2C (0x31)                   │ I2C (0x30)
              ┌────────┴─────────┐          ┌─────────┴─────────┐
              │ ATmega «Display» │          │ ATmega «Printer»  │
              │  Pantalla DWIN   │          │ Impresora térmica │
              │    por UART      │          │   + satélital.    │
              └──────────────────┘          └───────────────────┘
```

Los tres firmwares **comparten un único directorio `src/`**. Cada objetivo de compilación selecciona los archivos que le corresponden mediante `build_src_filter` en `platformio.ini`, y cada uno tiene su propio punto de entrada:

| Microcontrolador | Punto de entrada      | Rol                                                                                   |
|------------------|-----------------------|---------------------------------------------------------------------------------------|
| ESP32            | `src/main.cpp`        | Controlador principal: red, máquina de estados, periféricos, OTA.                     |
| ATmega «Display» | `src/mainDisplay.cpp` | Esclavo I2C (`0x31`); traduce comandos a la pantalla DWIN por UART.                   |
| ATmega «Printer» | `src/mainPrinter.cpp` | Esclavo I2C (`0x30`); controla la impresora térmica y mide batería/estado satelital.  |

---

## Características

- **Despacho de combustible** con caudalímetro por interrupciones y control de relevadores.
- **Autenticación de usuarios** mediante contraseña y/o tag RFID.
- **Máquina de estados** para la interfaz de la pantalla táctil (menús, calibración, configuración, despacho, reimpresión).
- **Impresión de tickets** (último ticket, ticket del día) en impresora térmica.
- **Conectividad** vía Ethernet (W5500) con TLS, enviando eventos y actualizaciones por HTTP y Socket.IO.
- **Registro y respaldo** de eventos, despachos y parámetros en tarjeta SD, con reenvío de eventos pendientes cuando se recupera la conexión.
- **Actualización de firmware OTA** de los tres microcontroladores desde un repositorio de GitHub.
- **Diseño orientado a objetos** con patrón *Builder* para abstraer dispositivos UART e I2C.

---

## Periféricos

| Periférico                      | Interfaz            | Controlador        | Módulo                         |
|---------------------------------|---------------------|--------------------|--------------------------------|
| Pantalla táctil DWIN            | UART                | ATmega «Display»   | `Display`                      |
| Impresora térmica               | I2C → UART          | ATmega «Printer»   | `Printer` / `Peripehals`       |
| Lector RFID MT124               | UART                | ESP32              | `MT124`                        |
| Sensor de nivel de combustible  | UART (LLS)          | ESP32              | `LLS`                          |
| Módem satelital Smartone        | UART                | ESP32              | `Smartone` / `Satellite`       |
| Módem satelital Iridium         | UART                | ESP32              | `Iridium` / `Satellite`        |
| Teclado matricial               | I2C (PCF8574)       | ESP32              | `Keypad_I2C`                   |
| RTC DS3231                      | I2C                 | ESP32              | `DS3231` (librería externa)    |
| Tarjeta SD                      | SPI                 | ESP32              | `SD_Card`                      |
| Ethernet W5500                  | SPI                 | ESP32              | `Ethernet_*` / `Global_Client` |
| Módulo Bluetooth HC-05          | UART                | (opcional)         | `HC05`                         |

---

## Estructura del proyecto

```
.
├── include/                # Cabeceras (.h) con la documentación Doxygen de la API
│   ├── Device.h            # Interfaz base Device + patrón Builder
│   ├── UART_Device.h       # Clase base para dispositivos UART
│   ├── I2C_Master_Device.h # Clase base para dispositivos I2C maestros
│   ├── Display.h           # Driver de pantalla DWIN
│   ├── Printer.h           # Impresora térmica
│   ├── LLS.h               # Sensor de nivel de combustible
│   ├── MT124.h             # Lector RFID
│   ├── Satellite.h / Smartone.h / Iridium.h   # Comunicación satelital
│   ├── Screen.h / State.h  # Máquina de estados de la interfaz
│   ├── Global_Client.h / Ethernet_HTTP.h / Ethernet_WebSocket.h / Ethernet_SocketIO.h
│   ├── json_parser.h / string_handlers.h      # Utilidades
│   ├── SD_Card.h / Logger.h / Definitions.h
│   └── main.h / mainDisplay.h / mainPrinter.h # Cabeceras de cada firmware
├── src/                    # Implementaciones (.cpp) compartidas por los tres firmwares
├── lib/                    # Librerías incluidas en el repositorio (ESP_SSLClient, jsmn)
├── test/                   # Pruebas
├── docs/                   # Documentación HTML generada por Doxygen (no versionada)
├── bootloader/             # Bootloader I2C de los ATmega328PB (proyecto de PlatformIO aparte, se graba por ISP)
├── firmware_repository/    # Clone de aortiz-sudo/firmware_repository con los binarios OTA (ignorado por git)
├── scripts/                # avr_bin.py (display.bin/printer.bin), esp32_otadata.py (boot_app0 en otadata) y publish_firmware.py (publica la OTA)
├── partitions.csv          # Esquema de particiones de la ESP32 (con OTA y coredump)
├── platformio.ini          # Configuración de entornos de compilación
├── Doxyfile                # Configuración de Doxygen
└── versions.txt            # Versiones de firmware (esp32;display;printer)
```

---

## Requisitos

- [PlatformIO Core](https://platformio.org/install) o la extensión de PlatformIO para VS Code.
- Toolchains gestionados automáticamente por PlatformIO:
  - **espressif32** para el ESP32.
  - **atmelavr** para los ATmega328P/PB.
- Las dependencias se descargan solas durante la primera compilación (ver [Dependencias](#dependencias)).

---

## Compilación y carga

El proyecto define varios entornos en `platformio.ini`. Cada microcontrolador tiene su variante de *debug* y *release*:

| Entorno            | Objetivo                          | Notas                                   |
|--------------------|-----------------------------------|-----------------------------------------|
| `esp32dev`         | ESP32 (desarrollo)                | Incluye `-D debug_mode`.                |
| `esp32_debug`      | ESP32 (depuración)                |                                         |
| `esp32_release`    | ESP32 (producción)                | Sin `debug_mode`.                       |
| `display_debug`    | ATmega328P – pantalla             | `f_cpu = 8 MHz`, `TWI_BUFFER_SIZE=64`.  |
| `display_release`  | ATmega328PB – pantalla            |                                         |
| `printer_debug`    | ATmega328P – impresora            | `f_cpu = 8 MHz`.                        |
| `printer_release`  | ATmega328PB – impresora           |                                         |

### Comandos

Compilar un entorno:

```bash
pio run -e esp32_release
pio run -e display_release
pio run -e printer_release
```

Compilar y cargar (ajusta el puerto en `upload_port` o con `--upload-port`):

```bash
pio run -e esp32dev -t upload
pio run -e display_release -t upload --upload-port COM8
```

Monitor serie:

```bash
pio device monitor
```

> **Nota sobre C++17:** los entornos ESP32 fuerzan el estándar `gnu++17` (`build_unflags`/`build_flags`) porque el código usa características como `if constexpr` en el parser de JSON.

---

## Configuración (tarjeta SD)

El ESP32 lee su configuración desde la tarjeta SD al arrancar. Los archivos principales (definidos en `include/SD_Card.h`) son:

| Archivo                 | Contenido                                                                      |
|-------------------------|--------------------------------------------------------------------------------|
| `/Parametros.txt`       | Parámetros generales (servidor, negocio, precio, etc.).                        |
| `/usuarios.txt`         | Usuarios registrados.                                                          |
| `/Eventos.txt`          | Eventos pendientes de envío al servidor.                                       |
| `/Despachos.txt`        | Historial de despachos para reimpresión de tickets.                            |
| `/github.txt`           | Línea 1: token de GitHub. Líneas 2-4: versiones instaladas (ESP32, display, impresora). Ver [OTA](#actualización-de-firmware-ota). |
| `/api-key.txt`          | Clave de API.                                                                  |
| `/ca-cert.pem`          | Certificado raíz (CA) para las conexiones TLS.                                 |
| `/backups/`             | Respaldos de los archivos anteriores.                                          |
| `/firmwares/`           | Binarios descargados para OTA (`firmware.bin`, `display.bin`, `printer.bin`).  |
| `/coredump.elf`         | Volcado de memoria tras un reinicio inesperado.                                |

> La configuración de red (IP estática, gateway, DNS, puerto) y las direcciones I2C/pines se definen en `include/main.h` y en `config_ethernet()` dentro de `src/main.cpp`.

---

## Comunicación I2C entre microcontroladores

El ESP32 envía comandos a los esclavos mediante el bus I2C. Los códigos de comando y las direcciones de memoria están definidos en `include/Definitions.h`. Direcciones de los esclavos (en `include/main.h`):

| Dispositivo            | Dirección de operación | Dirección del bootloader |
|------------------------|------------------------|--------------------------|
| Pantalla (Display)     | `0x31`                 | `0x41`                   |
| Impresora (Printer)    | `0x30`                 | `0x40`                   |
| Teclado                | `0x38`                 | —                        |

Ejemplos de comandos I2C: imprimir ticket, cambiar imagen de la pantalla, ajustar brillo, reiniciar periféricos por watchdog, etc.

---

## Actualización de firmware (OTA)

El sistema puede actualizar los tres firmwares de forma remota:

1. Todos los días, la primera vez que hay red durante la hora 04 del RTC, el ESP32 lee `versions.txt` de la rama `main` del repo privado [`aortiz-sudo/firmware_repository`](https://github.com/aortiz-sudo/firmware_repository). El formato es `esp32;display;impresora`, sin salto de línea final, y se compara con las versiones instaladas en `/github.txt`.
2. Si una versión cambió, descarga ese binario desde el **tag del componente** a `/firmwares/download.tmp`. Solo si llegó completo lo mueve a su ruta final en `/firmwares/`.
3. Con la pantalla en reposo, `check_firmware_triggers()` lo aplica: primero el display, luego la impresora y al final el ESP32, porque reinicia. La versión se guarda en `/github.txt` **después** de aplicarla; si falla, se reintenta en la siguiente revisión.
4. **ESP32:** se actualiza a sí mismo con el esquema de particiones OTA (`partitions.csv`, `app0`/`app1`). Al arrancar escribe en el log la partición en uso.
5. **ATmega (pantalla e impresora):** el ESP32 los reprograma por I2C a través de su bootloader (ver [Bootloader de los ATmega](#bootloader-de-los-atmega)). Envía el firmware página por página y lo verifica con CRC; se hacen hasta 3 intentos.

La plataforma también puede pedir una versión del ESP32 con el evento WebSocket `actualizacionFirmware` y el payload `{"version":"1.1.0"}` (sin prefijo). Se aplica con la pantalla en reposo.

### Bootloader de los ATmega

El bootloader I2C de los dos ATmega328PB está en [bootloader/](bootloader/), un proyecto de PlatformIO aparte (4 KB en `0x7000`; display en `0x41`, impresora en `0x40`). Las apps entran a él con el comando `0x00` (reinicio por watchdog).

| Comando | Trama | Respuesta (lectura de 1 byte) |
|---|---|---|
| Iniciar | `0x3E 0x02 TAM_MSB TAM_LSB` | Borra la página 0: desde aquí la app queda inválida |
| Página | `0x3C 0x80 0x00` + 128 datos (131 bytes) | `0` = OK |
| CRC | `0x3D CRC_MSB CRC_LSB` | `0` = OK; la página 0 se escribe solo si coincide |
| Arrancar | `0x3F` | Arranca la app |

Códigos de estado: `1` tamaño inválido, `2` dirección inválida, `3` fallo de escritura, `4` CRC distinto, `5` timeout, `6` desborde de buffer y `7` sin app (en reposo).

- **Con app válida**, después de cada reinicio el bootloader espera **1 s** a que el ESP32 empiece a flashear (`burn_firmware()` lo busca 20 ms después de mandar el reinicio) y luego arranca la app. La arranca con un reinicio por watchdog y una marca en RAM (`.noinit`), no con un salto directo: así la app empieza con todos los periféricos en su estado de reset. Con el salto directo, la app del display no arrancaba.
- **Sin app** (flasheo cortado, o recién grabado por ISP), el bootloader no la arranca y espera al ESP32. Con la pantalla en reposo, el ESP32 revisa cada 60 s (hasta 3 veces por arranque) si algún ATmega responde `7`. Si es así, lo flashea con el binario de `/firmwares/`.
- Si el ESP32 deja de mandar a mitad del flasheo, a los 10 s el bootloader se reinicia y espera sin app.
- La app del display se reinicia sola por su watchdog si pasa ~1 s sin transmitir a la pantalla. Cada uno de esos reinicios la deja ~1 s sin responder (la ventana del bootloader).

**Grabarlo por ISP (USBasp), una sola vez por ATmega:**

```bash
pio run -d bootloader -e bootloader_display -t fuses     # o bootloader_printer
pio run -d bootloader -e bootloader_display -t upload
```

Fusibles: `lfuse 0xE2`, `hfuse 0xD0` (BOOTRST activo, sección de arranque de 4 KB), `efuse 0xF7`. Las impresoras salieron de fábrica con `hfuse 0xD7`, que arranca directo en la app y nunca entra al bootloader; hay que cambiarlo a `0xD0`.

> ⚠️ Grabar por ISP borra la flash completa, incluida la app; también cargar la app por USBasp borra el bootloader. Lo más práctico es grabar en una sola pasada la app y el bootloader juntos (un `.hex` con los dos, con `avrdude -e`), y verificar leyendo la flash. Si solo se graba el bootloader, la app se recupera desde el ESP32: copia el binario a `/firmwares/display.bin` (o `printer.bin`) en la SD.

Los equipos con el bootloader anterior, o sin bootloader, no se pueden flashear por I2C. El intento falla sin daño: la app se reinicia sola en ~1 s y se reintenta en la siguiente revisión.

### Carga por USB después de una OTA

`scripts/esp32_otadata.py` hace que `pio run -t upload` escriba `boot_app0.bin` en la dirección de `otadata` de `partitions.csv` (`0x9000`); el framework lo mandaba a `0xe000`. Así, un equipo que una OTA dejó arrancando en `app1` arranca el firmware recién cargado en `app0`.

### Tags y rutas en `firmware_repository`

Cada componente tiene su propio número de versión. El binario de la versión `X` está en el tag `<componente>-X`:

| Componente | Tag           | Ruta en el repo                                                   |
|------------|---------------|-------------------------------------------------------------------|
| ESP32      | `esp32-X`     | `.pio/build/esp32_release/firmware.bin` (pantalla con RTC)        |
|            |               | `.pio/build/esp32_release_legacy/firmware.bin` (pantalla anterior) |
| Display    | `display-X`   | `.pio/build/display_release/display.bin`                          |
| Impresora  | `printer-X`   | `.pio/build/printer_release/printer.bin`                          |

Cada build del ESP32 descarga la ruta de su propia variante (`DISPLAY_HAS_INTERNAL_RTC`): un equipo con pantalla anterior nunca recibe el firmware de la pantalla nueva. Las descargas van a `api.github.com` por HTTPS y validan el certificado con las raíces de `include/github_ca.h` y la hora del DS3231. Si el RTC no tiene una hora válida, la descarga no se intenta.

### Token de GitHub

El repo es privado, así que cada equipo necesita un token en la línea 1 de `/github.txt` (y de `/backups/github.txt`). Usa un token **fine-grained** con estos datos: dueño `aortiz-sudo`, acceso *Only select repositories* → `firmware_repository` y permiso *Contents: Read-only*. El firmware acepta hasta 126 caracteres; uno fine-grained mide 93. El token queda en texto plano en la SD; con estos permisos, si se filtra solo da lectura a los binarios. Anota la fecha de vencimiento: cuando vence, la OTA deja de funcionar y cambiarlo requiere editar la SD.

### Publicar una versión

1. Haz commit de los cambios del firmware en este repo. El script se niega a publicar si hay cambios sin commitear, porque el binario no correspondería a ningún commit.
2. Clona el repo de binarios dentro del proyecto (está en `.gitignore`):
   ```bash
   git clone https://github.com/aortiz-sudo/firmware_repository.git firmware_repository
   ```
3. Publica solo los componentes que cambiaron:
   ```bash
   python scripts/publish_firmware.py publish --esp32 1.1.1 --push
   python scripts/publish_firmware.py publish --esp32 1.2.0 --display 1.2.0 --printer 1.2.0 --push
   ```
   El script hace un build limpio de los entornos `*_release` (incluida la variante legacy del ESP32) y valida los binarios. Después los copia, reescribe `versions.txt`, hace un commit con el `git describe` de la fuente y los SHA-256, crea los tags anotados y sube todo con un push atómico. Sin `--push`, todo queda local.
4. Verifica lo publicado con el mismo token que usan los equipos:
   ```bash
   GITHUB_TOKEN=<token> python scripts/publish_firmware.py verify
   ```
   Descarga cada binario con las mismas URLs que arma el firmware y compara el HTTP 200, `Content-Length` y SHA-256 con el tag.

Los `.bin` de los ATmega los genera `scripts/avr_bin.py` al compilar (`avr-objcopy -O binary`). Es la imagen cruda desde la dirección 0 que espera el bootloader I2C.

---

## Documentación (Doxygen)

La API está documentada con comentarios Doxygen en las cabeceras de `include/`. Para generar la documentación HTML:

```bash
doxygen Doxyfile
```

El resultado se genera en `docs/html/index.html`. Requiere [Doxygen](https://www.doxygen.nl/); para los diagramas de clases es recomendable instalar también [Graphviz](https://graphviz.org/).

> El `Doxyfile` ya está configurado para documentar `include/` y `src/`, excluir las librerías de terceros (`.pio`) e incluir las funciones `static` de los archivos `main`.

---

## Dependencias

Gestionadas por PlatformIO (`lib_deps` en `platformio.ini`):

| Librería                              | Uso                                    |
|---------------------------------------|----------------------------------------|
| `chris--a/Keypad`                     | Lectura del teclado matricial.         |
| `northernwidget/DS3231`               | Reloj de tiempo real (RTC).            |
| `mobizt/ESP_SSLClient`                | Cliente TLS para conexiones seguras.   |
| `arduino-libraries/Ethernet`          | Controlador del módulo Ethernet W5500. |

Incluidas en el repositorio (`lib/`):

- **ESP_SSLClient** y **jsmn** (parser de JSON).
