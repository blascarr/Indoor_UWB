# Tests Indoor_UWB

Tests [Unity](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html) para validar la lógica del proyecto sin depender del firmware completo en `src/`.

## Suites

| Carpeta | Qué valida |
|---------|------------|
| `test_trilateration` | `norm3`, `trilateration3D`, coherencia de distancias |
| `test_anchor_list` | `AnchorList` (alta/baja/búsqueda/capacidad), POD `EspNowPosition` |
| `test_board_pinout` | Pines DW1000 según placa (`INDOOR_UWB_BOARD_*`) |
| `test_range_filter` | `RangeFilterManager` (dead zone, gate, multi-anchor) |
| `test_real_distance` | Captura UWB en hardware real (Serial / LittleFS / ambos) |
| `test_wifi` | Integración STA + IP estática (requiere AP real) |

## Ejecutar

Solo compilar (sin placa / sin subir firmware):

```bash
pio test -e test_uwb_unit --without-uploading --without-testing
```

Tests unitarios en hardware (ESP32 por USB):

```bash
pio test -e test_uwb_unit -f test_trilateration
pio test -e test_uwb_unit -f test_anchor_list
pio test -e test_uwb_unit -f test_board_pinout
pio test -e test_uwb_unit -f test_range_filter
pio test -e test_uwb_unit
```

Test de distancia real (tag + anchor encendidos, posición fija conocida):

```bash
# Solo Serial → CSV en test/captures/ (script host)
CAPTURE_OUTPUT=serial SERIAL_PORT=/dev/ttyUSB0 \
  RANGING_TEST_NAME=fijo_50cm RANGING_REF_M=0.5 RANGING_SAMPLES=500 \
  ./test/scripts/run_real_distance_test.sh

# Solo LittleFS en el ESP32 (/captures/...)
CAPTURE_OUTPUT=littlefs RANGING_TEST_NAME=fijo_50cm pio test -e test_uwb_real_distance -f test_real_distance

# Híbrido (por defecto): Serial + LittleFS
CAPTURE_OUTPUT=both SERIAL_PORT=/dev/ttyUSB0 \
  RANGING_TEST_NAME=fijo_50cm ./test/scripts/run_real_distance_test.sh
```

Variables de entorno:

| Variable | Default | Descripción |
|----------|---------|-------------|
| `CAPTURE_OUTPUT` | `both` | `none`, `serial`, `littlefs`, `both` |
| `RANGING_TEST_NAME` | `real_distance` | Nombre de la prueba (nombre de archivo) |
| `RANGING_REF_M` | `0.5` | Distancia real de referencia (m) |
| `RANGING_SAMPLES` | `500` | Número de muestras |
| `RANGING_MAX_STD_MM` | `15` | Máx. desv. típica aceptada |
| `RANGING_MIN_ACCEPT_PCT` | `90` | Mín. % muestras aceptadas |
| `RANGING_MAX_ERR_MM` | `50` | Máx. error absoluto (mm) |
| `SERIAL_PORT` | — | Puerto USB (requerido si `serial` o `both`) |

Pinout Pro con display:

```bash
pio test -e test_uwb_pinout_pro -f test_board_pinout
```

WiFi (integración; AP `ZMS` según `src/config.h`):

```bash
pio test -e test_uwb_wifi -f test_wifi
```

## Configuración de prueba

- `test/src/test_config.h` — incluye `src/config.h` y define `TEST_WIFI_LOCAL_IP` (`.199`) para no chocar con el tag en `.200`.
- Credenciales WiFi: editar `WIFI_SSID` / `WIFI_PASS` en `src/config.h`.
