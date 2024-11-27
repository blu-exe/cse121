use esp_idf_svc::sys::link_patches;
use esp_idf_svc::log::EspLogger;
use esp_idf_svc::hal::gpio::*;
use esp_idf_svc::hal::delay::FreeRtos;
use esp_idf_svc::hal::peripherals::Peripherals;

fn main() -> anyhow::Result<()> {
    // Required to ensure runtime patches are linked
    link_patches();

    // Initialize logging
    EspLogger::initialize_default();

    // Print "Hello, world!" to the console
    log::info!("Hello, world!");

    // Access the peripherals
    let peripherals = Peripherals::take()?;
    let mut led = PinDriver::output(peripherals.pins.gpio7)?; // GPIO7 for simple LED

    // Blink LED in a loop
    loop {
        led.set_high()?; // Turn LED on
        log::info!("LED is on");
        FreeRtos::delay_ms(1000); // Wait 1 second

        led.set_low()?; // Turn LED off
        log::info!("LED is off");
        FreeRtos::delay_ms(1000); // Wait 1 second
    }
}
