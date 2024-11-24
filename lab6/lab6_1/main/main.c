#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_timer.h"

// Ultrasonic sensor pin configuration
#define TRIG_PIN GPIO_NUM_4
#define ECHO_PIN GPIO_NUM_5

// I2C Configuration
#define I2C_MASTER_SCL_IO 8      // SCL on GPIO8
#define I2C_MASTER_SDA_IO 10     // SDA on GPIO10
#define I2C_MASTER_FREQ_HZ 100000  // 100kHz I2C clock
#define I2C_MASTER_NUM I2C_NUM_0  // I2C port number
#define SHTC3_ADDR 0x70          // I2C address for SHTC3 sensor

// I2C commands for SHTC3
#define WAKEUP_CMD  0x3517
#define MEASURE_CMD 0x7CA2
#define SLEEP_CMD   0xB098

// Constants
#define SPEED_OF_SOUND_BASE 331.3 // Speed of sound in m/s at 0°C
#define TEMP_COEFFICIENT 0.606    // Change in speed of sound per °C
#define TIMEOUT_US 30000          // 30ms timeout for pulse (30000us)

// Logging tag
static const char *TAG = "SR04_SHTC3";

// Function prototypes
void i2c_master_init(void);
esp_err_t write_command(uint16_t command);
esp_err_t read_sensor(float *temperature_C, float *humidity);
void ultrasonic_init(void);
float read_distance(float temperature);

void app_main(void) {
    // Initialize I2C and ultrasonic sensor
    i2c_master_init();
    ultrasonic_init();

    float temperature_C = 25.0; // Default temperature
    float humidity = 50.0;      // Default humidity
    float distance;

    while (1) {
        // Read temperature and humidity from the sensor
        esp_err_t temp_read_status = read_sensor(&temperature_C, &humidity);

        if (temp_read_status == ESP_OK) {
            // ESP_LOGI(TAG, "Temperature: %.2f°C, Humidity: %.2f%%", temperature_C, humidity);
        } else {
            // ESP_LOGE(TAG, "Failed to read sensor data, using default values.");
        }

        // Calculate distance based on the current temperature
        distance = read_distance(temperature_C);

        // Print results to the monitor
        if (distance >= 0) {
            printf("Distance: %.2f cm at %.1f°C\n", distance, temperature_C);
        } else {
            printf("Distance measurement failed at %.1f°C\n", temperature_C);
        }

        // Delay 1 second
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

// Initialize I2C with proper configuration
void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };

    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0));
    ESP_LOGI(TAG, "I2C driver installed successfully");
}

// Send I2C commands to the temperature sensor
esp_err_t write_command(uint16_t command) {
    uint8_t data[2] = {command >> 8, command & 0xFF};
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, SHTC3_ADDR, data, sizeof(data), 1000 / portTICK_PERIOD_MS);
    return ret;
}

// Read temperature and humidity from the sensor
esp_err_t read_sensor(float *temperature_C, float *humidity) {
    uint8_t data[6];
    esp_err_t ret;

    // Wake up the sensor
    ret = write_command(WAKEUP_CMD);
    if (ret != ESP_OK) return ret;
    vTaskDelay(1 / portTICK_PERIOD_MS);

    // Send measurement command
    ret = write_command(MEASURE_CMD);
    if (ret != ESP_OK) return ret;
    vTaskDelay(20 / portTICK_PERIOD_MS);

    // Read 6 bytes from the sensor
    ret = i2c_master_read_from_device(I2C_MASTER_NUM, SHTC3_ADDR, data, 6, 1000 / portTICK_PERIOD_MS);
    // ESP_LOGI(TAG, "Raw data: %02X %02X %02X %02X %02X %02X", data[0], data[1], data[2], data[3], data[4], data[5]);

    uint16_t temp_raw = (data[0] << 8) | data[1];
    uint16_t hum_raw = (data[3] << 8) | data[4];

    *temperature_C = -45 + (175.0 * (temp_raw / 65535.0));
    // ESP_LOGI(TAG, "Temp: %f", -45 + (175.0 * (temp_raw / 65535.0)));
    *humidity = 100.0 * (hum_raw / 65535.0);

    // Put the sensor to sleep
    write_command(SLEEP_CMD);
    return ret;
}

// Initialize the ultrasonic sensor
void ultrasonic_init(void) {
    gpio_config_t io_conf;

    // Configure Trigger pin as output
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = (1ULL << TRIG_PIN);
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    gpio_config(&io_conf);

    // Configure Echo pin as input
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pin_bit_mask = (1ULL << ECHO_PIN);
    gpio_config(&io_conf);

    ESP_LOGI(TAG, "Ultrasonic sensor initialized");
}

// Function to calculate distance using SR04
float read_distance(float temperature) {
    float speed_of_sound = SPEED_OF_SOUND_BASE + (TEMP_COEFFICIENT * temperature);

    // Trigger ultrasonic pulse
    gpio_set_level(TRIG_PIN, 1);
    esp_rom_delay_us(10); // 10µs pulse width
    gpio_set_level(TRIG_PIN, 0);

    // Wait for Echo signal to go HIGH
    uint64_t start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 0) {
        if ((esp_timer_get_time() - start_time) > TIMEOUT_US) {
            ESP_LOGW(TAG, "Echo timeout waiting for HIGH");
            return -1; // Timeout
        }
    }

    // Measure the HIGH duration of Echo signal
    start_time = esp_timer_get_time();
    while (gpio_get_level(ECHO_PIN) == 1) {
        if ((esp_timer_get_time() - start_time) > TIMEOUT_US) {
            ESP_LOGW(TAG, "Echo timeout waiting for LOW");
            return -1; // Timeout
        }
    }
    uint64_t duration = esp_timer_get_time() - start_time;

    // Convert duration to distance
    float distance_cm = (duration * (speed_of_sound / 10000)) / 2;

    return distance_cm;
}
