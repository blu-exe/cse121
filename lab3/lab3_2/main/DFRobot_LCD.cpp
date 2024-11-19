/*!
 * @file DFRobot_LCD.cpp
 * @brief Modified for ESP32 using ESP-IDF
 */

#include <stdio.h>
#include <string.h>
#include <inttypes.h>
#include "DFRobot_LCD.h"
#include "driver/i2c.h"
#include "esp_log.h"

// Constants
#define I2C_MASTER_PORT I2C_NUM_0  // change if using a different I2C port

const uint8_t color_define[5][3] = {
    {255, 255, 255},  // white
    {255, 0, 0},      // red   
    {0, 255, 0},      // green
    {0, 0, 255},      // blue
    {81, 201, 245}    // bonnie blue
};

DFRobot_LCD::DFRobot_LCD(uint8_t lcd_cols, uint8_t lcd_rows, uint8_t lcd_Addr, uint8_t RGB_Addr) {
    _lcdAddr = lcd_Addr;
    _RGBAddr = RGB_Addr;
    _cols = lcd_cols;
    _rows = lcd_rows;
}

// void i2c_master_init() {
//     i2c_config_t conf;
//     conf.mode = I2C_MODE_MASTER;
//     conf.sda_io_num = GPIO_NUM_10;
//     conf.scl_io_num = GPIO_NUM_8;
//     conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
//     conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
//     conf.master.clk_speed = 100000;
//     // conf.master.clk_speed = 50000;


//     i2c_param_config(I2C_NUM_0, &conf);
//     esp_err_t err = i2c_param_config(I2C_MASTER_PORT, &conf);
//     if (err != ESP_OK) {
//         printf("I2C configuration error: %s\n", esp_err_to_name(err));
//     }

//     err = i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
//     if (err != ESP_OK) {
//         printf("I2C driver install error: %s\n", esp_err_to_name(err));
//     }
// }

void DFRobot_LCD::init() {
    // i2c_master_init();

    // original master_init()
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = 10;
    conf.scl_io_num = 8;
    conf.sda_pullup_en = GPIO_PULLUP_ENABLE;
    conf.scl_pullup_en = GPIO_PULLUP_ENABLE;
    conf.master.clk_speed = 100000;
    conf.clk_flags = 0;

    i2c_param_config(I2C_NUM_0, &conf);

    esp_err_t err = i2c_param_config(I2C_MASTER_PORT, &conf);
    if (err != ESP_OK) {
        printf("I2C configuration error: %s\n", esp_err_to_name(err));
    }

    err = i2c_driver_install(I2C_MASTER_PORT, conf.mode, 0, 0, 0);
    if (err != ESP_OK) {
        printf("I2C driver install error: %s\n", esp_err_to_name(err));
    }

    _showfunction = LCD_4BITMODE | LCD_1LINE | LCD_5x8DOTS;
    begin(_cols, _rows);
}

void DFRobot_LCD::clear() {
    command(LCD_CLEARDISPLAY);
    vTaskDelay(2 / portTICK_PERIOD_MS);
}

void DFRobot_LCD::home() {
    command(LCD_RETURNHOME);
    vTaskDelay(2 / portTICK_PERIOD_MS);
}

void DFRobot_LCD::noDisplay() {
    _showcontrol &= ~LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

void DFRobot_LCD::display() {
    _showcontrol |= LCD_DISPLAYON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

void DFRobot_LCD::stopBlink() {
    _showcontrol &= ~LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

void DFRobot_LCD::blink() {
    _showcontrol |= LCD_BLINKON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

void DFRobot_LCD::noCursor() {
    _showcontrol &= ~LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

void DFRobot_LCD::cursor() {
    _showcontrol |= LCD_CURSORON;
    command(LCD_DISPLAYCONTROL | _showcontrol);
}

void DFRobot_LCD::scrollDisplayLeft(void) {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVELEFT);
}

void DFRobot_LCD::scrollDisplayRight(void) {
    command(LCD_CURSORSHIFT | LCD_DISPLAYMOVE | LCD_MOVERIGHT);
}

void DFRobot_LCD::leftToRight(void) {
    _showmode |= LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::rightToLeft(void) {
    _showmode &= ~LCD_ENTRYLEFT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::noAutoscroll(void) {
    _showmode &= ~LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::autoscroll(void) {
    _showmode |= LCD_ENTRYSHIFTINCREMENT;
    command(LCD_ENTRYMODESET | _showmode);
}

void DFRobot_LCD::customSymbol(uint8_t location, uint8_t charmap[]) {
    location &= 0x7;  // we only have 8 locations 0-7
    command(LCD_SETCGRAMADDR | (location << 3));

    uint8_t data[9];
    data[0] = 0x40;
    for (int i = 0; i < 8; i++) {
        data[i + 1] = charmap[i];
    }
    send(data, 9);
}

void DFRobot_LCD::setCursor(uint8_t col, uint8_t row) {
    col = (row == 0 ? col | 0x80 : col | 0xc0);
    uint8_t data[2] = {0x80, col};
    send(data, 2);
}

void DFRobot_LCD::setRGB(uint8_t r, uint8_t g, uint8_t b) {
    setReg(REG_RED, r);
    setReg(REG_GREEN, g);
    setReg(REG_BLUE, b);
}

void DFRobot_LCD::setColor(uint8_t color) {
    if (color > 4) return;
    setRGB(color_define[color][0], color_define[color][1], color_define[color][2]);
}

void DFRobot_LCD::blinkLED(void) {
    setReg(0x07, 0x17);
    setReg(0x06, 0x7f);
}

void DFRobot_LCD::noBlinkLED(void) {
    setReg(0x07, 0x00);
    setReg(0x06, 0xff);
}

inline size_t DFRobot_LCD::write(uint8_t value) {
    uint8_t data[3] = {0x40, value};
    send(data, 2);
    return 1;  // assume success
}

inline void DFRobot_LCD::command(uint8_t value) {
    uint8_t data[2] = {0x80, value};
    send(data, 2);
}

void DFRobot_LCD::begin(uint8_t cols, uint8_t lines, uint8_t dotsize) {
    if (lines > 1) {
        _showfunction |= LCD_2LINE;
    }
    vTaskDelay(50 / portTICK_PERIOD_MS);

    command(LCD_FUNCTIONSET | _showfunction);
    vTaskDelay(5 / portTICK_PERIOD_MS);
    command(LCD_FUNCTIONSET | _showfunction);
    vTaskDelay(5 / portTICK_PERIOD_MS);

    display();
    clear();
    _showmode = LCD_ENTRYLEFT | LCD_ENTRYSHIFTDECREMENT;
    command(LCD_ENTRYMODESET | _showmode);

    setColor(WHITE);
}

void DFRobot_LCD::send(uint8_t *data, uint8_t len) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_lcdAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write(cmd, data, len, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
}

void DFRobot_LCD::setReg(uint8_t addr, uint8_t data) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (_RGBAddr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, addr, true);
    i2c_master_write_byte(cmd, data, true);
    i2c_master_stop(cmd);
    i2c_master_cmd_begin(I2C_MASTER_PORT, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
}

void DFRobot_LCD::printstr(const char c[]) {
    for (int i = 0; c[i] != '\0'; i++) {
        write(c[i]);
    }
}