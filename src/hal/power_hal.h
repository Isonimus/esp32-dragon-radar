#ifndef POWER_HAL_H
#define POWER_HAL_H

/**
 * @brief Abstract power management interface.
 * Covers hold-pin keep-alive, battery ADC, and shutdown.
 * Boards without battery/power management provide no-op stubs.
 */
class PowerHAL {
public:
    virtual ~PowerHAL() = default;

    virtual void begin() = 0;

    /** @brief Perform a clean shutdown / power-off. */
    virtual void shutdown() = 0;

    /** @brief Battery voltage in volts. Returns 0 if no battery. */
    virtual float getBatteryVoltage() = 0;

    /** @brief True if the board has battery monitoring. */
    virtual bool hasBattery() = 0;
};

#endif // POWER_HAL_H
