#include <stdio.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"

#define I2C_MASTER_SCL_IO 8  // SCL on GPIO8
#define I2C_MASTER_SDA_IO 10 // SDA on GPIO10
#define I2C_MASTER_FREQ_HZ 100000  // 100kHz I2C clock
#define I2C_MASTER_NUM I2C_NUM_0  // I2C port number
#define TAG "I2C_SHTC3"

#define SHTC3_ADDR 0x70  // I2C address for SHTC3 sensor

// I2C commands
#define WAKEUP_CMD  0x3517
#define MEASURE_CMD 0x7CA2
#define SLEEP_CMD   0xB098

// Initialize I2C with proper configuration
void i2c_master_init() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ  // 100kHz I2C clock
    };

    // Check and configure I2C parameters
    ESP_ERROR_CHECK(i2c_param_config(I2C_MASTER_NUM, &conf));
    ESP_LOGI(TAG, "I2C configuration set. SDA: GPIO%d, SCL: GPIO%d, Frequency: %d Hz",
             I2C_MASTER_SDA_IO, I2C_MASTER_SCL_IO, I2C_MASTER_FREQ_HZ);

    // Install the I2C driver
    ESP_ERROR_CHECK(i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0));
    ESP_LOGI(TAG, "I2C driver installed successfully");
}

// Function to send commands over I2C
void write_command(uint16_t command) {
    // command >> 8 gives MSB, command & 0xFF gives LSB
    uint8_t data[2] = { command >> 8, command & 0xFF };
    ESP_LOGI(TAG, "Sending command: 0x%04X", command);
    esp_err_t ret = i2c_master_write_to_device(
        I2C_MASTER_NUM, // I2C port number
        SHTC3_ADDR, // I2C address of the device
        data, // pointer to the data buffer to be written
        sizeof(data), // size of the data to be written
        1000 / portTICK_PERIOD_MS // timeout period
    );

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Command 0x%04X sent successfully", command);
    } else {
        ESP_LOGE(TAG, "Failed to send command 0x%04X", command);
    }
}

// Function to read temperature and humidity from the sensor
void read_sensor(float *temperature_C, float *humidity) {
    uint8_t data[6];
    
    // Wake up sensor
    write_command(WAKEUP_CMD);
    vTaskDelay(1 / portTICK_PERIOD_MS);  // small delay to ensure wakeup

    // Send measurement command
    write_command(MEASURE_CMD);
    vTaskDelay(20 / portTICK_PERIOD_MS);  // wait for measurement to complete

    // Read 6 bytes (temp MSB, temp LSB, checksum, hum MSB, hum LSB, checksum)
    esp_err_t ret = i2c_master_read_from_device(I2C_MASTER_NUM, SHTC3_ADDR, data, 6, 1000 / portTICK_PERIOD_MS);

    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Received data from sensor");

        // Calculate temperature in Celsius
        uint16_t temp_raw = (data[0] << 8) | data[1];
        *temperature_C = -45 + (175.0 * (temp_raw / 65535.0));

        // Calculate humidity
        uint16_t hum_raw = (data[3] << 8) | data[4];
        *humidity = 100.0 * (hum_raw / 65535.0);

        ESP_LOGI(TAG, "Raw temp: %u, Temp in C: %.2f", temp_raw, *temperature_C);
        ESP_LOGI(TAG, "Raw hum: %u, Humidity: %.2f", hum_raw, *humidity);
    } else {
        ESP_LOGE(TAG, "Failed to read data from sensor");
    }

    // Put sensor back to sleep
    write_command(SLEEP_CMD);
}

void app_main() {
    // Initialize I2C
    i2c_master_init();

    float temperature_C, humidity;

    // Repeatedly read sensor values every 2 seconds
    while (1) {
        read_sensor(&temperature_C, &humidity);

        // Convert temperature to Fahrenheit
        float temperature_F = (temperature_C * 9 / 5) + 32;

        // Print temperature and humidity
        printf("Temperature is %.0fC (or %.0fF) with a %.0f%% humidity\n", temperature_C, temperature_F, humidity);

        vTaskDelay(2000 / portTICK_PERIOD_MS);  // Wait 2 seconds before the next reading
    }
}