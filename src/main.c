#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pico/stdlib.h>
#include <hardware/i2c.h>

#define I2C_PORT i2c0
#define SDA_PIN 16
#define SCL_PIN 17
#define EEPROM_ADDR 0x50

// LED pins
const uint LEFT_LED = 20;
const uint MIDDLE_LED = 21;
const uint RIGHT_LED = 22;

// Button pins
const uint LEFT_LED_BUTTON = 9;
const uint MIDDLE_LED_BUTTON = 8;
const uint RIGHT_LED_BUTTON = 7;

// LED states
bool left_led_state = false;
bool middle_led_state = false;
bool right_led_state = false;

// Button states
bool middle_button_last_state = true;
bool left_button_last_state = true;
bool right_button_last_state = true;

uint8_t program_start_time;


void init_all(void) {
    // Initialize LED pins
    gpio_init(LEFT_LED);
    gpio_set_dir(LEFT_LED, GPIO_OUT);
    gpio_put(LEFT_LED, left_led_state);

    gpio_init(MIDDLE_LED);
    gpio_set_dir(MIDDLE_LED, GPIO_OUT);
    gpio_put(MIDDLE_LED, middle_led_state);

    gpio_init(RIGHT_LED);
    gpio_set_dir(RIGHT_LED, GPIO_OUT);
    gpio_put(RIGHT_LED, right_led_state);

    // Initialize button pins
    gpio_init(LEFT_LED_BUTTON);
    gpio_set_dir(LEFT_LED_BUTTON, GPIO_IN);
    gpio_pull_up(LEFT_LED_BUTTON);

    gpio_init(MIDDLE_LED_BUTTON);
    gpio_set_dir(MIDDLE_LED_BUTTON, GPIO_IN);
    gpio_pull_up(MIDDLE_LED_BUTTON);

    gpio_init(RIGHT_LED_BUTTON);
    gpio_set_dir(RIGHT_LED_BUTTON, GPIO_IN);
    gpio_pull_up(RIGHT_LED_BUTTON);

    // Initialize I2C
    i2c_init(I2C_PORT, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);

    // Initialize stdio
    stdio_init_all();
}

// EEPROM single byte write
void single_write_eeprom(int addr, int data) {
    uint8_t data_to_write[3];
    data_to_write[0] = (addr >> 8) & 0xFF;
    data_to_write[1] = addr & 0xFF;
    data_to_write[2] = data;

    int result = i2c_write_blocking(I2C_PORT, EEPROM_ADDR, data_to_write, 3, false);
    if (result > 0) {
        printf("EEPROM write done\n");
    } else {
        printf("Write failed\n");
    }
}

// EEPROM single byte read
int single_read_eeprom(int addr) {
    uint8_t addr_to_write[2];
    addr_to_write[0] = (addr >> 8) & 0xFF;
    addr_to_write[1] = addr & 0xFF;

    uint8_t returned_data[1];

    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, addr_to_write, 2, true);
    sleep_ms(10);
    int result = i2c_read_blocking(I2C_PORT, EEPROM_ADDR, returned_data, 1, false);

    if (result > 0) {
        printf("EEPROM read done\n");
        return returned_data[0];
    } else {
        printf("Read failed\n");
        return -1;
    }
}

// EEPROM multi-byte write
int multi_write_eeprom(int addr, const uint8_t *data, const int length) {
    uint8_t *data_to_write = malloc(length + 2);
    if (!data_to_write) {
        printf("Memory allocation error\n");
        return -1;
    }

    data_to_write[0] = (addr >> 8) & 0xFF;
    data_to_write[1] = addr & 0xFF;
    memcpy(&data_to_write[2], data, length);

    int result = i2c_write_blocking(I2C_PORT, EEPROM_ADDR, data_to_write, length + 2, false);
    free(data_to_write);

    if (result > 0) {
        printf("Multi-byte Write Successful\n");
        return 1;
    } else {
        printf("Multi-byte Write Failed\n");
        return 0;
    }
}

// EEPROM multi-byte read
int multi_read_eeprom(int addr, uint8_t *buffer, int length) {
    uint8_t addr_to_write[2];
    addr_to_write[0] = (addr >> 8) & 0xFF;
    addr_to_write[1] = addr & 0xFF;

    i2c_write_blocking(I2C_PORT, EEPROM_ADDR, addr_to_write, 2, true);
    sleep_ms(10);
    int result = i2c_read_blocking(I2C_PORT, EEPROM_ADDR, buffer, length, false);

    if (result > 0) {
        printf("Multi-byte Read Successful\n");
        return 1;
    } else {
        printf("Multi-byte Read Failed\n");
        return 0;
    }
}

// Store LED states to EEPROM
void store_led_states_to_eeprom(void) {
    // Calculate the normal LED state (bitwise OR for all LEDs)
    const uint8_t led_state = (left_led_state << 2) | (middle_led_state << 1) | right_led_state;
    // Calculate the inverted LED state by flipping all bits (bitwise NOT)
    const uint8_t inverted_led_state = ~led_state;
    // Store both the normal and inverted states to EEPROM.
    single_write_eeprom(0x00, led_state);
    sleep_ms(10);
    single_write_eeprom(0x01, inverted_led_state); // Store inverted state at the next address (0x01)
    sleep_ms(10);
}


// Restore LED states from EEPROM
typedef struct ledstate {
    uint8_t state;
    uint8_t not_state;
} ledstate;

// Function to check if the LED state is valid (normal state matches inverted state)
bool led_state_is_valid(ledstate const *ls) {
    return ls->state == (uint8_t) ~ls->not_state;
}

void print_led_state_change(void) {
    // Get the current time in microseconds and convert it to seconds
    uint64_t current_time_us = time_us_64();
    uint64_t time_since_start = current_time_us - program_start_time; // Time since program started
    uint64_t time_in_seconds = time_since_start / 1000000; // Convert microseconds to seconds

    // Print the LED state along with the time since the program started
    printf("At %llu seconds, LED states are - Left: %s, Middle: %s, Right: %s\n",
           time_in_seconds,
           left_led_state ? "ON" : "OFF",
           middle_led_state ? "ON" : "OFF",
           right_led_state ? "ON" : "OFF");
}

void restore_led_states_from_eeprom(void) {
    // Read the stored normal LED state from EEPROM (0x00)
    int stored_led_state = single_read_eeprom(0x00);
    // Read the stored inverted LED state from EEPROM (0x01)
    sleep_ms(5);
    int stored_inverted_state = single_read_eeprom(0x01);
    sleep_ms(5);

    // Check if both reads were successful
    if (stored_led_state != -1 && stored_inverted_state != -1) {
        // Create ledstate struct to hold the normal and inverted states
        ledstate ls;
        ls.state = stored_led_state;
        ls.not_state = stored_inverted_state;

        // Validate if the state is correct (normal matches inverted)
        if (led_state_is_valid(&ls)) {
            // Extract the LED states from the stored value
            bool new_left_led_state = (stored_led_state >> 2) & 0x01;
            bool new_middle_led_state = (stored_led_state >> 1) & 0x01;
            bool new_right_led_state = stored_led_state & 0x01;

            // Update the global variables only if there is a change in the state
            if (new_left_led_state != left_led_state) {
                left_led_state = new_left_led_state;
                gpio_put(LEFT_LED, left_led_state); // Update the LED pin
            }

            if (new_middle_led_state != middle_led_state) {
                middle_led_state = new_middle_led_state;
                gpio_put(MIDDLE_LED, middle_led_state); // Update the LED pin
            }

            if (new_right_led_state != right_led_state) {
                right_led_state = new_right_led_state;
                gpio_put(RIGHT_LED, right_led_state); // Update the LED pin
            }
        } else {
            // If validation fails (corrupted or invalid state), set default values
            printf("Invalid LED state detected, setting default values\n");
            left_led_state = false;
            middle_led_state = true;
            right_led_state = false;
            gpio_put(LEFT_LED, left_led_state); // Update the LED pin
            gpio_put(MIDDLE_LED, middle_led_state); // Update the LED pin
            gpio_put(RIGHT_LED, right_led_state); // Update the LED pin
        }
    } else {
        // If reading from EEPROM fails, set default values
        printf("Failed to read LED state from EEPROM, setting default values\n");
        left_led_state = false;
        middle_led_state = true;
        right_led_state = false;
        gpio_put(LEFT_LED, left_led_state); // Update the LED pin
        gpio_put(MIDDLE_LED, middle_led_state); // Update the LED pin
        gpio_put(RIGHT_LED, right_led_state); // Update the LED pin
    }
}

void toggle_led(uint led_pin, bool *led_state) {
    *led_state = !(*led_state);
    gpio_put(led_pin, *led_state);
    printf("LED on pin %u is now %s\n", led_pin, (*led_state ? "ON" : "OFF"));
    // Store updated LED states to EEPROM
    store_led_states_to_eeprom();
    print_led_state_change();
}

void handle_button(uint button_pin, bool *last_state, uint led_pin, bool *led_state) {
    uint8_t current_state = gpio_get(button_pin); // Read button state
    if (!current_state && *last_state) {
        toggle_led(led_pin, led_state);
        store_led_states_to_eeprom();
    }
    *last_state = current_state; // Update last state
}


int main() {
    init_all(); // Initialize all hardware
    restore_led_states_from_eeprom();
    while (1) {
        const uint8_t program_start_time = time_us_64();
        handle_button(LEFT_LED_BUTTON, &left_button_last_state, LEFT_LED, &left_led_state);
        handle_button(MIDDLE_LED_BUTTON, &middle_button_last_state, MIDDLE_LED, &middle_led_state);
        handle_button(RIGHT_LED_BUTTON, &right_button_last_state, RIGHT_LED, &right_led_state);
        sleep_ms(50); // Delay for debouncing
    }
}
