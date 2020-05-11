/*
 * @file    jackMidiOutClient.c
 * @author  Athanasios Silis
 * @license Public Domain
 */

#include <stdio.h>
#include <pthread.h>
#include <jack/jack.h>
#include <jack/midiport.h>
#include <jack/ringbuffer.h>

#include "jackMidiOutClient.h"

#define MIDI_CC_CMD 0xB0 /*  controller change command (the 4 MSBs only)*/

typedef struct midiCtrlWithVal_t
{
    uint8_t channel;
    uint8_t controller;
    int8_t value;
} midiCtrlWithVal_t;

static int jmocProcess(jack_nframes_t nframes, void *arg);


static jack_client_t * client = NULL;
static jack_port_t * midiOutPort = NULL;
static jack_ringbuffer_t * rb = NULL;
static pthread_mutex_t msg_thread_lock = PTHREAD_MUTEX_INITIALIZER;

int jmocInit(const char * name)
{
    if ((client = jack_client_open(name, JackNullOption, NULL)) == 0) {
        fprintf (stderr, "JACK server not running?\n");
        return -1;
    }

    rb = jack_ringbuffer_create (MIDI_MSG_QUEUE_SIZE * sizeof(midiCtrlWithVal_t));
    jack_set_process_callback(client, jmocProcess, 0);
    midiOutPort = jack_port_register(client, "out", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput, 0);

    if (jack_activate(client)) {
        fprintf (stderr, "cannot activate client");
        return -1;
    }

    return 0;
}

void jmocReset()
{
    if (client)
    {
        jack_deactivate(client);
        jack_client_close(client);
        client = NULL;
    }

    if (rb)
    {
        jack_ringbuffer_free(rb);
        rb = NULL;
    }
}


void jmocWriteMidiData(const uint8_t channel, const uint8_t controller, const int8_t value)
{
    midiCtrlWithVal_t midiVal = { 0 };

    midiVal.channel = channel;
    midiVal.controller = controller;
    midiVal.value = value;

    pthread_mutex_lock(&msg_thread_lock);
    jack_ringbuffer_write(rb, (void *)&midiVal, sizeof(midiCtrlWithVal_t));
    pthread_mutex_unlock(&msg_thread_lock);
}

static int jmocProcess(jack_nframes_t nframes, void *arg)
{
    (void)arg;
    unsigned i = 0;
    void* port_buf = jack_port_get_buffer(midiOutPort, nframes);
    unsigned char* buffer = NULL;

    jack_midi_clear_buffer(port_buf);

    if (0 == pthread_mutex_trylock(&msg_thread_lock))
    {
        for (i = 0; i < nframes; i++)
        {
            midiCtrlWithVal_t midiVal = { 0 };
            size_t readBytes = jack_ringbuffer_read(rb, (char*)&midiVal, sizeof(midiCtrlWithVal_t));

            if (0u < readBytes)
            {
                buffer = jack_midi_event_reserve(port_buf, i, 3);
                if (buffer)
                {
                    buffer[2] = midiVal.value;
                    buffer[1] = midiVal.controller;
                    buffer[0] = (MIDI_CC_CMD | (midiVal.channel & 0xF));
                }
            }
        }
        pthread_mutex_unlock(&msg_thread_lock);
    }

    return 0;
}
