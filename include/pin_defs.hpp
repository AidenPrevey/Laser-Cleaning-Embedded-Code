#pragma once

/* -------------------------------------------------------------------------- */
/*                           PCF8676 PIN DEFINITIONS                          */
/* -------------------------------------------------------------------------- */
constexpr static uint8_t ENCODER_JAW_ROTATION_PIN1       = 5;
constexpr static uint8_t ENCODER_JAW_ROTATION_PIN2       = 6;
constexpr static uint8_t ENCODER_JAW_ROTATION_BUTTON_PIN = 7;
constexpr static uint8_t ENCODER_JAW_ROTATION_SPEED_LED  = 1;

constexpr static uint8_t ENCODER_JAW_POSITION_PIN1       = 8;
constexpr static uint8_t ENCODER_JAW_POSITION_PIN2       = 9;
constexpr static uint8_t ENCODER_JAW_POSITION_BUTTON_PIN = 10;
constexpr static uint8_t ENCODER_JAW_POSITION_SPEED_LED  = 13;

constexpr static uint8_t ENCODER_CLAMP_PIN1       = 2;
constexpr static uint8_t ENCODER_CLAMP_PIN2       = 3;
constexpr static uint8_t ENCODER_CLAMP_BUTTON_PIN = 4;
constexpr static uint8_t ENCODER_CLAMP_SPEED_LED  = 0;

constexpr static uint8_t IO_EXTENDER_INT = D10;  // Pin for PCF8575 interrupt
/* -------------------------------------------------------------------------- */
/*                                 SPI DEFINITIONS                            */
/* -------------------------------------------------------------------------- */
constexpr static uint8_t SW_MOSI = 255;  // Pin for software SPI MOSI
constexpr static uint8_t SW_MISO = 255;  // Pin for software SPI MISO
constexpr static uint8_t SW_SCK  = 255;  // Pin for software SPI SCK

constexpr static uint8_t HW_MOSI = D11;
constexpr static uint8_t HW_MISO = D12;
constexpr static uint8_t HW_SCK  = D13;

// Chip Select Pins
constexpr static uint8_t JAW_ROTATION_CS_PIN = D6;  // Pin for jaw rotation motor
constexpr static uint8_t JAW_POSITION_CS_PIN = D9;  // Pin for jaw position motor
constexpr static uint8_t CLAMP_CS_PIN        = D3;  // Pin for clamp motor
constexpr static uint8_t ENCODER_CS_PIN      = D1;  // Pin for encoder CS

/* -------------------------------------------------------------------------- */
/*                           MOTOR PIN DEFINITIONS                            */
/* -------------------------------------------------------------------------- */
constexpr static uint8_t JAW_ROTATION_STEP_PIN = D5;  // Pin for jaw rotation step
constexpr static uint8_t JAW_ROTATION_DIR_PIN  = D4;  // Pin for jaw rotation direction
constexpr static uint8_t JAW_POSITION_STEP_PIN = D8;  // Pin for jaw position step
constexpr static uint8_t JAW_POSITION_DIR_PIN  = D7;  // Pin for jaw position direction
constexpr static uint8_t CLAMP_STEP_PIN        = D2;  // Pin for clamp step
constexpr static uint8_t CLAMP_DIR_PIN         = A7;  // Pin for clamp direction

/* -------------------------------------------------------------------------- */
/*                                OTHER PINS                                  */
/* -------------------------------------------------------------------------- */
constexpr static uint8_t LIMIT_SWITCH_PIN_JAW_ROTATION = 255;  // Pin for limit switch
constexpr static uint8_t ESTOP_VSAMPLE_PIN             = 255;  // Pin for ESTOP voltage sample
constexpr static uint8_t CLAMP_POT_PIN                 = A1;   // Pin for current selection on clamp
constexpr static uint8_t ROLL_BRAKE_BUT_PIN            = 12;   // Pin for the roller brake input
constexpr static uint8_t ROLL_BRAKE_REAL_PIN           = A3;   // Pin for actual brake
constexpr static uint8_t MODE_PIN                      = 11;   // Pin for mode control
constexpr static uint8_t ESTOP_PIN                     = 255;  // Pin for emergency stop