/*
 * @file    rotaryEncoderClient.h
 * @author  Athanasios Silis
 * @license Public Domain
 * @desc    Module uses pigpio lib in order to listen and forward rotary encoder (with button) events.
 * 
 *          Based heavily in example code "Rotary Encoder 2015-11-18"
 *          http://abyz.me.uk/rpi/pigpio/examples.html#pigpiod_if2%20code
 * 
 * @usage   For linking use "-lpigpiod_if2 -lrt" lib
 */
#ifndef ROTARYENCODERCLIENT_H
#define ROTARYENCODERCLIENT_H

#define DEFAULT_STEADY_TIME (1000u) /* glitch */

struct _rec_s;

typedef struct _rec_s rec_t;
typedef void (*recCb_t)(int, unsigned);
typedef struct encoderPinout_t
{
    unsigned gpioA;
    unsigned gpioB;
    unsigned gpioButton;
} encoderPinout_t;


/** @brief Connect to pigpiod daemon */
extern int recPigpioConnect(const char * host, const char * port);

/** @brief Disconnect from pigpiod daemon */
extern void recPigpioDisconnect(int pi);

/** @brief Initialise encoder */
extern rec_t * recInit(int pi, const encoderPinout_t * const encPins, /* BCM pin numbers*/
    const unsigned encoderId, const int step, const unsigned glitch, recCb_t cbFnEncoder, recCb_t cbFnButton);

/** @brief Reset encoder and free resources */
extern void recReset(rec_t *renc);

/** @brief Update encoder's filter
 *
 *  @details Mechanical encoders may suffer from switch bounce.
             rec_set_glitch_filter may be used to filter out edges
             shorter than glitch microseconds.  By default a glitch
             filter of 1000 microseconds is used.
 * 
 */
extern void recSetGlitchFilter(rec_t *renc, unsigned glitch);

#endif /* ROTARYENCODERCLIENT_H */

