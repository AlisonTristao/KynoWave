#include "display/St7735.h"

#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

namespace {

constexpr const char* kTag = "st7735";

// MADCTL bits.
constexpr uint8_t kMadMx = 0x40;
constexpr uint8_t kMadMv = 0x20;
constexpr uint8_t kMadRgb = 0x00;   // black tab panels are RGB (BGR would be 0x08)

struct InitStep {
    uint8_t cmd;
    uint8_t data[16];
    uint8_t size;
    uint16_t delay_ms;
};

// Adafruit/TFT_eSPI "Rcmd1 + Rcmd2red + Rcmd3" for the black tab ST7735R.
constexpr InitStep kInit[] = {
    {0x01, {}, 0, 150},                                    // SWRESET
    {0x11, {}, 0, 500},                                    // SLPOUT
    {0xB1, {0x01, 0x2C, 0x2D}, 3, 0},                      // FRMCTR1
    {0xB2, {0x01, 0x2C, 0x2D}, 3, 0},                      // FRMCTR2
    {0xB3, {0x01, 0x2C, 0x2D, 0x01, 0x2C, 0x2D}, 6, 0},    // FRMCTR3
    {0xB4, {0x07}, 1, 0},                                  // INVCTR
    {0xC0, {0xA2, 0x02, 0x84}, 3, 0},                      // PWCTR1
    {0xC1, {0xC5}, 1, 0},                                  // PWCTR2
    {0xC2, {0x0A, 0x00}, 2, 0},                            // PWCTR3
    {0xC3, {0x8A, 0x2A}, 2, 0},                            // PWCTR4
    {0xC4, {0x8A, 0xEE}, 2, 0},                            // PWCTR5
    {0xC5, {0x0E}, 1, 0},                                  // VMCTR1
    {0x20, {}, 0, 0},                                      // INVOFF
    {0x36, {kMadMx | kMadMv | kMadRgb}, 1, 0},             // MADCTL: rotation 3
    {0x3A, {0x05}, 1, 0},                                  // COLMOD: 16 bit
    {0xE0, {0x02, 0x1C, 0x07, 0x12, 0x37, 0x32, 0x29, 0x2D, 0x29, 0x25, 0x2B, 0x39, 0x00, 0x01, 0x03, 0x10}, 16, 0},
    {0xE1, {0x03, 0x1D, 0x07, 0x06, 0x2E, 0x2C, 0x29, 0x2D, 0x2E, 0x2E, 0x37, 0x3F, 0x00, 0x00, 0x02, 0x10}, 16, 0},
    {0x13, {}, 0, 10},                                     // NORON
    {0x29, {}, 0, 100},                                    // DISPON
};

}  // namespace

esp_err_t St7735::begin(const Config& config, DoneFn done, void* context) {
    done_ = done;
    context_ = context;

    spi_bus_config_t bus = {};
    bus.mosi_io_num = config.mosi;
    bus.miso_io_num = -1;
    bus.sclk_io_num = config.sck;
    bus.quadwp_io_num = -1;
    bus.quadhd_io_num = -1;
    bus.max_transfer_sz = static_cast<int>(config.max_transfer_bytes);
    esp_err_t err = spi_bus_initialize(config.host, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "SPI bus init failed (%s)", esp_err_to_name(err));
        return err;
    }

    esp_lcd_panel_io_spi_config_t io = {};
    io.cs_gpio_num = config.cs;
    io.dc_gpio_num = config.dc;
    io.spi_mode = 0;
    io.pclk_hz = config.pclk_hz;
    io.trans_queue_depth = 4;
    io.on_color_trans_done = &St7735::on_color_done;
    io.user_ctx = this;
    io.lcd_cmd_bits = 8;
    io.lcd_param_bits = 8;
    err = esp_lcd_new_panel_io_spi(static_cast<esp_lcd_spi_bus_handle_t>(config.host), &io, &io_);
    if (err != ESP_OK) {
        ESP_LOGE(kTag, "panel IO init failed (%s)", esp_err_to_name(err));
        return err;
    }

    // Hardware reset.
    gpio_config_t reset = {};
    reset.pin_bit_mask = 1ULL << config.reset;
    reset.mode = GPIO_MODE_OUTPUT;
    gpio_config(&reset);
    gpio_set_level(config.reset, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(config.reset, 1);
    vTaskDelay(pdMS_TO_TICKS(120));

    for (const InitStep& step : kInit) {
        err = command(step.cmd, step.size != 0 ? step.data : nullptr, step.size);
        if (err != ESP_OK) {
            ESP_LOGE(kTag, "init command 0x%02X failed (%s)", step.cmd, esp_err_to_name(err));
            return err;
        }
        if (step.delay_ms != 0) vTaskDelay(pdMS_TO_TICKS(step.delay_ms));
    }
    return ESP_OK;
}

esp_err_t St7735::command(uint8_t cmd, const uint8_t* data, size_t size) {
    return esp_lcd_panel_io_tx_param(io_, cmd, data, size);
}

esp_err_t St7735::draw(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, const void* pixels) {
    const uint8_t columns[4] = {static_cast<uint8_t>(x1 >> 8), static_cast<uint8_t>(x1),
                                static_cast<uint8_t>(x2 >> 8), static_cast<uint8_t>(x2)};
    const uint8_t rows[4] = {static_cast<uint8_t>(y1 >> 8), static_cast<uint8_t>(y1),
                             static_cast<uint8_t>(y2 >> 8), static_cast<uint8_t>(y2)};
    esp_err_t err = command(0x2A, columns, sizeof(columns));   // CASET
    if (err == ESP_OK) err = command(0x2B, rows, sizeof(rows));  // RASET
    if (err != ESP_OK) return err;
    const size_t bytes = static_cast<size_t>(x2 - x1 + 1) * (y2 - y1 + 1) * 2U;
    return esp_lcd_panel_io_tx_color(io_, 0x2C, pixels, bytes);   // RAMWR
}

bool St7735::on_color_done(esp_lcd_panel_io_handle_t, esp_lcd_panel_io_event_data_t*, void* context) {
    auto* self = static_cast<St7735*>(context);
    if (self->done_ != nullptr) self->done_(self->context_);
    return false;
}
