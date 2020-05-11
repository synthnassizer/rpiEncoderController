/*
 * @file    jackMidiOutClient.h
 * @author  Athanasios Silis
 * @license Public Domain
 * @desc    Module uses jack lib in order to register the encoders as jack client and forward their events as midi msgs.
 * 
 *          Based heavily in example code "Rotary Encoder 2015-11-18"
 *          http://abyz.me.uk/rpi/pigpio/examples.html#pigpiod_if2%20code
 * 
 * @usage   For linking use "-pthread -ljack" lib
 */

#include <inttypes.h>

#define MIDI_MSG_QUEUE_SIZE 100u

/** @brief inits jack midi out client
 *  @return 0 on success
 */
int jmocInit(const char * name);
void jmocReset();
void jmocWriteMidiData(const uint8_t channel, const uint8_t controller, const int8_t value);
