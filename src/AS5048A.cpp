#include "AS5048A.hpp"

#include "Arduino.h"

static constexpr uint16_t AS5048A_CLEAR_ERROR_FLAG           = 0x0001;
static constexpr uint16_t AS5048A_PROGRAMMING_CONTROL        = 0x0003;
static constexpr uint16_t AS5048A_OTP_REGISTER_ZERO_POS_HIGH = 0x0016;
static constexpr uint16_t AS5048A_OTP_REGISTER_ZERO_POS_LOW  = 0x0017;
static constexpr uint16_t AS5048A_DIAG_AGC                   = 0x3FFD;
static constexpr uint16_t AS5048A_MAGNITUDE                  = 0x3FFE;
static constexpr uint16_t AS5048A_ANGLE                      = 0x3FFF;

static constexpr uint8_t AS5048A_AGC_FLAG                   = 0xFF;
static constexpr uint8_t AS5048A_ERROR_PARITY_FLAG          = 0x04;
static constexpr uint8_t AS5048A_ERROR_COMMAND_INVALID_FLAG = 0x02;
static constexpr uint8_t AS5048A_ERROR_FRAMING_FLAG         = 0x01;

static constexpr uint16_t AS5048A_DIAG_COMP_HIGH = 0x2000;
static constexpr uint16_t AS5048A_DIAG_COMP_LOW  = 0x1000;
static constexpr uint16_t AS5048A_DIAG_COF       = 0x0800;
static constexpr uint16_t AS5048A_DIAG_OCF       = 0x0400;

static constexpr double AS5048A_MAX_VALUE = 8191.0;

/**
 * Constructor
 */
AS5048A::AS5048A(byte cs, bool debug /*=false*/)
    : _cs(cs),
      errorFlag(false),
      ocfFlag(false),
      position(0),
      debug(debug),
      filter(filter::butterworth<3, filter::LOWPASS, float>(1000.0, 1/1000.0))
{
}

/**
 * Initialise
 * Sets up the SPI interface
 */
void AS5048A::begin()
{
    setDelay();

    // 1MHz clock (AMS should be able to accept up to 10MHz)
    this->settings = SPISettings(3000000 / 10, MSBFIRST, SPI_MODE1);

    // setup pins
    pinMode(this->_cs, OUTPUT);
    digitalWrite(this->_cs, HIGH);

    // SPI has an internal SPI-device counter, it is possible to call "begin()" from different
    // devices
    SPI.begin();
}

/**
 * Closes the SPI connection
 * SPI has an internal SPI-device counter, for each begin()-call the close() function must be called
 * exactly 1 time
 */
void AS5048A::close() { SPI.end(); }

/**
 * Utility function used to calculate even parity of an unigned 16 bit integer
 */
uint8_t AS5048A::spiCalcEvenParity(uint16_t value)
{
    uint8_t cnt = 0;

    for (uint8_t i = 0; i < 16; i++)
    {
        if (value & 0x1)
        {
            cnt++;
        }
        value >>= 1;
    }
    return cnt & 0x1;
}

/**
 * @brief  Verify the even-parity bit in a 16-bit SPI word.
 * @param  value  Raw 16-bit word read from the encoder.
 * @return 1 if parity is *good*, 0 if there is a parity error.
 */
uint8_t AS5048A::spiCheckParity(uint16_t value)
{
    uint8_t parityBit = (value >> 15) & 0x01;   // MSB
    uint16_t data     =  value & 0x7FFF;        // lower 15 bits

    /*--- Compute parity of the 15 data bits -----------------------------*/
    /* GCC & Clang: use fast builtin (returns 1 for odd number of 1-bits) */
    uint8_t dataParity = __builtin_parity(data);

    /*--- Even-parity test ----------------------------------------------*/
    /* For even parity: parityBit should equal dataParity                 */
    return (parityBit == dataParity) ? 1U : 0U;
}

/**
 * Get the rotation of the sensor relative to the zero position.
 *
 * @return {int16_t} between -2^13 and 2^13
 */
int16_t AS5048A::getRotation() { return getRotationUnwrapped() - (revCount * FULL_SCALE); }

/**
 * Get the unwrapped rotation of the sensor, referenced to `position`.
 * The result can grow beyond ±8191 by counting full revolutions.
 *
 * @return int32_t  continuous angle in raw counts (1 LSB ≈ 0.02197°)
 */
int32_t AS5048A::getRotationUnwrapped()
{
    // 1. Read the current 14-bit angle
    uint16_t raw = getRawRotation();

    // raw = filter.filterData(raw);

    if (errorFlag)
    {
        raw = prevRaw;  // If an error occurred, use the previous value
    }

    if (raw == 0)
    {
        raw = prevRaw;
    }

    if (raw >= FULL_SCALE)
    {
        raw = prevRaw;
    }

	if (!prevRawinitialized){
		prevRawinitialized = true;
		prevRaw = raw;  // Initialize prevRaw with the first read value
		revCount = -1;  // Reset revolution count on first read
	}
    // 3. Detect wrap-around
    int16_t delta = raw - prevRaw;

    if (delta > HALF_SCALE)
        --revCount;
    else if (delta < -HALF_SCALE)
        ++revCount;

    prevRaw = raw;  // store for next time

    // 4. Convert to continuous counts and subtract the user-defined zero
    const int32_t continuous = (revCount * FULL_SCALE) + raw;

    return continuous - static_cast<int32_t>(this->position) - HALF_SCALE;
}

/**
 * Returns the raw angle directly from the sensor
 */
int16_t AS5048A::getRawRotation() { return AS5048A::read(AS5048A_ANGLE); }

/**
 * Get the rotation of the sensor relative to the zero position in degrees.
 *
 * @return {double} between 0 and 360
 */

double AS5048A::getRotationInDegrees()
{
    int16_t rotation = getRotation();
    double degrees   = 360.0 * (rotation + AS5048A_MAX_VALUE) / (AS5048A_MAX_VALUE * 2.0);
    return degrees;
}

/**
 * Get the rotation of the sensor relative to the zero position in radians.
 *
 * @return {double} between 0 and 2 * PI
 */

double AS5048A::getRotationInRadians()
{
    int16_t rotation = getRotation();
    double radians   = PI * (rotation + AS5048A_MAX_VALUE) / AS5048A_MAX_VALUE;
    return radians;
}

/**
 * Get the rotation unwrapped of the sensor relative to the zero position in radians.
 *
 * @return {double}
 */

double AS5048A::getRotationUnwrappedInRadians()
{
    int32_t rotation = getRotationUnwrapped();
    double radians   = PI * (rotation + AS5048A_MAX_VALUE) / AS5048A_MAX_VALUE;
    return radians;
}

/**
 * returns the value of the state register
 * @return unsigned 16 bit integer containing flags
 */
uint16_t AS5048A::getState() { return AS5048A::read(AS5048A_DIAG_AGC); }

/**
 * Print the diagnostic register of the sensor
 */
void AS5048A::printState()
{
    if (this->debug)
    {
        uint16_t data = AS5048A::getState();
        if (AS5048A::error())
        {
            Serial.print("Error bit was set!");
        }
        Serial.println(data, BIN);
    }
}

/**
 * Returns the value used for Automatic Gain Control (Part of diagnostic
 * register)
 */
uint8_t AS5048A::getGain()
{
    uint16_t data = AS5048A::getState();
    return static_cast<uint8_t>(data & AS5048A_AGC_FLAG);
}

/**
 * Get diagnostic
 */
String AS5048A::getDiagnostic()
{
    uint16_t data = AS5048A::getState();
    if (data & AS5048A_DIAG_COMP_HIGH)
    {
        return "COMP high";
    }
    if (data & AS5048A_DIAG_COMP_LOW)
    {
        return "COMP low";
    }
    if (data & AS5048A_DIAG_COF)
    {
        return "CORDIC overflow";
    }
    if (data & AS5048A_DIAG_OCF && ocfFlag == false)
    {
        ocfFlag = true;
        return "Offset compensation finished";
    }
    return "";
}

/*
 * Get and clear the error register by reading it
 */
String AS5048A::getErrors()
{
    uint16_t error = AS5048A::read(AS5048A_CLEAR_ERROR_FLAG);
    if (error & AS5048A_ERROR_PARITY_FLAG)
    {
        return "Parity Error";
    }
    if (error & AS5048A_ERROR_COMMAND_INVALID_FLAG)
    {
        return "Command invalid";
    }
    if (error & AS5048A_ERROR_FRAMING_FLAG)
    {
        return "Framing error";
    }
    return "";
}

/*
 * Set the zero position
 */
void AS5048A::setZeroPosition(uint16_t position) { this->position = position % 0x3FFF; }

/*
 * Returns the current zero position
 */
uint16_t AS5048A::getZeroPosition() { return this->position; }

/*
 * Check if an error has been encountered.
 */
bool AS5048A::error() { return this->errorFlag; }

/*
 * Read a register from the sensor
 * Takes the address of the register as an unsigned 16 bit
 * Returns the value of the register
 */
uint16_t AS5048A::read(uint16_t registerAddress)
{
    uint16_t command = 0x4000;  // PAR=0 R/W=R

    command = command | registerAddress;

    // Add a parity bit on the the MSB
    command |= static_cast<uint16_t>(spiCalcEvenParity(command) << 0xF);

    // command &= 0x7FFF;                              // Clear bit 15
    // command |= (spiCalcEvenParity(command) << 15);  // Set bit 15 if parity is 1

    if (this->debug)
    {
        Serial.print("Read (0x");
        Serial.print(registerAddress, HEX);
        Serial.print(") with command: 0b");
        Serial.println(command, BIN);
    }

    // SPI - begin transaction
    SPI.beginTransaction(this->settings);

    // Send the command
    digitalWrite(this->_cs, LOW);
    SPI.transfer16(command);
    digitalWrite(this->_cs, HIGH);
    delayMicroseconds(1);
    // Now read the response
    digitalWrite(this->_cs, LOW);
    uint16_t response = SPI.transfer16(0x00);
    digitalWrite(this->_cs, HIGH);

    // SPI - end transaction
    SPI.endTransaction();

    if (this->debug)
    {
        Serial.print("Read returned: ");
        Serial.println(response, BIN);
    }

    // Check if the error bit is set or parity fails
    // if (response & 0x4000 || spiCheckParity(response) == 0)
    if (response & (0x01 >> 14) || spiCheckParity(response) == 0)
    {
        if (this->debug)
        {
            Serial.println("Setting error bit");
        }
        this->errorFlag = true;
    }
    else
    {
        this->errorFlag = false;
    }

    // Return the data, stripping the parity and error bits
    return response & ~0xC000;
}

/*
 * Write to a register
 * Takes the 16-bit  address of the target register and the unsigned 16 bit of data
 * to be written to that register
 * Returns the value of the register after the write has been performed. This
 * is read back from the sensor to ensure a sucessful write.
 */
uint16_t AS5048A::write(uint16_t registerAddress, uint16_t data)
{
    uint16_t command = 0x0000;  // PAR=0 R/W=W
    command |= registerAddress;

    // Add a parity bit on the the MSB
    command |= static_cast<uint16_t>(spiCalcEvenParity(command) << 0xF);

    if (this->debug)
    {
        Serial.print("Write (0x");
        Serial.print(registerAddress, HEX);
        Serial.print(") with command: 0b");
        Serial.println(command, BIN);
    }

    // SPI - begin transaction
    SPI.beginTransaction(this->settings);

    // Start the write command with the target address
    digitalWrite(this->_cs, LOW);
    SPI.transfer16(command);
    digitalWrite(this->_cs, HIGH);

    uint16_t dataToSend = 0x0000;
    dataToSend |= data;

    // Craft another packet including the data and parity
    dataToSend |= static_cast<uint16_t>(spiCalcEvenParity(dataToSend) << 0xF);

    if (this->debug)
    {
        Serial.print("Sending data to write: ");
        Serial.println(dataToSend, BIN);
    }

    // Now send the data packet
    digitalWrite(this->_cs, LOW);
    SPI.transfer16(dataToSend);
    digitalWrite(this->_cs, HIGH);

    delay(this->esp32_delay);

    digitalWrite(this->_cs, LOW);
    uint16_t response = SPI.transfer16(0x0000);
    digitalWrite(this->_cs, HIGH);

    // SPI - end transaction
    SPI.endTransaction();

    // Return the data, stripping the parity and error bits
    return response & ~0xC000;
}

/**
 * Set the delay acording to the microcontroller architecture
 */
void AS5048A::setDelay()
{
#if defined(ESP32) || defined(ARDUINO_ARCH_ESP32)
    this->esp32_delay = 50;
    if (this->debug)
    {
        Serial.println("AS5048A working with ESP32");
    }
#elif __AVR__
    this->esp32_delay = 0;
    if (this->debug)
    {
        Serial.println("AS5048A working with AVR");
    }
#else
    this->esp32_delay = 0;
    if (this->debug)
    {
        Serial.println("Device not detected");
    }
#endif
}
