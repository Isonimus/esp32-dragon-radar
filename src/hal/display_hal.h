#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

/**
 * @brief Abstract display interface.
 * Each board provides a concrete implementation with its own
 * LovyanGFX panel configuration and pin assignments.
 */
class DisplayHAL {
public:
    virtual ~DisplayHAL() = default;

    /** @brief False when the panel did not start; nothing can be shown then. */
    virtual bool init() = 0;
    virtual void setBrightness(uint8_t brightness) = 0;

    /** @brief Access the underlying LovyanGFX device for drawing. */
    virtual lgfx::LGFX_Device& getDevice() = 0;

    virtual int width() = 0;
    virtual int height() = 0;

    /** @brief True if the display is round (e.g. GC9A01). Useful for layout hints. */
    virtual bool isRound() = 0;
};

#endif // DISPLAY_HAL_H
