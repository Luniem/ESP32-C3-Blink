/* Blink Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led_strip.h"
#include "sdkconfig.h"
#include "esp_random.h"

struct rgb_state
{
    uint8_t r;
    uint8_t g;
    uint8_t b;
    int debounce;
};

static const char *TAG = "example";

/* Use project configuration menu (idf.py menuconfig) to choose the GPIO to blink,
   or you can edit the following line and set a number here.
*/
#define BLINK_GPIO CONFIG_BLINK_GPIO

static struct rgb_state rgb_states[25];

#ifdef CONFIG_BLINK_LED_STRIP

static led_strip_handle_t led_strip;

#define select_size 8
static uint8_t selectedPins[select_size];

static void init_specific_led(uint8_t pin_index)
{
    rgb_states[pin_index].r = esp_random() % 150;
    rgb_states[pin_index].g = esp_random() % 150;
    rgb_states[pin_index].b = esp_random() % 150;
}

static bool pin_already_selected(uint8_t selected_pin)
{
    uint8_t array_length = sizeof(selectedPins) / sizeof(selectedPins[0]);

    for (int i = 0; i < array_length; i++)
    {
        if (selectedPins[i] == selected_pin)
        {
            return true;
        }
    }

    return false;
}

static void pick_new_led(uint8_t index_to_remove)
{
    uint8_t selected_number;
    bool already_exists = false;
    do
    {
        selected_number = (uint8_t)(esp_random() % 25);
        already_exists = pin_already_selected(selected_number);
    } while (already_exists);

    selectedPins[index_to_remove] = selected_number;

    init_specific_led(selected_number);
}

static void blink_led(void)
{
    int i;
    for (i = 0; i < select_size; i++)
    {
        struct rgb_state *curr_state = &rgb_states[selectedPins[i]];

        led_strip_set_pixel(led_strip, selectedPins[i], curr_state->r, curr_state->g, curr_state->b);

        // start updating just after some debounce time
        if (curr_state->debounce > 0)
        {
            curr_state->debounce = curr_state->debounce - 1;
        }
        else
        {
            if (curr_state->r > 0)
            {
                curr_state->r = curr_state->r - 1;
            }

            if (curr_state->g > 0)
            {
                curr_state->g = curr_state->g - 1;
            }

            if (curr_state->b > 0)
            {
                curr_state->b = curr_state->b - 1;
            }

            if (curr_state->r == 0 && curr_state->g == 0 && curr_state->b == 0)
            {
                pick_new_led(i);
            }
        }
    }

    /* Refresh the strip to send data */
    led_strip_refresh(led_strip);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink addressable LED!");
    /* LED strip initialization with the GPIO and pixels number*/
    led_strip_config_t strip_config = {
        .strip_gpio_num = BLINK_GPIO,
        .max_leds = 25, // at least one LED on board
    };
#if CONFIG_BLINK_LED_STRIP_BACKEND_RMT
    led_strip_rmt_config_t rmt_config = {
        .resolution_hz = 10 * 1000 * 1000, // 10MHz
        .flags.with_dma = false,
    };
    ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
#elif CONFIG_BLINK_LED_STRIP_BACKEND_SPI
    led_strip_spi_config_t spi_config = {
        .spi_bus = SPI2_HOST,
        .flags.with_dma = true,
    };
    ESP_ERROR_CHECK(led_strip_new_spi_device(&strip_config, &spi_config, &led_strip));
#else
#error "unsupported LED strip backend"
#endif
    /* Set all LED off to clear all pixels */
    led_strip_clear(led_strip);
}

#elif CONFIG_BLINK_LED_GPIO

static void
blink_led(void)
{
    /* Set the GPIO level according to the state (LOW or HIGH)*/
    gpio_set_level(BLINK_GPIO, s_led_state);
}

static void configure_led(void)
{
    ESP_LOGI(TAG, "Example configured to blink GPIO LED!");

    gpio_reset_pin(BLINK_GPIO);
    /* Set the GPIO as a push/pull output */
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);
}

#else
#error "unsupported LED type"
#endif

static void init_led_states(void)
{
    for (int i = 0; i < select_size; i++)
    {
        uint8_t selected_number;
        bool already_exists = false;
        do
        {
            selected_number = (uint8_t)(esp_random() % 25);
            already_exists = pin_already_selected(selected_number);
        } while (already_exists);

        selectedPins[i] = selected_number;
    }

    int debounce = 0;
    for (int i = 0; i < select_size; i++)
    {
        init_specific_led(selectedPins[i]);
        rgb_states[selectedPins[i]].debounce = debounce;
        debounce += 30;
    }
}

void app_main(void)
{

    /* Configure the peripheral according to the LED type */
    configure_led();

    led_strip_clear(led_strip);

    ESP_LOGI(TAG, "Init of LED-States!");
    init_led_states();
    ESP_LOGI(TAG, "Finished init of LED-States!");

    while (1)
    {
        // ESP_LOGI(TAG, "Turning the LED %s!", s_led_state == true ? "ON" : "OFF");
        blink_led();
        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}