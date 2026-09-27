# KynoWave + BTPDevice: plano

Branch: `btpdevice-test`. Hardware: o protótipo, com um microfone MAX4466
ligado direto no ADC (GPIO17). Objetivo duplo: reescrever o firmware do KynoWave
limpo, em ESP-IDF, e usar a placa como **primeiro teste em hardware da
[BTPDevice](https://github.com/AlisonTristao/BTPDevice)** — o TraceView
recebendo a intensidade do microfone.

## 1. Ponto de partida

`software_mvp/` é Arduino (TinyShell, TFT_eSPI, LVGL 8, PubSubClient, Wi-Fi e
senhas no `platformio.ini`). Funciona, mas:

- a BTPDevice é **só ESP-IDF** — não dá para encaixá-la no Arduino;
- a captura do microfone faz *busy-wait* com `analogRead()` a 16 kHz num loop
  com `taskYIELD()`, segurando um núcleo inteiro;
- Wi-Fi, MQTT, UDP, OTA, logger e display estão todos dentro de `KynoWave`.

**O que se aproveita:** o display (layout LVGL da `DisplayLCD`: barra de
status + área de log, o ícone de Wi-Fi) e o filtro do microfone (remoção de DC
+ passa-baixas de 3 kHz em 3 estágios + RMS). O resto é reescrito.

## 2. Decisões

| # | Decisão | Por quê |
|---|---|---|
| K1 | **ESP-IDF puro** (PlatformIO `framework = espidf`), projeto novo em `firmware/` | A BTPDevice exige; `software_mvp/` fica de referência até o fim e sai na etapa 6 |
| K2 | Mesmo layout dos exemplos da BTPDevice: `main/btp/{btp_device.h, btp_storage.h, btp_commands.cpp, btp_topics.cpp}` | O KynoWave vira o exemplo "de verdade" da lib |
| K3 | BTPDevice, BTP e TinyShell como componentes dos checkouts irmãos (`../../BTPDevice` etc.), como no bally_OS | Testa o código da lib que ainda não tem release |
| K4 | Primeiro link: **Serial (USB nativo)**; depois TCP e BLE | Serial não precisa de chave nem de Wi-Fi: menos variáveis no primeiro teste |
| K5 | Microfone: MAX4466 no ADC (GPIO17), com `adc_oneshot` numa task própria no core 1 | É o que o protótipo tem, e já funciona |
| K6 | Display: driver ST7735 próprio sobre `esp_lcd` (panel IO SPI) + LVGL 8 | TFT_eSPI é Arduino; o `esp_lcd` do IDF não tem ST7735 pronto, mas a sequência de init é curta (portar a do TFT_eSPI, BLACKTAB) |
| K7 | Sem senhas no código: Wi-Fi em `wifi.txt` e chave em `bally.key`, gravados na partição LittleFS com `pio run -t uploadfs` | É o provisionamento da BTPDevice |
| K8 | Console (`ESP_LOGx`) na UART0 (pinos `tx`/`rx` do conector); o USB nativo é da BTPDevice | O USB não serve aos dois ao mesmo tempo; logs de aplicação também vão para o TraceView (`btpdev::log`) |

## 3. Arquitetura

```text
core 1  audio      (prio 5)  ADC oneshot 16 kHz, blocos de 256 -> filtro -> RMS
                             -> atômico `level` (+ LEDs como VU)
core 1  display    (prio 2)  LVGL a ~10 Hz: barra (link/sessão, hora) + nível + log
core 0  btpdev_comms (prio 4, BTPDevice)  link, sessão, tópicos (lê `level`)
any     btpdev_shell (BTPDevice)          comandos do terminal
```

Nada de lógica dentro de uma classe-deus: um módulo por coisa (`Audio`,
`Display`, `Leds`), cada um com `begin()`/estado próprio; `main.cpp` só compõe.

### Telemetria (primeira versão)

| Tópico | Campos | Taxa |
|---|---|---|
| `audio.level` | `rms` (contagens do ADC, f32), `dbfs` (f32, relativo ao fundo de escala), `peak` (u16) | até 20 Hz |

### Comandos

| Comando | O quê |
|---|---|
| `audio -stats` | Taxa real de amostragem, blocos, último RMS/pico, tempo por bloco |
| `audio -rate <hz>` | Taxa de amostragem (8000..16000) |
| `leds -mode <vu|off>` | LEDs como VU meter ou apagados |
| `display -backlight <0|1>` | Luz de fundo |
| `btp -...` | Da própria lib (status, erros, tópicos, chave, link, wifi, fs) |

## 4. Etapas

### Etapa 0: esqueleto ESP-IDF
- [ ] `firmware/` com `platformio.ini` (espidf, ESP32-S3, 16 MB), `CMakeLists.txt` com os componentes irmãos, `partitions.csv` (app + LittleFS `storage`), `sdkconfig.defaults` (TinyUSB CDC, console UART0, NimBLE desligado por enquanto).
- [ ] `main/btp/` com o dispositivo `kynowave`, link Serial, preset `Small`.
- **Pronto quando:** compila e sobe com um tópico de teste (contador).

### Etapa 1: BTPDevice no hardware (Serial) — o teste da lib
- [ ] TraceView acha a porta pelo nome, abre sessão, terminal responde (`help`, `btp -status`).
- [ ] Tópico de teste plotando; `btp -errors` zerado.
- [ ] Anotar e corrigir na BTPDevice o que aparecer (sessão, keepalive, COBS, fila).
- **Pronto quando:** 10 min de sessão estável com o TraceView.

### Etapa 2: microfone
- [ ] `Audio`: task no core 1, `adc_oneshot` (ADC2 canal 6 = GPIO17), amostragem por tempo sem busy-wait de núcleo inteiro (espera curta por bloco + `vTaskDelay` entre blocos), filtro portado do `AudioUdpStreamer`.
- [ ] Tópico `audio.level` e `audio -stats`; LEDs como VU (portado de `setLedsIntensity`).
- **Pronto quando:** o TraceView plota a intensidade reagindo a som, sem travar o terminal.

### Etapa 3: display
- [ ] Driver ST7735 (`esp_lcd` panel IO SPI: CS 4, RST 5, DC 6, MOSI 7, SCK 15, BL 16) + LVGL 8.
- [ ] UI portada da `DisplayLCD`: barra de status (link e sessão da BTPDevice no lugar do MQTT; hora quando houver), nível do microfone, área de log.
- **Pronto quando:** a tela mostra o estado da BTPDevice e o nível ao vivo.

### Etapa 4: TCP e BLE
- [ ] `wifi.txt` + `bally.key` em `firmware/data/` → `pio run -t uploadfs`.
- [ ] TCP (`kynowave.local:44300`) e BLE com o TraceView, cifrados; `btp -link_set` para trocar.
- [ ] **Verificar ADC2 com Wi-Fi ligado** (GPIO17 é ADC2; no ESP32-S3 o Wi-Fi tem prioridade sobre o ADC2 e a leitura pode falhar). Se falhar: medir a taxa de erro e decidir entre BLE/Serial para o KynoWave ou passar o microfone para um pino do ADC1 (GPIO1..10) na próxima revisão da placa.
- **Pronto quando:** os três links funcionam com a mesma telemetria.

### Etapa 5: exemplo na BTPDevice
- [ ] O que o KynoWave mostrar que é geral volta para a lib (bugs, ajustes de API).
- [ ] Um exemplo `06_sound_level` na BTPDevice, sem display, só ADC + tópico (qualquer ESP32-S3 com um microfone analógico).

### Etapa 6: limpeza
- [ ] Remover `software_mvp/` e `software_mvp.zip` do repositório.
- [ ] README do firmware (gravar, provisionar, TraceView).

### Etapa 7 (depois)
- Mais do que intensidade: espectro por bandas, eventos de ruído.
- Sensores ambientais (I2C), microSD, botões.

## 5. Riscos

| Risco | Mitigação |
|---|---|
| ADC2 (GPIO17) disputado com o Wi-Fi | Serial e BLE primeiro; medir no TCP; se não servir, microfone num pino do ADC1 na próxima placa |
| Sem console no USB (é da BTPDevice) | Console na UART0 do conector; `btpdev::log` para o TraceView |
| ST7735 sem driver pronto no IDF | Init curto, portado do TFT_eSPI; se travar, usar o componente de terceiros |
| Senhas no histórico do git (`platformio.ini` do `software_mvp`) | Não vão para o firmware novo; trocar a senha do Wi-Fi se o repositório for público |
