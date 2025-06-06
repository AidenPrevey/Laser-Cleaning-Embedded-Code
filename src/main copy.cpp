// /**
//  * Author Teemu Mäntykallio
//  * Initializes the library and runs the stepper motor.
//  */

// #include <TMCStepper.h>

// #include "pin_defs.hpp"

// // #define EN_PIN 38    // Enable
// // #define DIR_PIN 55   // Direction
// // #define STEP_PIN 54  // Step
// // #define CS_PIN 42    // Chip select
// // #define SW_MOSI 66   // Software Master Out Slave In (MOSI)
// // #define SW_MISO 44   // Software Master In Slave Out (MISO)
// // #define SW_SCK 64    // Software Slave Clock (SCK)

// #define R_SENSE \
//     0.075f  // Match to your driver
//             // SilentStepStick series use 0.11
//             // UltiMachine Einsy and Archim2 boards use 0.2
//             // Panucatt BSD2660 uses 0.1
//             // Watterott TMC5160 uses 0.075

// // Select your stepper driver type
// // TMC2130Stepper driver = TMC2130Stepper(CS_PIN, R_SENSE); // Hardware SPI
// // TMC2130Stepper driver = TMC2130Stepper(CS_PIN, R_SENSE, SW_MOSI, SW_MISO, SW_SCK); // Software
// // SPI TMC2660Stepper driver = TMC2660Stepper(CS_PIN, R_SENSE); // Hardware SPI TMC2660Stepper
// // driver = TMC2660Stepper(CS_PIN, R_SENSE, SW_MOSI, SW_MISO, SW_SCK); TMC5160Stepper driver =
// // TMC5160Stepper(CS_PIN, R_SENSE);
// TMC5160Stepper driver = TMC5160Stepper(JAW_ROTATION_CS_PIN, R_SENSE, HW_MOSI, HW_MISO, HW_SCK);

// bool dir = true;

// void setup()
// {
//     Serial.begin(9600);
//     // while (!Serial)
//     //     ;
//     Serial.println("Start...");
//     driver.begin();           // Initiate pins and registeries
//     driver.rms_current(1200);  // Set stepper current to 600mA. The command is the same as command
//                               // TMC2130.setCurrent(600, 0.11, 0.5);
//     driver.en_pwm_mode(1);    // Enable extremely quiet stepping

//     // pinMode(EN_PIN, OUTPUT);
//     pinMode(JAW_ROTATION_STEP_PIN, OUTPUT);
//     // digitalWrite(EN_PIN, LOW);  // Enable driver in hardware

//     Serial.print("DRV_STATUS=0b");
//     Serial.println(driver.DRV_STATUS(), BIN);
// }

// void loop()
// {
//     digitalWrite(JAW_ROTATION_STEP_PIN, HIGH);
//     delayMicroseconds(1000);
//     digitalWrite(JAW_ROTATION_STEP_PIN, LOW);
//     delayMicroseconds(1000);
//     uint32_t ms               = millis();
//     static uint32_t last_time = 0;
//     // Serial.println(driver.DRV_STATUS(), HEX);
//     Serial.println(driver.GCONF(), HEX);
//     // Serial.println(driver.GSTAT(), BIN);


//     if ((ms - last_time) > 2000)
//     {
//         if (dir)
//         {
//             Serial.println("Dir -> 0");
//             driver.shaft(0);
//             // Serial.println(driver.DRV_STATUS() & (1 << 28), BIN);
//         }
//         else
//         {
//             Serial.println("Dir -> 1");
//             driver.shaft(1);
//             // Serial.println(driver.DRV_STATUS() & (1 << 28), BIN);
//         }
//         dir       = !dir;
//         last_time = ms;
//     }
// }
