# Changelog

## [UNRELEASED]

Based on MeshCore v1.17.1
main@d92964352441e53b93e8667b802e04f6e072b39e

### Funcionalidades

- REPETIDOR: Atribuição automática de regiões geográficas a partir da localização do nó (coordenadas das preferências ou GPS).
- RÁDIO: Novo modo automático para o limiar de interferência (`set int.thresh 255`, `get int.thresh` responde `auto`), guardado pela flag de build `LUSOFW_RADIO_INT_THR_AUTO`.
- REPETIDOR: Adicionada proteção de adverts (`ENABLE_ADVERT_PROTECT`): o advert de cada repetidor remoto é repetido no máximo uma vez a cada 12 horas (por chave pública); os adverts duplicados continuam a ser processados localmente, mas já não são retransmitidos. Motivado por firmware muito antigo que entra frequentemente em boot loop e gera advert storms durante horas.

### Segurança

- REPETIDOR: A sincronização de hora por rede aceita agora correções do relógio em qualquer direção (incluindo para trás).
- REPETIDOR: Adicionado limite de sanidade contra saltos impossíveis para a frente (timekeeper com bug ou comprometido).

### Correções

- CLIENTE: A página inicial do GPS agora fica oculta no carrossel quando nenhum GPS é detectado na porta série no arranque, em vez de aparecer sempre quando `ENV_INCLUDE_GPS` está ativado.
- REPETIDOR: Corrigido o envio de flood adverts: agora respeita o `default_scope` e o modo de hash de caminho (`path_hash_mode` nas preferências).
- REPETIDOR: Removida a redução probabilística de flood adverts.
- REPETIDOR: Corrigido um *buffer overflow* no acumulador de comandos série.
- NRF52: Limpo o registo de retenção GPREGRET (0) no arranque e antes do SYSTEMOFF, evitando que um valor mágico de DFU residual faça o dispositivo arrancar no modo bootloader.

#### Melhorias

- CLIENTE: Extraídos para `src/lusofw/` os módulos comuns `BootScreen` (logótipo de arranque), `BatteryCurve` (curva de descarga LiPo) e `SmartAdverts` (agendamento determinístico de adverts), eliminando código duplicado entre as UIs; sem alteração de comportamento.
- RÁDIO: Os logs de RSSI/CAD em `isChannelActive()` e o log de conclusão de TX no `Dispatcher` passaram a estar guardados pela flag `LUSOFW_RADIO_DEBUG`.
- CLI: Adicionada a leitura do motivo de arranque do ESP32 (`get pwrmgt.bootreason`).
- CLI: Reforçada a cópia segura de strings no tratamento de comandos.
- RÁDIO: Ajustado o atraso de retentativa CAD para um intervalo entre 120 e 360 ms.

#### Observabilidade

- DISPATCHER: Adicionados logs de debug para conclusão de TX.
- ESTATÍSTICAS: Removidos parâmetros não usados na formatação de respostas.

#### Build e Configuração

- BUILD: Adicionadas as flags `LUSOFW_LIPO_CURVE` e `LUSOFW_BOOT_LOGO_PT`, que permitem reverter a curva de bateria LiPo e o logótipo de arranque para o comportamento upstream sem editar código.
- BUILD: Convertidos os blocos mortos `#if 0` das UIs (ui-new/ui-orig) em guards nomeados e removido código de referência duplicado.
- BUILD: `examples/simple_room_server/MyMesh.cpp` revertido para o conteúdo upstream: novas instalações de room server passam a utilizar o intervalo de flood advert predefinido do MeshCore (47 h); as instalações existentes não são afetadas.
- BUILD: Removidas divergências cosméticas face ao conteúdo upstream e normalizados os fins-de-linha CRLF nos variants `minewsemi_me25ls01` e `wio_wm1110`, reduzindo conflitos em futuras sincronizações.
- BUILD: Adicionada a variável `DISABLE_DEBUG` ao processo de build.
- BUILD: Reorganizadas as build flags no platformio.ini.
- BUILD: Adicionado o ambiente de build da ponte RS232 (`heltec_v4_repeater_bridge_rs232`) para o Heltec v4.

## [v2026.7.1] - 01/07/2026

Based on MeshCore v1.16.0
main@e8d3c53ba1ea863937081cd0caad759b832f3028

### Features

- Added SHTC3 temperature/humidity sensor support (fixed I²C address 0x70) to the environment-sensor manager.
- Added smart adverts: deterministic, collision-resistant flood-advert scheduling over a rolling 23h window, with each node's slot derived from its name and public key plus per-cycle jitter.
- Disabled the per-hop probabilistic forwarding filter (`P(h)=0.308^(hops-1)`) that randomly dropped flood adverts as they propagated.
- Implemented hardware Channel Activity Detection (CAD) listen-before-talk before each transmit, enabled by default (Cherry-picked upstream PR #1727).
- Loop detection is now enabled by default at `LOOP_DETECT_MINIMAL` sensitivity (previously off).
- Outbound packets are now expired and dropped once they exceed the maximum queue age (`MAX_PACKET_QUEUE_AGE_MS`).
- Reverted the default airtime (duty-cycle) factor to 1.0, i.e. a 50% duty cycle, rolling back the prior setting that caused internationalization-related issues.
- The repeater home screen now surfaces battery charge as an intuitive percentage, so remaining power can be read at a glance.
- The repeater path-hash mode now defaults to 2 bytes (`path_hash_mode=1`), up from 1 byte.

### Client

- Implemented hardware Channel Activity Detection (CAD) listen-before-talk before each transmit, enabled by default (Cherry-picked upstream PR #1727).
- Rendered the T114 boot logo as a true-color RGB image at its native panel resolution (192×54).
- Shrunk the T114 battery indicator icon to 16×8 logical pixels.
- Switched the T114 ST7789 font to DejaVu Sans UI.
- The client path-hash mode now defaults to 2 bytes (`path_hash_mode=1`), up from 1 byte.

### Security

- Fixed network time replay vulnerability; reject time-source timestamps not newer than the last accepted one.
- Enforce initial sync (forward-only) vs maintenance sync (±60s), replacing the symmetric ±30s logic that allowed backward clock moves.

### Improvements

- Updated MeshCore website links and logo placement across the UI and splash screens.

### KTLO

- Added `--upload ` argument to `build.sh` to compile and upload firmware in a single command.
- Increased the GitHub Actions parallel job limit and updated the PlatformIO dependencies.
- Removed the `ENABLE_CONSENSUS_TIME_SYNC` feature from the codebase.

## [v2026.5.2] - 25/05/2026

Based on MeshCore v1.15.0
main@ecd0cfc1c133aad93e65257f002151591f6bcfd9

### Features

- Add support for the Seeed P1 Pro bridge type
- Restrict the “01” advert reduction to repeaters only

## [v2026.5.1] - 22/05/2026

Based on MeshCore v1.15.0
main@ecd0cfc1c133aad93e65257f002151591f6bcfd9

### Features

- Update versioning scheme to year.month.release format (e.g. 2026.5.1)
- Include Lora longer preamble #1954 to improve 868MHz performance in Portugal
- Reduce advert rate from 3 to 1 within the permitted broadcast window
- Prevent advert from mobile repeaters identified with (01)

## [v0.0.7] - 01/04/2026

Based on MeshCore v1.14.1
main@467959cc3bfc884e5f3425caac89453a450151b6

### Features

- Increase default airtime factor to 9.0 (targets ~10% duty cycle)
- Set default `flood_advert_interval` to 24 hours (instead of disabled)
- Only schedule flood advert timers when `flood_advert_interval` is greater than 0
- Add version-aware defaults migration with persisted firmware version tracking
- Apply only defaults newer than the stored version during migration
- `radio.lna` renamed to `radio.rxgain`, use with `get` and `set`
- Add rs232 support for Xiao NRF52 (serial1, rx(7), tx(6))

## [v0.0.6] - 06/03/2026

Based on MeshCore v1.14.0
dev@3fe2dd7f48733fe77da7549cd24ef28bf07e1e5a

### Features

- Disable advert interval by default (was 2 minutes)
- Disable flood advert interval by default (was 12 hours)
- Enable listen before talk with interference threshold of 14
- Refactor buildAdvertData to use prefs when no GPS support is enabled
- Set default loop detection preference to minimal sensitivity
- Add 0x01 to reserved identity hash prefixes

------

## [v0.0.5] - 04/03/2026

Based on upstream MeshCore dev@3e5522fcded70751c5a06ad1183b3eb1821397fd.

### Features

- Added master time synchronization feature using a network time master to sync radio clocks
- Consensus time sync is now optional and disabled by default (replaced by master time sync)
- Master time sync only accepts timestamps from a specific trusted identity
- Master time sync applies to packets with path length < 8 and timestamps after 2026
- Improved time consensus algorithm with trimmed mean approach for better outlier rejection
- Time sync now distinguishes initial sync (unlimited forward) vs maintenance sync (±60s limit)
- Extended time sync sample collection to accept adverts up to 8 hops (was 4)
- Flood advert filter now applies to all non-CHAT and non-NONE advert types (was REPEATER only)
- Increased time sync sample buffer from 8 to 16 samples
- Improved debug logging with human-readable DateTime formatting

### Build Configuration

- Added `ENABLE_MASTER_TIME_SYNC` build flag (enabled by default)
- Disabled `ENABLE_CONSENSUS_TIME_SYNC` by default (can be re-enabled if needed)

### Devcontainer

- Added Bun feature for development environment
- Changed USB mount from volume to device passthrough
- Added opencode CLI installation

------

## [v0.0.4] - 22/02/2026

Based on upstream MeshCore dev@bbc5f0c11a1fbf613cac4f10525cfe60699c7373.

### Features

- Further logic improvement of the repeater flood adverts limiter,

------

## [v0.0.3] - 21/02/2026

Based on upstream MeshCore dev@bbc5f0c11a1fbf613cac4f10525cfe60699c7373.

### Features

- Fix errors with the repeater flood adverts limiter,

------

## [v0.0.2] - 19/02/2026

Based on upstream MeshCore dev@bbc5f0c11a1fbf613cac4f10525cfe60699c7373.

### Features

- Enable AHTx0 sensors
- Heltec v4 build error fix
- Consensus time sync over the network based on advert data
- Limit repeater flood advert packet forwarding using a probabilistic reduction
- Limit repeater flood adverts to the maintenance window between 02:00 to 06:00

### CLI commands

- `get flood.advert.base`
- `set flood.advert.base <0-1>`: defaults to 0.308f

------

## [v0.0.1] - 13/02/2026

Based on upstream MeshCore dev@3f33455b4d96426b2f8b462b48ff1d4e31de1bf8.

### Features

- Change default configuration to use 433 MHz frequency band
- Configure bridge mode to be disabled by default
- Disable advertising functionality during system initialization
- Disable all sensor features and interfaces except BME280, BMP280 and INA3221
- Enable CLI boosted gain settings for SX126X radio modules (LNA)
- Enforce duty cycle limits using token bucket algorithm
- Implement hardware support for T114 sensor modules
- Neighbours older than 48h will be automatically removed.
- The #portugal region is added and set as flood by default.

### CLI commands

- `get radio.lna`: Gets the SX126X boosted gain status on/off
- `set radio.lna <on|off>`: Sets the SX126X boosted gain status on/off

### Notes

The T114 platform now features I2C sensor compatibility with the following pin configuration:
 1. VCC (3v3)
 2. GND
 3. GPIO8 (SCL)
 4. GPIO7 (SDA)
