// http://docs.platformio.org/en/stable/plus/unit-testing.html

// Unit test to test opperation of ATmega328 by flashing LED (pin 9) and checking output

// Requires PlatformIO 3.0

#include <Arduino.h>
#include <unity.h>
#include <SI7021.h>
SI7021 SI7021_sensor_test;

#ifdef UNIT_TEST

// void setUp(void) {
// // set stuff up here
// }

// void tearDown(void) {
// // clean stuff up here
// }

const byte LED =9;

void test_9_pin_number(void) {
    TEST_ASSERT_EQUAL(LED, LED);
}

void test_led_state_high(void) {
    digitalWrite(LED, HIGH);
    TEST_ASSERT_EQUAL(digitalRead(LED), HIGH);
}

void test_led_state_low(void) {
    digitalWrite(LED, LOW);
    TEST_ASSERT_EQUAL(digitalRead(LED), LOW);
}


void test_si7021(void) {
    TEST_ASSERT_EQUAL(SI7021_sensor_test.begin(), 1);
}




void setup() {
    UNITY_BEGIN();    // IMPORTANT LINE!
    RUN_TEST(test_9_pin_number);
    pinMode(LED, OUTPUT);
    RUN_TEST(test_si7021);
}

uint8_t i = 0;
uint8_t max_blinks = 5;

void loop() {
    if (i < max_blinks)
    {
        RUN_TEST(test_led_state_high);
        delay(500);
        RUN_TEST(test_led_state_low);
        delay(500);
        i++;
    }
    else if (i == max_blinks) {
      UNITY_END(); // stop unit testing
    }
}

#endif
