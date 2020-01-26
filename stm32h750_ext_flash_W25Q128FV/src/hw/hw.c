/*
 * hw.c
 *
 *  Created on: 2020. 1. 21.
 *      Author: Baram
 */




#include "hw.h"





void hwInit(void)
{
  bspInit();

  delayInit();
  millis();

  ledInit();

  ledOn(0);
  delay(50);
  ledOff(0);

  qspiInit();
  flashInit();
}
