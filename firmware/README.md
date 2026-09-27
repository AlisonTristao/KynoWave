# KynoWave firmware

ESP-IDF, ESP32-S3. Mede o nível sonoro do microfone (MAX4466 no ADC) e manda
para o [TraceView](https://github.com/AlisonTristao/TraceView) pela
[BTPDevice](https://github.com/AlisonTristao/BTPDevice). O display mostra o
link, o nível e um log; os LEDs fazem de VU meter.

## Compilar e gravar

BTPDevice, BTP e TinyShell vêm dos checkouts irmãos deste repositório:

```text
c:/git/BTPDevice   c:/git/BTP   c:/git/TinyShell   c:/git/kynowave
```

```text
cd firmware
pio run -t upload            # grava
pio device monitor           # console na UART0 (pinos tx/rx do conector)
```

A porta USB nativa é o link da BTPDevice com o TraceView (não é console).

## TraceView

1. Ligue a USB nativa da placa no PC.
2. No TraceView, adicione `kynowave` (Serial, automático).
3. Plote `audio.level` (`rms`, `dbfs`, `peak`, até 20 Hz).

## Terminal

| Comando | O quê |
|---|---|
| `audio -stats` | Taxa configurada e medida, blocos, tempo por bloco, erros de leitura, último nível |
| `audio -rate <hz>` | Taxa de amostragem, 2000..16000 |
| `leds -vu <0\|1>` | VU meter ligado/desligado |
| `leds -step <contagens>` | RMS por LED no VU meter (padrão 9) |
| `leds -set <máscara>` | Padrão fixo (bit i = LED i), desliga o VU |
| `display -backlight <0\|1>` | Luz de fundo |
| `btp -status`, `-errors`, `-topics`, ... | Da BTPDevice |

## Como funciona

| Task | Core | O quê |
|---|---|---|
| `audio` | 1 | Blocos de 256 amostras (16 ms a 16 kHz) a cada 50 ms: tira o DC, passa-baixas de 3 kHz, RMS e pico |
| `display` | 1 | LVGL no ST7735 (driver próprio sobre `esp_lcd`) |
| `app_main` | — | A cada 50 ms: LEDs, nível e estado do link na tela |
| `btpdev_comms`, `btpdev_shell` | 0 / — | BTPDevice: link, sessão, tópicos, comandos |

O microfone é lido no GPIO1 (ADC1). No protótipo a trilha do MAX4466 vai
para o GPIO17 (ADC2, compartilhado com o Wi-Fi); um jumper leva o sinal ao
GPIO1.

## Wi-Fi e chave (links TCP e BLE)

Veja [data/README.txt](data/README.txt): `wifi.txt` e `bally.key` vão para a
partição LittleFS com `pio run -t uploadfs`.
