#include "mad_parser.h"
#include <process.h>
#include <unistd.h>
#include "Fluid/PlayerGUI_prot.h"

#define INNITIAL_NUMFRAMES 1024
int outputBuffer[2*1152];

data_buffer* mpdata_init(){
    struct data_buffer* playback_info;
    playback_info = (struct data_buffer*) malloc(sizeof(struct data_buffer));
    if(!playback_info)
        return 0;
//    playback_info->audioStream=0;
    playback_info->currentFrame=0;
    playback_info->fdm=0;
    playback_info->length=0;
    playback_info->numFrames=0;
    playback_info->samples_per_frame=0;
    playback_info->samplerate=0;
    playback_info->audioStream=NULL;
    playback_info->frame_offset=(unsigned int*)malloc(INNITIAL_NUMFRAMES*sizeof(int));
    playback_info->decoder=(struct mad_decoder*) malloc(sizeof(struct mad_decoder));
    memset(playback_info->decoder,0,sizeof(struct mad_decoder));
    playback_info->slider_in_use=false;
    playback_info->nchannels=0;
    Fl_Double_Window *processingWindow=NULL;
    return playback_info;
}

static inline signed int scale(mad_fixed_t sample){
	/* round */
	sample += (1L << (MAD_F_FRACBITS - 16));

	/* clip */
	if (sample >= MAD_F_ONE)
		sample = MAD_F_ONE - 1;
	else if (sample < -MAD_F_ONE)
		sample = -MAD_F_ONE;

	/* quantize */
	sample=sample >> (MAD_F_FRACBITS + 1 - 16);
//	sample = (sample >> 0) & 0xff;
//	sample = (sample >> 8) & 0xff;
	return sample ;
}

enum mad_flow mad_input_cb(void *data, struct mad_stream *stream){
    struct data_buffer *buffer = (struct data_buffer*) data;
    if (!buffer->length)
        return MAD_FLOW_STOP;
    int error;
    mad_stream_buffer(stream, buffer->fdm, buffer->length);
    mad_stream_skip(stream, buffer->frame_offset[buffer->currentFrame]);
    error=mad_stream_sync(stream);
    printf("Stream sync error: %d\n",error);
    //buffer->length = 0;
    return MAD_FLOW_CONTINUE;
}

enum mad_flow read_mad_frame(void *data, struct mad_stream *stream){
    unsigned long max_frames=INNITIAL_NUMFRAMES;
    struct data_buffer *buffer = (struct data_buffer*) data;
    if (!buffer->length || buffer->p->isStopped)
        return MAD_FLOW_STOP;
    mad_stream_buffer(stream, buffer->fdm, buffer->length);
    struct mad_frame frame;
	mad_frame_init(&frame);
	int result=-1;
	while(-1==result)
        result=mad_frame_decode(&frame, stream);
	printf("Decode result: %d\n", result);
	printf("Duration: %d\n",frame.header.duration);
	printf("Fraction: %d\n",frame.header.duration.fraction);
	buffer->nchannels=frame.header.mode;
    buffer->samplerate=frame.header.samplerate;
	buffer->samples_per_frame=32 * MAD_NSBSAMPLES (&frame.header);
	printf("Samplerate on header: %d\n",buffer->samples_per_frame);
	printf("Samplerate: %d\n",buffer->samplerate);
    void* end_of_file=(void*) (buffer->fdm+buffer->length);
    while(result>=0){
        buffer->frame_offset[buffer->numFrames++]=stream->this_frame-stream->buffer;
        if(buffer->numFrames==max_frames){
            buffer->frame_offset=(unsigned int*)realloc(buffer->frame_offset, sizeof(int) * (max_frames * 3 / 2));
            max_frames=max_frames*3/2;
            if(!buffer->frame_offset)
                return MAD_FLOW_BREAK;
        }
        result=mad_header_decode(&frame.header, stream);
    }
	//result=mad_header_decode(&frame.header, stream);
	printf("Total frames: %u\n", buffer->numFrames);
    printf("Decode result: %d\n", result);
	printf("Duration: %d\n",frame.header.duration);
	printf("Fraction: %d\n",frame.header.duration.fraction);
	printf("Mode: %d\n",frame.header.mode);
	printf("Length: %d\n",buffer->length);
    if(32 * MAD_NSBSAMPLES (&frame.header)!=buffer->samples_per_frame)
        printf("Error! Samplerate on header: %d\n",32 * MAD_NSBSAMPLES (&frame.header));


	mad_frame_finish(&frame);
    buffer->length = 0;
    return MAD_FLOW_STOP;
}

enum mad_flow mad_output_cb(void *data, struct mad_header const *header, struct mad_pcm *pcm){

	data_buffer* buffer=(data_buffer* )data;

	if(buffer->p->isStopped){
        printf("Received stop signal!\n");
        return MAD_FLOW_BREAK;
    }

    if(buffer->currentFrame > buffer->numFrames){
        printf("Memory overrun!\n");
        return MAD_FLOW_BREAK;
    }

    int sampleNum=0;
    ++buffer->currentFrame;
    //if(buffer->p->){
    if(!buffer->slider_in_use){
		//printf("Event handler: %d\n",buffer->slider_in_use);
        double pos;
        pos=(double) buffer->currentFrame * buffer->samples_per_frame / (double) buffer->samplerate;
        buffer->p->slider->value(pos);
        buffer->p->currentPos->value(pos);
        buffer->p->remaining->value(buffer->p->duration->value() - pos);
    }else{
		printf("In Use\n");
		printf("Boolean?: %d\n",buffer->slider_in_use);
    }


    //setpos(buffer->p, TXTREM);
    //printf("Current pos: %f\n", pos);
	/* pcm->samplerate contains the sampling frequency */

	memcpy(buffer->p->inputData[0], pcm->samples[0], 1152*sizeof(int));
	if(buffer->nchannels==1)
        memcpy(buffer->p->inputData[1], pcm->samples[0], 1152*sizeof(int));
    else
        memcpy(buffer->p->inputData[1], pcm->samples[1], 1152*sizeof(int));

	audioOutput();
	return MAD_FLOW_CONTINUE;
}

enum mad_flow mad_error_cb(void *data, struct mad_stream *stream, struct mad_frame *frame){
	struct data_buffer *buffer = (struct data_buffer *)data;

	fprintf(stderr, "decoding error 0x%04x (%s) at byte offset %u\n",
		stream->error, mad_stream_errorstr(stream),
		stream->this_frame - buffer->fdm);
    printf("Current frame: %d\n",buffer->currentFrame);
    printf("Number of frames: %d\n",buffer->numFrames);
    if(buffer->currentFrame >= buffer->numFrames-1 ){
        buffer->p->currentPos->value(0);
        buffer->p->isPlaying=false;
        buffer->p->isStopped=false;
        setpos(buffer->p, TXTPOS);
        return MAD_FLOW_BREAK;
    }

	/* return MAD_FLOW_BREAK here to stop decoding (and propagate an error) */
	return MAD_FLOW_CONTINUE;
}
