#ifndef CARDPUTER_DISPLAY_H
#define CARDPUTER_DISPLAY_H

#include "../../display_hal.h"

/**
 * @brief Cardputer / Cardputer ADV display — ST7789v2, 135×240.
 *
 * LovyanGFX's autodetect has no Cardputer entry, so the panel is configured
 * explicitly. Values are taken from M5GFX's autodetect branch, which uses the
 * same configuration for both Cardputer revisions.
 */
class LGFX_Cardputer : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;
    lgfx::Light_PWM    _light_instance;

public:
    LGFX_Cardputer() {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host    = SPI3_HOST;
            cfg.spi_mode    = 0;
            cfg.freq_write  = 40000000;
            cfg.freq_read   = 16000000;
            cfg.spi_3wire   = true;
            cfg.use_lock    = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk    = 36;
            cfg.pin_mosi    = 35;
            cfg.pin_miso    = -1;
            cfg.pin_dc      = 34;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs       = 37;
            cfg.pin_rst      = 33;
            cfg.pin_busy     = -1;
            cfg.panel_width  = 135;
            cfg.panel_height = 240;
            cfg.offset_x     = 52;
            cfg.offset_y     = 40;
            cfg.invert       = true;
            cfg.readable     = true;
            cfg.bus_shared   = false;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = 38;
            cfg.freq        = 256;
            cfg.pwm_channel = 7;
            cfg.invert      = false;
            cfg.offset      = 16;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }
};

class CardputerDisplay : public DisplayHAL {
public:
    bool init() override;
    void setBrightness(uint8_t brightness) override;
    lgfx::LGFX_Device& getDevice() override;
    int width() override;
    int height() override;
    bool isRound() override;

private:
    LGFX_Cardputer _tft;
};

#endif // CARDPUTER_DISPLAY_H
