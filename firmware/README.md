# KynoWave firmware

ESP-IDF, ESP32-S3. Transmite o microfone (MAX4466 no ADC) para o
[TraceView](https://github.com/AlisonTristao/TraceView) pela
[BTPDevice](https://github.com/AlisonTristao/BTPDevice): o áudio contínuo a
83 333 Hz (o máximo do ADC do S3) e o nível sonoro. O display mostra o link, o
nível e um log; os LEDs fazem de VU meter.

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
3. Adicione um widget **Audio Spectrum** no tópico `audio.pcm`: toca o som e
   mostra a FFT ao vivo.
4. Plote `audio.level` (`rms`, `dbfs`, `peak`, até 20 Hz).

`audio.pcm` é um tópico stream: blocos de 1024 amostras (`int16`, contagens
de -2048 a 2047 em torno do DC) com a taxa medida e um contador de blocos,
~170 KB/s pela USB nativa. Cabe no Serial e no TCP, não no BLE nem no
ESP-NOW.

## Terminal

| Comando | O quê |
|---|---|
| `audio -stats` | Taxa configurada e medida, blocos lidos e enviados, estouros do DMA, último nível |
| `audio -rate <hz>` | Taxa de amostragem, 8000..83333 |
| `leds -vu <0\|1>` | VU meter ligado/desligado |
| `leds -step <contagens>` | RMS por LED no VU meter (padrão 9) |
| `leds -set <máscara>` | Padrão fixo (bit i = LED i), desliga o VU |
| `display -backlight <0\|1>` | Luz de fundo |
| `btp -status`, `-errors`, `-topics`, ... | Da BTPDevice (`-topics` mostra blocos enviados/descartados do stream) |

## Como funciona

| Task | Core | O quê |
|---|---|---|
| `audio` | 1 | ADC contínuo (DMA): cada frame de 1024 amostras vai para o stream sem DC; o nível (passa-baixas de 3 kHz, RMS e pico) sai a cada 50 ms |
| `display` | 1 | LVGL no ST7735 (driver próprio sobre `esp_lcd`) |
| `app_main` | — | A cada 50 ms: LEDs, nível e estado do link na tela |
| `btpdev_comms`, `btpdev_shell` | 0 / — | BTPDevice: link, sessão, tópicos, comandos |

O microfone é lido no GPIO1 (ADC1). No protótipo a trilha do MAX4466 vai
para o GPIO17 (ADC2, compartilhado com o Wi-Fi e sem DMA); um jumper leva o
sinal ao GPIO1.

## Wi-Fi e chave (links TCP e BLE)

Veja [data/README.txt](data/README.txt): `wifi.txt` e `bally.key` vão para a
partição LittleFS com `pio run -t uploadfs`.
