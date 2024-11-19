/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

const static char *TAG = "MorseDecoder";

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/
#define ADC_CHANNEL                  ADC_CHANNEL_0  // GPIO0
#define ADC_ATTEN                    ADC_ATTEN_DB_12
#define ADC_THRESHOLD                200  // Adjust this based on your setup
#define DOT_DURATION                 300000  
#define DASH_DURATION                900000  
#define SYMBOL_SPACE_DURATION        200000  
#define WORD_SPACE_DURATION          1400000 
#define LOG_INTERVAL_MS              500  // Log every 500 milliseconds

// Morse code dictionary
typedef struct {
    char symbol;
    const char *code;
} MorseCode;

static MorseCode morse_dict[] = {
    {'A', ".-"}, {'B', "-..."}, {'C', "-.-."}, {'D', "-.."}, {'E', "."},
    {'F', "..-."}, {'G', "--."}, {'H', "...."}, {'I', ".."}, {'J', ".---"},
    {'K', "-.-"}, {'L', ".-.."}, {'M', "--"}, {'N', "-."}, {'O', "---"},
    {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."}, {'S', "..."}, {'T', "-"},
    {'U', "..-"}, {'V', "...-"}, {'W', ".--"}, {'X', "-..-"}, {'Y', "-.--"},
    {'Z', "--.."}, {'1', ".----"}, {'2', "..---"}, {'3', "...--"},
    {'4', "....-"}, {'5', "....."}, {'6', "-...."}, {'7', "--..."},
    {'8', "---.."}, {'9', "----."}, {'0', "-----"}, {' ', "/"}
};

// Function to match Morse code to character
static char match_morse_code(const char *code) {
    for (int i = 0; i < sizeof(morse_dict) / sizeof(MorseCode); i++) {
        if (strcmp(morse_dict[i].code, code) == 0) {
            return morse_dict[i].symbol;
        }
    }
    return '?';  // Unknown character
}

void app_main(void)
{
    // Initialize ADC
    adc_oneshot_unit_handle_t adc_handle;
    adc_oneshot_unit_init_cfg_t init_config = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc_handle));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN,
        .bitwidth = ADC_BITWIDTH_DEFAULT,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc_handle, ADC_CHANNEL, &config));

    // Timer variables
    int adc_reading;
    int64_t signal_start = 0, signal_end = 0;
    int64_t space_start = 0;
    char morse_code[10] = "";
    char word[50] = "";  // Buffer to store the decoded word
    int morse_index = 0;
    int word_index = 0;
    int64_t last_log_time = esp_timer_get_time();
    bool signal_high = false;  // Track the signal state

    while (1) {
        ESP_ERROR_CHECK(adc_oneshot_read(adc_handle, ADC_CHANNEL, &adc_reading));

        // Log raw ADC data every 500 milliseconds
        int64_t current_time = esp_timer_get_time();
        if (current_time - last_log_time >= LOG_INTERVAL_MS * 1000) {
            // ESP_LOGI(TAG, "Raw ADC Reading: %d", adc_reading);
            last_log_time = current_time;
        }

        // Check if the signal is high or low, with debounce
        if (adc_reading > ADC_THRESHOLD && !signal_high) {
            // Signal just went high
            signal_high = true;
            signal_start = esp_timer_get_time();  // Start timing the high signal

            // Calculate the space duration
            if (space_start > 0) {
                int64_t space_duration = signal_start - space_start;
                if (space_duration >= WORD_SPACE_DURATION) {
                    ESP_LOGI(TAG, "Word space detected");

                    // Flush remaining Morse code to the word buffer
                    if (morse_index > 0) {
                        morse_code[morse_index] = '\0';
                        char decoded_char = match_morse_code(morse_code);
                        if (decoded_char != '?') {
                            word[word_index++] = decoded_char;
                        }
                        morse_index = 0;
                        memset(morse_code, 0, sizeof(morse_code));
                    }

                    word[word_index] = '\0';  // Terminate the word string
                    printf("%s ", word);  // Print the decoded word
                    word_index = 0;  // Reset for the next word
                    memset(word, 0, sizeof(word));  // Clear the word buffer
                } else if (space_duration >= SYMBOL_SPACE_DURATION) {
                    // Check for symbol space
                    morse_code[morse_index] = '\0';  // Terminate the Morse code string
                    char decoded_char = match_morse_code(morse_code);
                    if (decoded_char != '?') {
                        word[word_index++] = decoded_char;  // Add the decoded character to the word buffer
                        ESP_LOGI(TAG, "Char space detected");
                        ESP_LOGI(TAG, "Current word: %s", word);
                    } else {
                        ESP_LOGW(TAG, "Unknown Morse code sequence: %s", morse_code);
                    }
                    morse_index = 0;  // Reset for the next character
                    memset(morse_code, 0, sizeof(morse_code));  // Clear the buffer
                }
            }
        } else if (adc_reading <= ADC_THRESHOLD && signal_high) {
            // Signal just went low
            signal_high = false;
            signal_end = esp_timer_get_time();
            int64_t signal_duration = signal_end - signal_start;

            // Corrected logic for categorizing dots and dashes
            if (signal_duration >= DOT_DURATION && signal_duration < DASH_DURATION) {
                ESP_LOGI(TAG, "Dash detected");
                morse_code[morse_index++] = '-';
                ESP_LOGI(TAG, "Morse code reading: %s", morse_code);
            } else if (signal_duration < DOT_DURATION) {
                ESP_LOGI(TAG, "Dot detected");
                morse_code[morse_index++] = '.';
                ESP_LOGI(TAG, "Morse code reading: %s", morse_code);
            }

            // Start timing the low signal for space detection
            space_start = esp_timer_get_time();
        }

        // Final check for space duration to flush the last character/word
        if (space_start > 0) {
            int64_t space_duration = esp_timer_get_time() - space_start;
            if (space_duration >= WORD_SPACE_DURATION) {
                ESP_LOGI(TAG, "Word space detected");

                // Flush remaining Morse code to the word buffer
                if (morse_index > 0) {
                    morse_code[morse_index] = '\0';
                    char decoded_char = match_morse_code(morse_code);
                    if (decoded_char != '?') {
                        word[word_index++] = decoded_char;
                    }
                    morse_index = 0;
                    memset(morse_code, 0, sizeof(morse_code));
                }
                
                ESP_LOGI(TAG, "Current word (here): %s", word);
                word[word_index] = '\0';  // Terminate the word string
                printf("%s ", word);  // Print the decoded word
                ESP_LOGI(TAG, "DONE");
                word_index = 0;  // Reset for the next word
                memset(word, 0, sizeof(word));  // Clear the word buffer
                space_start = 0;  // Reset space_start to avoid repeated flushing
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));  // Poll the ADC every 10 ms
    }

    // Cleanup ADC
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc_handle));
}
