#pragma once
#include <optional>

#include <AccelStepper.h>
#include <Arduino.h>

#include "TMCStepper.h"

/**
 * Lightweight wrapper around TMC5160Stepper + AccelStepper that
 * separates *static* hardware‑level data (pins, rsense, etc.) from
 * *dynamic* runtime‑tuned parameters (motion profile and current limits).
 *
 *  ┌────────────────────┐              ┌─────────────────────┐
 *  │  StaticConfig      │ ────► ctor ─►│  StepperMotor       │
 *  └────────────────────┘              └─────────────────────┘
 *             ▲                                     │
 *             │   apply(...)                        │ provides
 *  ┌──────────────────────────┐           ┌──────────────────────────────┐
 *  │  MotionParams /          │──────────►│   AccelStepper / Driver      │
 *  │  ElectricalParams        │           └──────────────────────────────┘
 */
class StepperMotor : public AccelStepper
{
public:
    static constexpr float TMC5160_PLUS_RSENSE = 0.022f;
    static constexpr float TMC5160_PRO_RSENSE  = 0.075f;

    struct Pins
    {
        uint8_t cs;           ///< Chip‑select (always required)
        uint8_t step;         ///< Step pin
        uint8_t dir;          ///< Direction pin
        uint8_t brake = 255;  ///< Optional brake pin, 255 = unused
        uint8_t mosi  = 255;  ///< Soft‑SPI MOSI, 255 = use HW‑SPI
        uint8_t miso  = 255;  ///< Soft‑SPI MISO
        uint8_t sck   = 255;  ///< Soft‑SPI SCK

        constexpr Pins(
            uint8_t cs_,
            uint8_t step_,
            uint8_t dir_,
            uint8_t brake_ = 255,
            uint8_t mosi_  = 255,
            uint8_t miso_  = 255,
            uint8_t sck_   = 255)
            : cs(cs_),
              step(step_),
              dir(dir_),
              brake(brake_),
              mosi(mosi_),
              miso(miso_),
              sck(sck_)
        {
        }
    };

    struct StaticConfig
    {
        Pins pins;
        float rSense;
        const char* name = nullptr;

        constexpr StaticConfig(Pins pins_, float rSense_, const char* name_)
            : pins(pins_),
              rSense(rSense_),
              name(name_)
        {
        }
    };

    /* ---------- runtime‑tuned groups ---------------------------------------- */
    struct MotionParams
    {
        float maxSpeed     = 0.0f;  ///< steps / second
        float acceleration = 0.0f;  ///< steps / second²

        constexpr MotionParams() : maxSpeed(0.0f), acceleration(0.0f) {}  // default

        constexpr MotionParams(float maxSpeed_, float acceleration_)
            : maxSpeed(maxSpeed_),
              acceleration(acceleration_)
        {
        }
    };

    struct ElectricalParams
    {
        float runCurrent_mA = 1000.0f;  ///< RMS current in mA
        uint16_t microsteps  = 16;       ///< microsteps per full step (1, 2, 4, 8, 16, 32)

        constexpr ElectricalParams() : runCurrent_mA(1000.0f) {}  // default
        constexpr ElectricalParams(float runCurrent_mA, uint16_t microsteps)
            : runCurrent_mA(runCurrent_mA),
              microsteps(microsteps)
        {
        }
    };

    struct PhysicalParams
    {
        float stepDistance = 1.0f;  ///< scale factor for position units (e.g., mm/step)

        constexpr PhysicalParams() : stepDistance(1.0f) {}  // default
        constexpr PhysicalParams(float stepDistance_) : stepDistance(stepDistance_) {}
    };

    explicit StepperMotor(const StaticConfig& cfg);

    int begin();
    void kill();

    // Setters
    void apply(const MotionParams& p);
    void apply(const ElectricalParams& p);
    void apply(const PhysicalParams& p);

    float currentPositionUnits() { return currentPosition() * phys_.stepDistance; }
    void moveToUnits(float pos) { moveTo(pos / phys_.stepDistance); }
    void setSpeedUnits(float speed) { setSpeed(speed / phys_.stepDistance); }

    // getters
    const char* getName() const { return cfg_.name; }
    int getMicrosteps() const { return elec_.microsteps; }

    void printDriverDebug() { Serial.println(stepper_driver_.DRV_STATUS(), BIN); }

    MotionParams getMotionParams() const { return motion_; }
    ElectricalParams getElectricalParams() const { return elec_; }
    PhysicalParams getPhysicalParams() const { return phys_; }

private:
    StaticConfig cfg_;
    MotionParams motion_;
    ElectricalParams elec_;
    PhysicalParams phys_;

    /* driver instance (soft‑SPI vs HW‑SPI picked at run‑time) */
    uint8_t BrakePin;                // Pin used to brake the motor
    TMC5160Stepper stepper_driver_;  // The wrapped driver instance

    bool BrakeOn = LOW;      // Define which direction for the pin to activate the break.
    char* name_  = nullptr;  // The name of the motor, used for debugging
};
