idf_component_register(SRCS "main.c"
                    PRIV_REQUIRES spi_flash driver esp_hw_support esp_timer
                    INCLUDE_DIRS ".")
