#include "nrc_types.h"
#include "api_gpio.h"
#include "api_system.h"
#include "led_routine.h"

#define LED_PIN GPIO_18

int init_led()
{
  NRC_GPIO_CONFIG config = {
    .gpio_pin = LED_PIN,
    .gpio_mode = GPIO_PULL_DOWN,
    .gpio_dir = GPIO_OUTPUT,
    .gpio_alt = GPIO_FUNC
  };
  
  int code = nrc_gpio_config(&config);
  if(!code){
    reset_pin_on_led();
  }

  return code;
}

int set_pin_on_led()
{
  return nrc_gpio_outputb(LED_PIN,GPIO_LEVEL_HIGH);
}


int reset_pin_on_led()
{
  return nrc_gpio_outputb(LED_PIN, GPIO_LEVEL_LOW);  
}
