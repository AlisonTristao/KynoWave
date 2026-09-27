// Minimal ST7735 driver over esp_lcd's SPI panel IO (ESP-IDF ships no ST7735
// panel). Init sequence: the "black tab" one TFT_eSPI used on this LCD.
//
// draw() is asynchronous: the pixels go out by DMA and the done callback runs
// (from the SPI interrupt) when the buffer can be reused.
#pragma once

#include <cstdint>

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <esp_err.h>
#include <esp_lcd_panel_io.h>

class St7735 {
public:
    struct Config {
        spi_host_device_t host;
        gpio_num_t mosi;
        gpio_num_t sck;
        gpio_num_t cs;
        gpio_num_t dc;
        gpio_num_t reset;
        uint32_t pclk_hz;
        uint32_t max_transfer_bytes;   // largest draw() (bytes)
    };
    using DoneFn = void (*)(void* context);

    /// Landscape, 160 x 128, rotated like the MVP firmware (TFT_eSPI rotation 3).
    static constexpr uint16_t kWidth = 160;
    static constexpr uint16_t kHeight = 128;

    esp_err_t begin(const Config& config, DoneFn done, void* context);

    /// Pixels (RGB565, big-endian) for the inclusive area; async (see above).
    esp_err_t draw(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const void* pixels);

private:
    static bool on_color_done(esp_lcd_panel_io_handle_t io, esp_lcd_panel_io_event_data_t* event,
                              void* context);
    esp_err_t command(uint8_t cmd, const uint8_t* data, size_t size);

    esp_lcd_panel_io_handle_t io_ = nullptr;
    DoneFn done_ = nullptr;
    void* context_ = nullptr;
};
