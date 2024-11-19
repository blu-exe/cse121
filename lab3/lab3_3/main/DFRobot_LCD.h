#ifndef __DFRobot_LCD_H__
#define __DFRobot_LCD_H__

#include <inttypes.h>
#include "driver/i2c.h"

// LCD device I2C addresses
#define LCD_ADDRESS     0x3E
#define RGB_ADDRESS     0x2D

// color definitions
#define WHITE           0
#define RED             1
#define GREEN           2
#define BLUE            3
#define BONNIE_BLUE     4
#define ONLY            3

#define REG_RED         0x01        // pwm2
#define REG_GREEN       0x02        // pwm1
#define REG_BLUE        0x03        // pwm0
#define REG_ONLY        0x01

#define REG_MODE1       0x00
#define REG_MODE2       0x01
#define REG_OUTPUT      0x08

// command definitions
#define LCD_CLEARDISPLAY 0x01
#define LCD_RETURNHOME 0x02
#define LCD_ENTRYMODESET 0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_CURSORSHIFT 0x10
#define LCD_FUNCTIONSET 0x20
#define LCD_SETCGRAMADDR 0x40
#define LCD_SETDDRAMADDR 0x80

// flags for display entry mode
#define LCD_ENTRYRIGHT 0x00
#define LCD_ENTRYLEFT 0x02
#define LCD_ENTRYSHIFTINCREMENT 0x01
#define LCD_ENTRYSHIFTDECREMENT 0x00

// flags for display on/off control
#define LCD_DISPLAYON 0x04
#define LCD_DISPLAYOFF 0x00
#define LCD_CURSORON 0x02
#define LCD_CURSOROFF 0x00
#define LCD_BLINKON 0x01
#define LCD_BLINKOFF 0x00

// flags for display/cursor shift
#define LCD_DISPLAYMOVE 0x08
#define LCD_CURSORMOVE 0x00
#define LCD_MOVERIGHT 0x04
#define LCD_MOVELEFT 0x00

// flags for function set
#define LCD_8BITMODE 0x10
#define LCD_4BITMODE 0x00
#define LCD_2LINE 0x08
#define LCD_1LINE 0x00
#define LCD_5x10DOTS 0x04
#define LCD_5x8DOTS 0x00

class DFRobot_LCD {
public:
    DFRobot_LCD(uint8_t lcd_cols, uint8_t lcd_rows, uint8_t lcd_Addr = LCD_ADDRESS, uint8_t RGB_Addr = RGB_ADDRESS);
    
    void init();
    void clear();
    void home();
    void noDisplay();
    void display();
    void stopBlink();
    void blink();
    void noCursor();
    void cursor();
    void scrollDisplayLeft();
    void scrollDisplayRight();
    void leftToRight();
    void rightToLeft();
    void noAutoscroll();
    void autoscroll();
    void customSymbol(uint8_t, uint8_t[]);
    void setCursor(uint8_t, uint8_t);
    void setRGB(uint8_t r, uint8_t g, uint8_t b);
    void setColor(uint8_t color);
    void setColorAll();
    void blinkLED();
    void noBlinkLED();
    virtual size_t write(uint8_t);
    void command(uint8_t);
    
    void blink_on();
    void blink_off();
    void cursor_on();
    void cursor_off();
    void setBacklight(uint8_t new_val);
    void load_custom_character(uint8_t char_num, uint8_t *rows);
    void printstr(const char c[]);
    
    uint8_t status();
    void setContrast(uint8_t new_val);
    uint8_t keypad();
    void setDelay(int, int);
    void on();
    void off();
    uint8_t init_bargraph(uint8_t graphtype);
    void draw_horizontal_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end);
    void draw_vertical_graph(uint8_t row, uint8_t column, uint8_t len,  uint8_t pixel_col_end);
    
private:
    void begin(uint8_t cols, uint8_t rows, uint8_t charsize = LCD_5x8DOTS);
    void send(uint8_t *data, uint8_t len);
    void setReg(uint8_t addr, uint8_t data);

    uint8_t _showfunction;
    uint8_t _showcontrol;
    uint8_t _showmode;
    uint8_t _initialized;
    uint8_t _numlines, _currline;
    uint8_t _lcdAddr;
    uint8_t _RGBAddr;
    uint8_t _cols;
    uint8_t _rows;
    uint8_t _backlightval;
};

// Declare i2c_master_init so it can be used in other files
void i2c_master_init();

#endif // __DFRobot_LCD_H__
