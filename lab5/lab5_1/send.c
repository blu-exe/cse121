#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Default speed for Morse code (dot duration in seconds)
#define DEFAULT_SPEED 0.2

// Morse code dictionary
typedef struct {
    char character;
    const char *code;
} MorseCode;

static MorseCode morse_dict[] = {
    {'A', ".-"},   {'B', "-..."}, {'C', "-.-."}, {'D', "-.."},  {'E', "."},
    {'F', "..-."}, {'G', "--."},  {'H', "...."}, {'I', ".."},   {'J', ".---"},
    {'K', "-.-"},  {'L', ".-.."}, {'M', "--"},   {'N', "-."},   {'O', "---"},
    {'P', ".--."}, {'Q', "--.-"}, {'R', ".-."},  {'S', "..."},  {'T', "-"},
    {'U', "..-"},  {'V', "...-"}, {'W', ".--"},  {'X', "-..-"}, {'Y', "-.--"},
    {'Z', "--.."}, {'1', ".----"},{'2', "..---"},{'3', "...--"},{'4', "....-"},
    {'5', "....."},{'6', "-...."},{'7', "--..."},{'8', "---.."},{'9', "----."},
    {'0', "-----"},{' ', "/"}
};

// Lookup Morse code for a character
const char *lookup_morse_code(char c) {
    for (int i = 0; i < sizeof(morse_dict) / sizeof(MorseCode); i++) {
        if (morse_dict[i].character == c) {
            return morse_dict[i].code;
        }
    }
    return NULL;  // Character not found
}

// Send a dot
void send_dot(float speed) {
    // Placeholder for turning the LED on
    printf("[DOT] LED ON\n");
    usleep((int)(speed * 1000000));  // Convert seconds to microseconds
    // Placeholder for turning the LED off
    printf("[DOT] LED OFF\n");
    usleep((int)(speed * 1000000));  // Space between symbols
}

// Send a dash
void send_dash(float speed) {
    // Placeholder for turning the LED on
    printf("[DASH] LED ON\n");
    usleep((int)(3 * speed * 1000000));  // 3x longer for dash
    // Placeholder for turning the LED off
    printf("[DASH] LED OFF\n");
    usleep((int)(speed * 1000000));  // Space between symbols
}

// Send Morse code for a message
void send_morse_code(const char *message, float speed) {
    for (int i = 0; i < strlen(message); i++) {
        char c = message[i];
        const char *code = lookup_morse_code(c);
        if (code) {
            for (int j = 0; code[j] != '\0'; j++) {
                if (code[j] == '.') {
                    send_dot(speed);
                } else if (code[j] == '-') {
                    send_dash(speed);
                }
            }
            usleep((int)(3 * speed * 1000000));  // Space between letters
        } else if (c == '/') {
            usleep((int)(7 * speed * 1000000));  // Space between words
        }
    }
}

int main(int argc, char **argv) {
    if (argc < 3) {
        printf("Usage: %s <repeat_count> '<message>' [speed]\n", argv[0]);
        return 1;
    }

    int repeat_count = atoi(argv[1]);
    const char *message = argv[2];
    float speed = (argc > 3) ? atof(argv[3]) : DEFAULT_SPEED;  // Use default speed if not provided

    for (int i = 0; i < repeat_count; i++) {
        send_morse_code(message, speed);
        usleep((int)(2 * speed * 1000000));  // Space between repetitions
    }

    // Placeholder for GPIO cleanup
    printf("Cleanup GPIO\n");

    return 0;
}