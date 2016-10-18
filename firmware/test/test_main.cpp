// http://docs.platformio.org/en/stable/plus/unit-testing.html

// Unit test to test opperation of ATmega328 by flashing LED (pin 9) and checking output

// Requires PlatformIO 3.0

#include <Arduino.h>
#include <unity.h>
#include <SI7021.h>


SI7021 sensor;

#ifdef UNIT_TEST

 // void setUp(void) {
  // spiInit();
  // }

// void tearDown(void) {
// // clean stuff up here
// }

const byte LED =9;
const byte si7021_address=40;

void led_state_high(void) {
    digitalWrite(LED, HIGH);
    TEST_ASSERT_EQUAL(digitalRead(LED), HIGH);
}

void led_state_low(void) {
    digitalWrite(LED, LOW);
    TEST_ASSERT_EQUAL(digitalRead(LED), LOW);
}

void i2c_line_high(void){
  TEST_ASSERT_EQUAL(digitalRead(SCL), HIGH);
  TEST_ASSERT_EQUAL(digitalRead(SDA), HIGH);
}

void si7021_exists(void){
    // Scan I2C bus for Si7021
    byte found_address =0;
    byte count = 0;
    for (byte i = 1; i < 120; i++)
    {
      Wire.beginTransmission (i);
      if (Wire.endTransmission () == 0)
        {
        Serial.print (i, HEX);
        found_address = (i);
        count++;
        delay (1);
        }
    }
    TEST_ASSERT_EQUAL_HEX(found_address, si7021_address);
}

void si7021_start(void) {
    TEST_ASSERT_EQUAL(sensor.begin(), 1);
}

void si7021_read_temp(void) {
    sensor.begin();
    si7021_env data = sensor.getHumidityAndTemperature();
    int temp = data.celsiusHundredths;
    // 21.00 degC = 2100
    // delta, expected, actual
    TEST_ASSERT_INT_WITHIN(6000, 2100, temp);
}

void si7021_read_humidity(void) {
    sensor.begin();
    si7021_env data = sensor.getHumidityAndTemperature();
    int humidity = data.humidityBasisPoints;
    // 50.00 RH = 5000
    // delta, expected, actual
    TEST_ASSERT_INT_WITHIN(5000, 5000, humidity);
}



void setup() {
    UNITY_BEGIN();    // IMPORTANT LINE!

    // Test LED
    pinMode(LED, OUTPUT);
    RUN_TEST(led_state_high);
    RUN_TEST(led_state_low);

    // Test Sensor
    RUN_TEST(i2c_line_high);
    // I2C lines need to be pulled high for I2C to work
    if (digitalRead(SDA) == HIGH && digitalRead(SCL) == HIGH){
      RUN_TEST(si7021_start);
      RUN_TEST(si7021_read_temp);
      RUN_TEST(si7021_read_humidity);
    }

}


void loop() {

      UNITY_END(); // stop unit testing
}


#endif
