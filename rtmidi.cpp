#include <iostream>
#include <cstdlib>
#include <map>
#include <typeinfo>
#include <gtk/gtk.h>
#include <rtmidi/RtMidi.h>
#include "gdigi.h"

extern "C" void debug_msg (debug_flags_t flags, char *fmt, ...);
extern "C" void debug_msg_hex (unsigned char *in, int len);
extern "C" void push_message(GString *msg);

static void read_data_cb (double timeStamp, std::vector<unsigned char> *buf, void *userData);

static RtMidiIn *input = 0;
static RtMidiOut *output = 0;

extern "C" void _rtmidi_get_port_name (int api, int dev, char *name);
void _rtmidi_get_port_name (int api, int dev, char *name) {
    RtMidiIn *tmpdev = 0;
    std::string portName;

    try {
        tmpdev = new RtMidiIn(RtMidi::Api(api));
        portName = tmpdev->getPortName(dev);
    } catch ( RtMidiError &error ) {
      error.printMessage();
    }

    delete tmpdev;

    strcpy(name, portName.c_str());
}

/**
 *  Opens MIDI device. This function modifies global input and output variables.
 *
 *  \return FALSE on success, TRUE on error.
 **/
extern "C" gboolean open_device (gint packed_api_port);
gboolean open_device (gint packed_api_port) {
    gint api = RTMIDI_UNPACK_API(packed_api_port);
    gint device = RTMIDI_UNPACK_DEVICE(packed_api_port);

    debug_msg(DEBUG_STARTUP, (char *)"Using packed device %d = API: %d device: %d",
                                packed_api_port, api, device);

    input = new RtMidiIn(RtMidi::Api(api));
    output = new RtMidiOut(RtMidi::Api(api));

    input->openPort(device);
    output->openPort(device);

    input->ignoreTypes( false, true, true ); /* sysex, time, sense */

    input->setCallback(&read_data_cb);

    return FALSE;
}

/**
 *  \param data data to be sent
 *  \param length data length
 *
 *  Sends data to device. This function uses global output variable.
 **/
extern "C" void send_data(const unsigned char *data, int length);
void send_data(const unsigned char *data, int length) {
    output->sendMessage(data, length);
}

static void read_data_cb (double timeStamp, std::vector<unsigned char> *buf, void *userData)
{
    GString *string = NULL;
    int i = 0;
    int length = buf->size();

    debug_msg(DEBUG_MSG2HOST, (char *)"%s running: %d bytes", __FUNCTION__, buf->size());
    debug_msg_hex((unsigned char *)buf->data(), buf->size());

    while (i < length) {
        int pos;
        int bytes;

        if (string == NULL) {
            int debug_seek_start = i;
            /* seek start of SYSEX */
            while (buf->at(i) != MIDI_SYSEX && i < length)
                i++;
            if (i >= length) {
                debug_msg(DEBUG_MSG2HOST,
                            (char *)"%s didn't find MIDI_SYSEX from %d to %d; exiting",
                            __FUNCTION__, debug_seek_start, length);
                continue; /* no more SYSEX messages in this buffer */
            } else {
                debug_msg(DEBUG_MSG2HOST,
                            (char *)"%s found MIDI_SYSEX at input offset %d",
                            __FUNCTION__, i);
            }
        } else {
            debug_msg(DEBUG_MSG2HOST,
                        (char *)"%s appending to string, starting at input offset %d",
                        __FUNCTION__, i);
            /* else append to existing string */
        }

        pos = i;

        /* seek end of SYSEX */
        for (bytes = 0; (bytes<length-i) && (buf->at(i+bytes) != MIDI_EOX); bytes++);

        if (buf->at(i+bytes) == MIDI_EOX) {
            debug_msg(DEBUG_MSG2HOST,
                        (char *)"%s found MIDI_EOX at input offset %d",
                        __FUNCTION__, i + bytes);
            bytes++;
        } else {
            debug_msg(DEBUG_MSG2HOST,
                        (char *)"%s did not find MIDI_EOX in buffer (but appending)",
                        __FUNCTION__);
        }

        i += bytes;

        if (string == NULL) {
            debug_msg(DEBUG_MSG2HOST,
                        (char *)"%s created new string %d bytes starting at %d",
                        __FUNCTION__, bytes, pos);
            string = g_string_new_len((gchar*)&buf->at(pos), bytes);
            if (bytes == length)
                debug_msg_hex((unsigned char *)&buf->at(pos), bytes);
        } else {
            debug_msg(DEBUG_MSG2HOST,
                        (char *)"%s appended to string %d bytes at output offset %d",
                        __FUNCTION__, bytes, pos);
            g_string_append_len(string, (gchar*)&buf->at(pos), bytes);
        }

        if ((unsigned char)string->str[string->len-1] == MIDI_EOX) {
            debug_msg(DEBUG_MSG2HOST, (char *)"pushed onto stack");

            /* push message on stack */
            push_message(string);
            string = NULL;
        }
    }

    if (string) {
        g_string_free(string, TRUE);
        string = NULL;
    }
}

/**
 *  \param[out] devices GList containing numbers (packed into pointers)
 *              of connected DigiTech devices
 *
 *  Checks available soundcards for DigiTech devices.
 *
 *  \return the number of connected DigiTech devices.
 **/
extern "C" gint get_digitech_devices(GList **devices);
gint get_digitech_devices(GList **devices) {
    std::map<int, std::string> apiMap;
    apiMap[RtMidi::MACOSX_CORE] = "OS-X CoreMIDI";
    apiMap[RtMidi::WINDOWS_MM] = "Windows MultiMedia";
    apiMap[RtMidi::UNIX_JACK] = "Jack Client";
    apiMap[RtMidi::LINUX_ALSA] = "Linux ALSA";
    apiMap[RtMidi::RTMIDI_DUMMY] = "RtMidi Dummy";
    
    std::vector< RtMidi::Api > apis;
    RtMidi :: getCompiledApi( apis );

    debug_msg(DEBUG_STARTUP, (char *)"librtmidi version: %s",
                            RtMidi::getVersion().c_str());

    gint number = 0;

    for ( unsigned int api=0; api<apis.size(); api++ ){
        debug_msg(DEBUG_STARTUP, (char *)"Probing with API %s:",
                                apiMap[ apis[api] ].c_str());

        RtMidiIn *tmpdev = 0;

        try {
            tmpdev = new RtMidiIn(apis[api]);

            // Check inputs.
            unsigned int nPorts = tmpdev->getPortCount();

            for ( unsigned port=0; port<nPorts; port++ ) {
                gint packed_id = RTMIDI_PACK(apis[api], port);
                std::string portName = tmpdev->getPortName(port);
                std::string needle ("DigiTech");
                debug_msg(DEBUG_STARTUP, (char *)"  %d: %s (gdigi device id: %d)",
                                        port, portName.c_str(), packed_id);

                if (portName.find(needle) != std::string::npos) {
                    number++;
                    *devices = g_list_append(*devices, GINT_TO_POINTER(packed_id));
                }
            }

        } catch ( RtMidiError &error ) {
            error.printMessage();
        }
        delete tmpdev;
    }
    return number;
}

extern "C" void _rtmidi_exit();
void _rtmidi_exit() {
    if (output != NULL) {
        delete output;
    }

    if (input != NULL) {
        delete input;
    }
}
