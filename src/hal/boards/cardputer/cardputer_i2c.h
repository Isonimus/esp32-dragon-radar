#ifndef CARDPUTER_I2C_H
#define CARDPUTER_I2C_H

#include <Arduino.h>

/**
 * @brief The Cardputer ADV's internal I2C bus (G8 SDA, G9 SCL).
 *
 * The keyboard controller, audio codec and IMU all sit on it, so it is
 * started in one place instead of by whichever driver happens to run first.
 */
namespace cardputer_i2c {

/** @brief Start the bus once; later calls are no-ops. Returns false on failure. */
bool begin();

/** @brief Write one 8-bit register. Returns false if the device did not ACK. */
bool writeRegister(uint8_t deviceAddress, uint8_t reg, uint8_t value);

/** @brief Read one 8-bit register into @p value. Returns false on any bus error. */
bool readRegister(uint8_t deviceAddress, uint8_t reg, uint8_t& value);

} // namespace cardputer_i2c

#endif // CARDPUTER_I2C_H
