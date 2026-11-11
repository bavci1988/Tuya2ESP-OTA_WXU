# Tuya2ESP-OTA_WXU

Temporäre **Minimal-Firmware zur Migration** von Tuya-WXU-Geräten (Allwinner/
XRadio **XR806**) auf ESPHome — **ohne Hardware-/UART-Flash**, ausschließlich
per OTA über das laufende TuyaOS.

Dieses Repository bündelt den kompletten Arbeitsstand an einem Ort und wird
direkt auf `main` weiterentwickelt (kein Feature-Branch).

## Zweck / Migrationskonzept

1. **Tuya → ESPHome OTA:** ein diff2ya-SING-Paket, das das laufende TuyaOS
   akzeptiert, ersetzt die App durch eine minimale ESPHome-Firmware.
2. **Selbst-Umformatierung:** die konvertierte Firmware schreibt beim ersten
   Boot den BOOT-Header vom Tuya-Layout (1500K/400K) auf das ESPHome-Layout
   (1276K/640K) um — danach normal große OTA-Pakete.
3. **ESPHome → ESPHome OTA:** ab da ganz normale ESPHome-Updates.

Ziel ist eine **schlanke, temporäre** Migrations-Firmware, nicht ein Dauer-Fork.

## Struktur

| Pfad | Inhalt |
|---|---|
| `esphome/` | ESPHome-Fork mit XR806-Plattform, WiFi, OTA-Backend, Image-Tools (Snapshot) |
| `sdk/` | XR806-SDK (TuyaOS-lite) mit `trv16_port`, Make-Adapter, Image-Konfiguration (Snapshot) |

Beides sind **Snapshots des aktuellen Stands** (nur getrackte Dateien, ohne die
alte Git-Historie der Quell-Repos).

## Herkunft (Provenienz)

Stand 2026-11-11, kopiert aus den Arbeits-Repos (Branch
`Convert-tuya2esphome-ota`):

| Unterordner | Quelle | Commit |
|---|---|---|
| `esphome/` | `bavci1988/esphome-xr806` | `910e7124f8` |
| `sdk/` | `bavci1988/xr806-sdk` | `fe411b7` |

Die Reverse-Engineering- und Cloud-Emulator-Vorarbeiten (atop-Signatur,
KV-Krypto, diff2ya-Generator, Geräte-Identität) liegen weiterhin im privaten
Lab-Repo `bavci1988/xr806-lab` und werden bei Bedarf hierher übernommen.

## Build (XR806 / trv16_port)

Der Build läuft über den SDK-Make-Adapter, der den ESPHome-Fork einbindet
(`sdk/.../xr806_esphome.mk`). Details in `sdk/` und `esphome/components/xr806/`.

## Status

Erstbefüllung. Weiteres Vorgehen: Cloud-Emulator (offline), dann kontrollierter
Hardware-Test mit zuvor verifiziertem UART-Recovery-Pfad.
