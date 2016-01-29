
#ifndef MAD_PARSER_H_INCLUDED
#define MAD_PARSER_H_INCLUDED

#include "Fluid/TestPlayer.h"
#include <mad.h>
#include <portaudio.h>

//mad buffer
struct data_buffer {
    const unsigned char* fdm;//file descriptor
    unsigned long currentFrame; //current frame in mp3 file, for GUI update
    unsigned long numFrames;    // total number of frames in a file
    unsigned long length;       //file length in bytes
    unsigned int samples_per_frame; //number of (mono or stereo) samples contained ina frame
    unsigned int samplerate;    //sample frequency, tipically 44100 or 48000 per sec
    TestPlayerGui* p;           // a pointer to GUI, to update public fields
    PaStream *audioStream;      // a pointer to current Port Audio playback stream (e. g. Direct X or WMME or ALSA stream)
    unsigned int* frame_offset;  // Phisical address of frames in a file.
    boolean slider_in_use;
    struct mad_decoder* decoder;
    short nchannels;
    Fl_Double_Window *processingWindow;
};

data_buffer* mpdata_init();

enum mad_flow mad_input_cb(void *data, struct mad_stream *stream);//read and decode input mp3 data
enum mad_flow read_mad_frame(void *data, struct mad_stream *stream);//read frame
enum mad_flow mad_output_cb(void *data, struct mad_header const *header, struct mad_pcm *pcm);//outputing decoded mp3 data
enum mad_flow mad_error_cb(void *data, struct mad_stream *stream, struct mad_frame *frame);//error callback function

#endif // MAD_PARSER_H_INCLUDED
