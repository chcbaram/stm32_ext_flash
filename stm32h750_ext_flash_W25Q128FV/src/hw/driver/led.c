/*
 * led.c
 *
 *  Created on: 2020. 1. 21.
 *      Author: Baram
 */




#include "led.h"


typedef struct
{
  GPIO_TypeDef *port;
  uint16_t      pin;
  GPIO_PinState on_state;
  GPIO_PinState off_state;
} led_tbl_t;


led_tbl_t led_tbl[LED_MAX_CH] =
{
  {GPIOC, GPIO_PIN_15, GPIO_PIN_SET, GPIO_PIN_RESET},
};


static bool is_init = false;


bool ledInit(void)
{
  uint32_t i;

  GPIO_InitTypeDef  gpio_init_structure = {0};


  led_tbl[0].port = GPIOC;
  led_tbl[0].pin  = GPIO_PIN_15;
  led_tbl[0].on_state = GPIO_PIN_SET;
  led_tbl[0].off_state = GPIO_PIN_RESET;

  gpio_init_structure.Mode = GPIO_MODE_OUTPUT_PP;
  gpio_init_structure.Pull = GPIO_PULLUP;
  gpio_init_structure.Speed = GPIO_SPEED_LOW;


  for (i=0; i<LED_MAX_CH; i++)
  {
    gpio_init_structure.Pin = led_tbl[i].pin;
    HAL_GPIO_Init(led_tbl[i].port, &gpio_init_structure);

    ledOff(i);
  }





  is_init = true;

  return true;
}

bool ledIsInit(void)
{
  return is_init;
}

void ledOn(uint8_t ch)
{
  if (ch < LED_MAX_CH)
  {
    HAL_GPIO_WritePin(led_tbl[ch].port, led_tbl[ch].pin, led_tbl[ch].on_state);
  }
}

void ledOff(uint8_t ch)
{
  if (ch < LED_MAX_CH)
  {
    HAL_GPIO_WritePin(led_tbl[ch].port, led_tbl[ch].pin, led_tbl[ch].off_state);
  }
}

void ledToggle(uint8_t ch)
{
  if (ch < LED_MAX_CH)
  {
    HAL_GPIO_TogglePin(led_tbl[ch].port, led_tbl[ch].pin);
  }
}
