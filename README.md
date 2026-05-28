# Indoor_UWB

Localización indoor con UWB (DW1000), migrado desde [IndoorUWB](../IndoorUWB) a **PlatformIO** con arquitectura **Controller + Manager** (mismo patrón que [ZMSDoorGoon](../ZMSDoorGoon)).

## Roles

| Entorno PlatformIO        | Rol    | Descripción                               |
| ------------------------- | ------ | ----------------------------------------- |
| `uwb_tag_makerfabs_v1`    | Tag    | Móvil; ranging + web de mapa/anchors      |
| `uwb_anchor_makerfabs_v1` | Anchor | Fijo; ranging + configuración de posición |

## Placas soportadas

| Flag build                               | Placa                                                  |
| ---------------------------------------- | ------------------------------------------------------ |
| `INDOOR_UWB_BOARD_MAKERFABS_V1`          | Makerfabs ESP32 UWB v1.0 / Basic / Pro (SS=4)          |
| `INDOOR_UWB_BOARD_MAKERFABS_PRO_DISPLAY` | Pro con pantalla (SS=21)                               |
| `INDOOR_UWB_BOARD_ESP32DEV`              | ESP32 Dev Module (mismo pinout UWB que Makerfabs v1.0) |

Pinout DW1000 (Makerfabs v1.0): SCK 18, MISO 19, MOSI 23, CS 4, RST 27, IRQ 34.

## Estructura

```
src/
  config.h              # Pinout por placa + debug
  config_tag.h / config_anchor.h
  main.cpp
  manager/IndoorUWB_Manager.h
  controllers/          # WiFi, DW1000, ESP-NOW, OTA, Web, EEPROM, Tag
  models/               # AnchorList, trilateración
  assets/webpage.h
lib/TickerFree/         # Ticker no bloqueante (como DoorGoon)
```

## Compilar y subir

```bash
cd Indoor_UWB
pio run -e uwb_tag_makerfabs_v1
pio run -e uwb_tag_makerfabs_v1 -t upload

pio run -e uwb_anchor_makerfabs_v1 -t upload
```

Monitor serie: `pio device monitor -e uwb_tag_makerfabs_v1`

## Tests

Ver `test/README.md`. Compilar tests unitarios (sin placa):

```bash
pio test -e test_uwb_unit --without-uploading --without-testing
```

Con ESP32 conectado:

```bash
pio test -e test_uwb_unit
pio test -e test_uwb_wifi -f test_wifi   # requiere AP ZMS
```

## WiFi

Por defecto (editable en `config_tag.h` / `config_anchor.h`):

- SSID: `ZMS`
- Tag IP estática: `192.168.1.200`
- Anchor IP: `192.168.1.210`

## Notas

- Librería UWB en `lib/DW1000` (fork de [arduino-dw1000](https://github.com/thotro/arduino-dw1000) con parche ESP32: sin `SPI.usingInterrupt`). Makerfabs publica `mf_DW1000.zip` si necesitas su variante exacta.
- La trilateración 3D conserva el hook de IndoorUWB; el mapeo completo anchor↔distancia se puede ampliar en `IndoorUWB_Dw1000`.

## Licencia

Ver IndoorUWB original (MIT).
