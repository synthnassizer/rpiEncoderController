/*
 * @file    rotaryEncoderClient.c
 * @author  Athanasios Silis
 * @license Public Domain
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pigpiod_if2.h>

#include "rotaryEncoderClient.h"

struct _rec_s
{
   int pi;
   const encoderPinout_t * encPins;
   recCb_t cb;
   recCb_t cbButton;
   int cb_id_a;
   int cb_id_b;
   int cb_id_button;
   unsigned levA;
   unsigned levB;
   unsigned oldState;
   unsigned glitch;
   int step;
   unsigned encId;
};

/*

             +---------+         +---------+      0
             |         |         |         |
   A         |         |         |         |
             |         |         |         |
   +---------+         +---------+         +----- 1

       +---------+         +---------+            0
       |         |         |         |
   B   |         |         |         |
       |         |         |         |
   ----+         +---------+         +---------+  1

*/

/* format of transits array (4bits) : oldStateA,oldStateB,newStateA,newStateB */
static int transits[16]=
{
/* 0000 0001 0010 0011 0100 0101 0110 0111 */
      0,  -1,   1,   0,   1,   0,   0,  -1,
/* 1000 1001 1010 1011 1100 1101 1110 1111 */
     -1,   0,   0,   1,   0,   1,  -1,   0
};

static void _cbButton(
    int pi, unsigned gpio, unsigned level, uint32_t tick, void *user)
{
    (void)pi;
    (void)tick;
    rec_t *self=user;
    
    if (level != PI_TIMEOUT)
    {
        if ((gpio == self->encPins->gpioButton) && (self->cbButton))
        {
            (self->cbButton)( !(level & 0x1), self->encId);
        }
    }
}

static void _cbEncoder(
   int pi, unsigned gpio, unsigned level, uint32_t tick, void *user)
{
    (void)pi;
    (void)tick;
    rec_t *self=user;
    unsigned newState = 0u;
    int inc = 0;

    if (level != PI_TIMEOUT)
    {
        if (gpio == self->encPins->gpioA)
            self->levA = level;
        else
            self->levB = level;

        newState = self->levA << 1u | self->levB;
        inc = transits[self->oldState << 2u | newState];

        if (inc)
        {
            self->oldState = newState;

            if (self->cb)
            {
                (self->cb)( inc * self->step, self->encId);
            }
        }
    }
}

/* PUBLIC ----------------------------------------------------------------- */

extern int recPigpioConnect(const char * host, const char * port)
{
    int pi = -1;
    char * localHost = malloc(sizeof(host)+1);
    char * localPort = malloc(sizeof(port)+1);

    if (localHost && localPort)
    {
        strcpy(localHost, host);
        strcpy(localPort, port);

        pi = pigpio_start(localHost, localPort);

        free(localHost);
        free(localPort);
    }

    return pi;
}

void recPigpioDisconnect(int pi)
{
    pigpio_stop(pi);
}

extern rec_t *recInit(const int pi, const encoderPinout_t * const encPins,
    const unsigned encoderId, const int step, const unsigned glitch, recCb_t cbFnEncoder, recCb_t cbFnButton)
{
   rec_t *self;

   self = malloc(sizeof(rec_t));

   if (!self) return NULL;

   self->pi = pi;
   self->encPins = encPins;
   self->cb = cbFnEncoder;
   self->cbButton = cbFnButton;
   self->levA=0;
   self->levB=0;
   self->step = step;
   self->encId = encoderId;

   set_mode(pi, self->encPins->gpioA, PI_INPUT);
   set_mode(pi, self->encPins->gpioB, PI_INPUT);
   set_mode(pi, self->encPins->gpioButton, PI_INPUT);

   /* pull up is needed as encoder common is grounded */

   set_pull_up_down(pi, self->encPins->gpioA, PI_PUD_UP);
   set_pull_up_down(pi, self->encPins->gpioB, PI_PUD_UP);
   set_pull_up_down(pi, self->encPins->gpioButton, PI_PUD_UP);

   self->glitch = glitch;

   set_glitch_filter(pi, self->encPins->gpioA, self->glitch);
   set_glitch_filter(pi, self->encPins->gpioB, self->glitch);

   self->oldState = (gpio_read(pi, self->encPins->gpioA) << 1) | gpio_read(pi, self->encPins->gpioB);

   /* monitor encoder level changes */

   self->cb_id_a = callback_ex(pi, self->encPins->gpioA, EITHER_EDGE, _cbEncoder, self);
   self->cb_id_b = callback_ex(pi, self->encPins->gpioB, EITHER_EDGE, _cbEncoder, self);
   self->cb_id_button = callback_ex(pi, self->encPins->gpioButton, EITHER_EDGE, _cbButton, self);

   return self;
}

extern void recReset(rec_t *self)
{
   if (self)
   {
      if (self->cb_id_a >= 0)
      {
         callback_cancel(self->cb_id_a);
         self->cb_id_a = -1;
      }

      if (self->cb_id_b >= 0)
      {
         callback_cancel(self->cb_id_b);
         self->cb_id_b = -1;
      }

      if (self->cb_id_button >= 0)
      {
         callback_cancel(self->cb_id_button);
         self->cb_id_button = -1;
      }

      set_glitch_filter(self->pi, self->encPins->gpioA, 0u);
      set_glitch_filter(self->pi, self->encPins->gpioB, 0u);

      free(self);
   }
}

extern void recSetGlitchFilter(rec_t *self, unsigned glitch)
{
    if (self->glitch != glitch)
    {
        self->glitch = glitch;
        set_glitch_filter(self->pi, self->encPins->gpioA, glitch);
        set_glitch_filter(self->pi, self->encPins->gpioB, glitch);
    }
}

