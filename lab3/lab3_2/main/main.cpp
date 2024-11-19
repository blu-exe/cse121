#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "DFRobot_LCD.h"

DFRobot_LCD lcd(20, 2);


extern "C" void app_main() {

    // Create the LCD object
    printf("Initializing LCD...\n");
    lcd.init();
    printf("LCD Initialized\n");

    while (true) {
        lcd.setColor(BONNIE_BLUE);
        // Print messages to the LCD
        lcd.setCursor(0, 0); // set cursor to first line
        lcd.printstr("Hello CSE121!"); // print top line
        lcd.setCursor(0, 1); // set cursor to second line
        lcd.printstr("Huang"); // print bottom line
    }
}
