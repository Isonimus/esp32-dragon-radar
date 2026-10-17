#ifndef M5STICKC_DISPLAY_H
#define M5STICKC_DISPLAY_H

#include "../../display_hal.h"

/**
 * @brief M5StickC Plus 2 display — ST7789v2, 135×240.
 * LovyanGFX panel config originally from display_m5stickc.h.
 */
class LGFX_M5StickC_Plus2 : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance;
    lgfx::Bus_SPI      _bus_instance;
    lgfx::Light_PWM    _light_instance;

public:
    LGFX_M5StickC_Plus2() {
        {
            auto cfg = _bus_instance.config();
            cfg.spi_host    = HSPI_HOST;
            cfg.spi_mode    = 3;
            cfg.freq_write  = 40000000;
            cfg.freq_read   = 15000000;
            cfg.spi_3wire   = true;
            cfg.use_lock    = true;
            cfg.dma_channel = SPI_DMA_CH_AUTO;
            cfg.pin_sclk    = 13;
            cfg.pin_mosi    = 15;
            cfg.pin_miso    = -1;
            cfg.pin_dc      = 14;
            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs           = 5;
            cfg.pin_rst          = 12;
            cfg.pin_busy         = -1;
            cfg.memory_width     = 240;
            cfg.memory_height    = 320;
            cfg.panel_width      = 135;
            cfg.panel_height     = 240;
            cfg.offset_x         = 52;
            cfg.offset_y         = 40;
            cfg.invert           = true;
            cfg.rgb_order        = false;
            cfg.dlen_16bit       = false;
            cfg.bus_shared       = false;
            _panel_instance.config(cfg);
        }
        {
            auto cfg = _light_instance.config();
            cfg.pin_bl      = 27;
            cfg.freq        = 256;
            cfg.pwm_channel = 7;
            cfg.invert      = false;
            cfg.offset      = 40;
            _light_instance.config(cfg);
            _panel_instance.setLight(&_light_instance);
        }
        setPanel(&_panel_instance);
    }
};

class M5StickCDisplay : public DisplayHAL {
public:
    bool init() override;
    void setBrightness(uint8_t brightness) override;
    lgfx::LGFX_Device& getDevice() override;
    int width() override;
    int height() override;
    bool isRound() override;

private:
    LGFX_M5StickC_Plus2 _tft;
};

#endif // M5STICKC_DISPLAY_H
