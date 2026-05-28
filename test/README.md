# Tests Indoor_UWB

Tests [Unity](https://docs.platformio.org/en/latest/advanced/unit-testing/index.html) para validar la lógica del proyecto sin depender del firmware completo en `src/`.

## Suites

| Carpeta | Qué valida |
|---------|------------|
| `test_trilateration` | `norm3`, `trilateration3D`, coherencia de distancias |
| `test_anchor_list` | `AnchorList` (alta/baja/búsqueda/capacidad), POD `EspNowPosition` |
| `test_board_pinout` | Pines DW1000 según placa (`INDOOR_UWB_BOARD_*`) |
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
pio test -e test_uwb_unit
```

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
