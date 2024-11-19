#include "driver/i2c.h"
#include "esp_log.h"
#include <stdint.h>
#include <string.h>

#define I2C_MASTER_NUM I2C_NUM_0
#define I2C_MASTER_SDA_IO 10
#define I2C_MASTER_SCL_IO 8
#define I2C_MASTER_FREQ_HZ 100000

#define ICM42670_ADDR 0x68
#define PWR_MGMT0 0x1F
#define ACCEL_CONFIG0 0x21
#define ACCEL_CONFIG1 0x24
#define ACCEL_DATA_X1 0x0B
#define ACCEL_DATA_X0 0x0C
#define ACCEL_DATA_Y1 0x0D
#define ACCEL_DATA_Y0 0x0E

#define THRESHOLD 1000

static const char *TAG = "TiltDetection";

void i2c_master_init() {
    i2c_config_t config = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    i2c_param_config(I2C_MASTER_NUM, &config);
    i2c_driver_install(I2C_MASTER_NUM, config.mode, 0, 0, 0);
}

// write a byte to a register
esp_err_t i2c_write_byte(uint8_t reg, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ICM42670_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

// read a single byte from a register
esp_err_t i2c_read_byte(uint8_t reg, uint8_t *data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ICM42670_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (ICM42670_ADDR << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd, data, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

void configure_icm42670() {
    // set accelerometer to low-noise mode and disable gyro
    i2c_write_byte(PWR_MGMT0, 0x0B);
    
    // set accel FSR to ±4g and ODR to 100Hz
    i2c_write_byte(ACCEL_CONFIG0, 0x29);
    
    // set accel filter bandwidth to 73Hz
    i2c_write_byte(ACCEL_CONFIG1, 0x03);
}

void app_main() {
    i2c_master_init();
    configure_icm42670();

    while (1) {
        uint8_t x_high, x_low, y_high, y_low;
        int16_t x, y;
        char direction[20] = "";  // buffer to accumulate directions

        // read high and low bytes for X-axis
        i2c_read_byte(ACCEL_DATA_X1, &x_high);
        i2c_read_byte(ACCEL_DATA_X0, &x_low);
        x = (int16_t)((x_high << 8) | x_low);

        // read high and low bytes for Y-axis
        i2c_read_byte(ACCEL_DATA_Y1, &y_high);
        i2c_read_byte(ACCEL_DATA_Y0, &y_low);
        y = (int16_t)((y_high << 8) | y_low);

        // determine direction based on thresholds
        if (y > THRESHOLD) {
            strcat(direction, "UP ");
        } else if (y < -THRESHOLD) {
            strcat(direction, "DOWN ");
        }
        if (x > THRESHOLD) {
            strcat(direction, "RIGHT");
        } else if (x < -THRESHOLD) {
            strcat(direction, "LEFT");
        }

        // Print the direction or "FLAT" if no tilt is detected
        if (strlen(direction) > 0) {
            ESP_LOGI(TAG, "%s", direction);
        } else {
            ESP_LOGI(TAG, "FLAT");
        }

        // Wait for 250 milliseconds
        vTaskDelay(250 / portTICK_PERIOD_MS);
    }
}
