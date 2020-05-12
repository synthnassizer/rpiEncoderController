/*
 * @file encoderControllerMain.c
 * @author Athanasios Silis
 * @license Public Domain
 */

#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <inttypes.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>

#include "rotaryEncoderClient.h"
#include "jackMidiOutClient.h"

/* defines */
#define DEFAULT_JACK_NAME "encoderController"
#define DEFAULT_PIGPIOD_HOST "localhost"
#define DEFAULT_PIGPIOD_PORT "8888"
#define ENCODER_CTRL_MAX_NUM (   3u)
#define MIDI_CTRL_MIN_VALUE  (-128 )
#define MIDI_CTRL_MAX_VALUE  ( 127 )
#define MIDI_MAX_CHANNEL     (  15u)

//BCM numbers of pins
#define ENC_BCM_PIN_1A       (17u) //pin # 11
#define ENC_BCM_PIN_1B       (27u) //pin # 13
#define ENC_BCM_PIN_1Button  (22u) //pin # 15
#define ENC_BCM_PIN_2A       (23u) //pin # 16
#define ENC_BCM_PIN_2B       (24u) //pin # 18
#define ENC_BCM_PIN_2Button  (25u) //pin # 22
#define ENC_BCM_PIN_3A       (05u) //pin # 29
#define ENC_BCM_PIN_3B       (06u) //pin # 31
#define ENC_BCM_PIN_3Button  (16u) //pin # 36

#define ENC_TRIPLET_INITIALIZER(A,B,BUTTON) { .gpioA = A , .gpioB = B , .gpioButton = BUTTON }
#define ENC_ENCODER_VALUE_INITIALIZER { .encoder = 0x0 , .encoderLow = MIDI_CTRL_MIN_VALUE , .encoderHigh = MIDI_CTRL_MAX_VALUE , .button = 0x0 }

/* typedefs */
typedef struct encoderControl_t
{
    uint8_t encoder;
    uint8_t button;
} encoderControl_t;
typedef struct encoderValue_t
{
    int8_t encoder;
    int8_t encoderLow;
    int8_t encoderHigh;
    int8_t button;
} encoderValue_t;

/* function forward declarations */
static void callbackButton(int pressed, unsigned id);
static void callbackEncoder(int pos, unsigned id);
static void triggerShutdown(int /*sig*/);
static void print_usage();
static void setupSignals();
static int inits(rec_t ** encoder, int * piGpio, const char * host, const char * port,
    const char * jackName, const uint8_t encMidiCtrlCounter, const int step);
static void shutdowns(rec_t ** encoder, int piGpio, const uint8_t encMidiCtrlCounter);
static int sanityChecks(const uint8_t encMidiCtrlCounter, const uint8_t encMidiCtrlValueCounter,
    const uint8_t encMidiChannel);
static int parseArgs(const int argc, char **argv, uint8_t * encMidiCtrlCounter,
    uint8_t * encMidiCtrlValueCounter, uint8_t * encMidiChannel, bool * printToStdOut,
    const char ** jackName, const char ** host, const char ** port, int * step);

/* variables */
bool keepRunning = true;
bool printToStdOut = false;
encoderValue_t encMidiCtrlValue[ENCODER_CTRL_MAX_NUM] = {
    ENC_ENCODER_VALUE_INITIALIZER, ENC_ENCODER_VALUE_INITIALIZER, ENC_ENCODER_VALUE_INITIALIZER
};
encoderControl_t encMidiCtrl[ENCODER_CTRL_MAX_NUM] = { 0 };
const encoderPinout_t encoderPins[ENCODER_CTRL_MAX_NUM] = 
{
    ENC_TRIPLET_INITIALIZER(ENC_BCM_PIN_1A , ENC_BCM_PIN_1B , ENC_BCM_PIN_1Button),
    ENC_TRIPLET_INITIALIZER(ENC_BCM_PIN_2A , ENC_BCM_PIN_2B , ENC_BCM_PIN_2Button),
    ENC_TRIPLET_INITIALIZER(ENC_BCM_PIN_3A , ENC_BCM_PIN_3B , ENC_BCM_PIN_3Button)
};

/* main */
int main(int argc, char **argv)
{
    rec_t * encoder[ENCODER_CTRL_MAX_NUM] = { 0 };
    uint8_t encMidiCtrlCounter = 0u;
    uint8_t encMidiCtrlValueCounter = 0u;
    uint8_t encMidiChannel = 0u;
    const char * jackName = DEFAULT_JACK_NAME;
    const char * host = DEFAULT_PIGPIOD_HOST;
    const char * port = DEFAULT_PIGPIOD_PORT;
    int step = 1;
    int piGpio = 0;

    if (0 != parseArgs(argc, argv, &encMidiCtrlCounter, &encMidiCtrlValueCounter,
        &encMidiChannel, &printToStdOut, &jackName, &host, &port, &step)) {
        return -1;
    }

    if (0 != sanityChecks(encMidiCtrlCounter, encMidiCtrlValueCounter, encMidiChannel)) {
        return -1;
    }


    if (0 != inits(encoder, &piGpio, host, port, jackName, encMidiCtrlCounter, step)) {
        return -1;
    }

    /* main loop */
    while (keepRunning) {
        usleep(500u * 1000u);
    }

    shutdowns(encoder, piGpio, encMidiCtrlCounter);

    return 0;
}

static int parseArgs(const int argc, char ** argv, uint8_t * encMidiCtrlCounter,
    uint8_t * encMidiCtrlValueCounter, uint8_t * encMidiChannel, bool * printToStdOut,
    const char ** jackName, const char ** host, const char ** port, int * step)
{
    int argn = 1;

    if (argc <= 1) {
        print_usage();
        return -1;
    }

    while (argn < argc && argv[argn][0] == '-' && argv[argn][1] != '\0')
    {
        if ( strcmp( argv[argn], "-c" ) == 0 && argn + 1 < argc ) {
            ++argn;
            unsigned enc = 255u, but = 255u;
            if ((sscanf(argv[argn], "%u,%u", &enc, &but) != 2)) {
                print_usage();
                return -1;
            }

            encMidiCtrl[*encMidiCtrlCounter % ENCODER_CTRL_MAX_NUM].encoder = (uint8_t)enc;
            encMidiCtrl[*encMidiCtrlCounter % ENCODER_CTRL_MAX_NUM].button = (uint8_t)but;
            ++(*encMidiCtrlCounter);
        } else if ( strcmp( argv[argn], "-a" ) == 0 && argn + 1 < argc ) {
            ++argn;
            int val = 255, lowLim = MIDI_CTRL_MIN_VALUE, highLim = MIDI_CTRL_MAX_VALUE;
            if ((sscanf(argv[argn], "%d,%d,%d", &val, &lowLim, &highLim) != 3)) {
                if ((sscanf(argv[argn], "%d,%d", &val, &lowLim) != 2)) {
                    if ((sscanf(argv[argn], "%d", &val) != 1)) {
                        print_usage();
                        return -1;
                    }
                } else {
                    print_usage();
                    return -1;
                }
            } else {
                encMidiCtrlValue[*encMidiCtrlValueCounter % ENCODER_CTRL_MAX_NUM].encoderLow = (int8_t)lowLim;
                encMidiCtrlValue[*encMidiCtrlValueCounter % ENCODER_CTRL_MAX_NUM].encoderHigh = (int8_t)highLim;
            }

            encMidiCtrlValue[*encMidiCtrlValueCounter % ENCODER_CTRL_MAX_NUM].encoder = (int8_t)val;
            ++(*encMidiCtrlValueCounter);
        } else if ( strcmp( argv[argn], "-n" ) == 0 && argn + 1 < argc ) {
            ++argn;
            *jackName = argv[argn];
        } else if ( strcmp( argv[argn], "-s" ) == 0 && argn + 1 < argc ) {
            ++argn;
            *host = argv[argn];
        } else if ( strcmp( argv[argn], "-e" ) == 0 && argn + 1 < argc ) {
            ++argn;
            *step = atoi(argv[argn]);
        } else if ( strcmp( argv[argn], "-h" ) == 0 && argn + 1 < argc ) {
            ++argn;
            *encMidiChannel = atoi(argv[argn]);
        } else if ( strcmp( argv[argn], "-p" ) == 0 && argn + 1 < argc ) {
            ++argn;
            *port = argv[argn];
        } else if ( strcmp( argv[argn], "-t" ) == 0 ) {
            *printToStdOut = true;
        } else if ( strcmp( argv[argn], "-h" ) == 0 ) {
            print_usage();
            return 1;
        }
        else
        {
            print_usage();
            return -1;
        }
        ++argn;
    }

    if (argn > argc)
    {
        print_usage();
        return -1;
    }

    return 0;
}

static int inits(rec_t ** encoder, int * piGpio, const char * host, const char * port,
    const char * jackName, const uint8_t encMidiCtrlCounter, const int step)
{
    *piGpio = recPigpioConnect(host, port); 
    if (*piGpio < 0) {
        fprintf(stderr,"ERROR: Failed to connect to pigpiod\n\n");
        return -1;
    }

    if (0 != jmocInit(jackName)) {
        fprintf(stderr,"ERROR: Failed to initialise jack client\n\n");
        return -1;
    }

    for (unsigned i = 0u; i < encMidiCtrlCounter; ++i) {
        encoder[i] = recInit(*piGpio, &encoderPins[i], i, step, callbackEncoder, callbackButton);
    }

    setupSignals();
    printf ("Initialisations completed. Listening to encoders...\n");

    return 0;
}

static void setupSignals()
{
    signal(SIGTERM, triggerShutdown);
    signal(SIGHUP, triggerShutdown);
    signal(SIGINT, triggerShutdown);
}

static void shutdowns(rec_t ** encoder, int piGpio, const uint8_t encMidiCtrlCounter)
{
    printf("encoderController shutting down\n\n");

    for (unsigned i = 0u; i < encMidiCtrlCounter; ++i) {
        recReset(encoder[i]);
    }

    jmocReset();
    recPigpioDisconnect(piGpio);
}

static int sanityChecks(const uint8_t encMidiCtrlCounter, const uint8_t encMidiCtrlValueCounter,
    const uint8_t encMidiChannel)
{
    if ((ENCODER_CTRL_MAX_NUM < encMidiCtrlCounter) || (0u == encMidiCtrlCounter)) {
        fprintf(stderr,"ERROR: Midi controller assignments out of range 1-3\n\n");
        print_usage();
        return -1;
    }

    if (ENCODER_CTRL_MAX_NUM < encMidiCtrlValueCounter) {
        fprintf(stderr,"ERROR: Midi controller assigned value out of range 0-3\n\n");
        print_usage();
        return -1;
    }

    if ((0u < encMidiCtrlValueCounter) && (encMidiCtrlCounter != encMidiCtrlValueCounter)) {
        fprintf(stderr,"ERROR: Midi controller and controller value assignments do not match\n\n");
        print_usage();
        return -1;
    }

    for (unsigned i = 0u; i < encMidiCtrlCounter; ++i) {
        if ((MIDI_CTRL_MAX_VALUE < encMidiCtrl[i].encoder) || (MIDI_CTRL_MAX_VALUE < encMidiCtrl[i].button)) {
            fprintf(stderr,"ERROR: Midi controller %d controller# %u or %u out of range\n\n",i,encMidiCtrl[i].encoder,encMidiCtrl[i].button);
            print_usage();
            return -1;
        }
    }

    if (MIDI_MAX_CHANNEL < encMidiChannel) {
        fprintf(stderr,"ERROR: Midi channel out of range 0-15\n\n");
        print_usage();
        return -1;
    }

    return 0;
}

static void callbackEncoder(int step, unsigned id)
{
    int16_t encVal = encMidiCtrlValue[id].encoder;
    encVal += (int8_t)step;

    if (encMidiCtrlValue[id].encoderHigh < encVal) encVal = encMidiCtrlValue[id].encoderHigh;
    if (encMidiCtrlValue[id].encoderLow > encVal) encVal = encMidiCtrlValue[id].encoderLow;

    encMidiCtrlValue[id].encoder = encVal;
    jmocWriteMidiData(0x0, encMidiCtrl[id].encoder, encMidiCtrlValue[id].encoder);

    if (printToStdOut)
        printf("enc[%u]=%d (dir=%d)\n", encMidiCtrl[id].encoder, encMidiCtrlValue[id].encoder, step);
}

static void callbackButton(int pressed, unsigned id)
{
    encMidiCtrlValue[id].button = (int8_t)pressed;

    jmocWriteMidiData(0x0,encMidiCtrl[id].button, encMidiCtrlValue[id].button);

    if (printToStdOut)
        printf("but[%u]=%d\n", encMidiCtrl[id].button, encMidiCtrlValue[id].button);
}

static void triggerShutdown(int sig)
{
    (void)sig;
    keepRunning = false;
}

static void print_usage()
{
    printf( "Usage: encoderController -c <midi ctrl number> [-s host] [-p port] [-e step] [-t] [-h] [-n jackname] \n\n"
            "Desc:  The programs initialises reads input from 1-3 rotary encoders,\n"
            "       encapsulates the values to midi msgs (controller change - CC - specifically) and\n"
            "       registers itself as a jack midi readable (=with output port) client\n\n"
            "Arguments:\n"
            "\t-c <ctrl#Encoder,ctrl#Button> : midi controller # for encoder and button.\n"
            "\t                This options is allowed 1-3 times. Value up to 127.\n"
            "\t-a <value>,[low,high] : initial encoder value and optionally a range.\n"
            "\t                This options is allowed 0-3 times and is used for the encoders only.\n"
            "\t                It must correspond to the number of -c's if used.\n"
            "\t                Default Value range [-128,+127].\n"
            "\t-h <channel>  : midi channel (default: 0).\n"
            "\t-e <step>     : encoder step (default: 1).\n"
            "\t-s <host>     : running pigpiod (default: " DEFAULT_PIGPIOD_HOST ").\n"
            "\t-p <port>     : of running pigpiod (default: " DEFAULT_PIGPIOD_PORT ").\n"
            "\t-n <jackname> : name of jack client (default: " DEFAULT_JACK_NAME ").\n"
            "\t-t <no arg>   : print to stdout instead of loading the jack client\n"
            "\t-h <no arg>   : this help message\n\n"
    );
}
