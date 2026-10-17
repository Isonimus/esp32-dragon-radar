#ifndef IMU_HAL_H
#define IMU_HAL_H

/**
 * @brief Abstract IMU interface.
 * Boards without an IMU return available() == false and heading 0.
 */
class ImuHAL {
public:
    virtual ~ImuHAL() = default;

    /**
     * @brief False when an IMU is fitted but did not start; true on boards that have none,
     * so a missing sensor and a failed one log differently.
     */
    virtual bool begin() = 0;
    virtual void update() = 0;

    /** @brief Heading in degrees [0, 360). */
    virtual float getHeading() = 0;

    /** @brief False on boards that have no IMU hardware. */
    virtual bool available() = 0;
};

#endif // IMU_HAL_H
