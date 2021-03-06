// generated by Fast Light User Interface Designer (fluid) version 1.0107

#include <iostream>
#include "Fluid/TestPlayer.h"
#include <FL/x.H>
#include <sndfile.h>
#include <mad.h>
#include "threads.h"
#include <portaudio.h>
#include "mman.h"
#include <math.h>
#include "mad_parser.h"
#include "unistd.h"
#include <fftw3.h>
#include <limits.h>
#include "filters.h"

#define FRAME_BLOCK_LEN 1152

Fl_Thread play_thread;

struct data_buffer* playbackData;

const PaDeviceInfo *info;

struct audioData *filtLayout;

int *out;

boolean flag=true;

struct chartStruct{
    Fl_Chart* flChart;
    fftw_complex *input, *output;
    fftw_plan plan;
};

chartStruct inputCharts[2];
chartStruct outputCharts[8];

int audioCallback(
    const void *input, void *output,
    unsigned long frameCount,
    const PaStreamCallbackTimeInfo* timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData );

void setupChart(chartStruct* chart, boolean logScale){
    int i;
    chart->flChart->type(FL_FILL_CHART);
	chart->flChart->maxsize(520);
	if(logScale)
		chart->flChart->bounds(7.7, log10l(INT_MAX) * 1.12);
	else
		chart->flChart->bounds(.0, (double) INT_MAX * 15);
	for(i=0;i<520;i++){
		chart->flChart->add(0,0,0);
	}
	chart->input = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FRAME_BLOCK_LEN);
	chart->output = (fftw_complex*) fftw_malloc(sizeof(fftw_complex) * FRAME_BLOCK_LEN);
    chart->plan = fftw_plan_dft_1d(FRAME_BLOCK_LEN, chart->input, chart->output, FFTW_FORWARD, FFTW_ALLOW_PRUNING);
}


void setupSpectras(TestPlayerGui* p){
    inputCharts[0].flChart=p->inputSpectraL;
    inputCharts[1].flChart=p->inputSpectraR;
    outputCharts[0].flChart=p->outputSpectra0;
    outputCharts[1].flChart=p->outputSpectra1;
    outputCharts[2].flChart=p->outputSpectra2;
    outputCharts[3].flChart=p->outputSpectra3;
    outputCharts[4].flChart=p->outputSpectra4;
    outputCharts[5].flChart=p->outputSpectra5;
    outputCharts[6].flChart=p->outputSpectra6;
    outputCharts[7].flChart=p->outputSpectra7;
	setupChart(&inputCharts[1], flag);
    setupChart(&inputCharts[0], flag);

    int i;
    for(i=0;i<2;i++)
        setupChart(&inputCharts[i], flag);
    for(i=0;i<info->maxOutputChannels;i++)
        setupChart(&outputCharts[i], flag);
}


void plotSpectras(void* gui){
	TestPlayerGui* p=(TestPlayerGui*) gui;
    int i, j;
    double datapoint;
	while(true)
	if(p->isPlaying && !p->isStopped && p->processingWindow->visible()){
			usleep(1000000000);
			for(i=0;i<FRAME_BLOCK_LEN;i++){
				inputCharts[0].input[i][0]=(double)p->inputData[0][i];
				inputCharts[1].input[i][0]=(double)p->inputData[1][i];
				for(j=0;j<info->maxOutputChannels;j++)
					outputCharts[j].input[i][0]=(double)p->outputData[j][i];
			}
			fftw_execute(inputCharts[0].plan);
			fftw_execute(inputCharts[1].plan);
			for(j=0;j<info->maxOutputChannels;j++)
				fftw_execute(outputCharts[j].plan);

			for(i=8;i<520;i++){
				for(j=0;j<info->maxOutputChannels;j++){
					datapoint= sqrtl(outputCharts[j].output[i][0]*outputCharts[j].output[i][0]
										+ outputCharts[j].output[i][1]*outputCharts[j].output[i][1]);
					if(flag)
						outputCharts[j].flChart->replace(i,log10l(datapoint),0,FL_GREEN);
					else
						outputCharts[j].flChart->replace(i,datapoint,0,FL_GREEN);
				}
				for(j=0;j<2;j++){
					datapoint= sqrtl(inputCharts[j].output[i][0]*inputCharts[j].output[i][0]
										+ inputCharts[j].output[i][1]*inputCharts[j].output[i][1]);
					if(flag)
						inputCharts[j].flChart->replace(i,log10l(datapoint),0,FL_GREEN);
					else
						inputCharts[j].flChart->replace(i,datapoint,0,FL_GREEN);
				}
			}
        }
}

void toggleProcWindow(void *gui){
    TestPlayerGui *p=(TestPlayerGui*) gui;
    if(p->procToggle->value()){
        p->processingWindow->position(p->MainWindow->x()+p->MainWindow->w()+5,p->MainWindow->y());
        p->processingWindow->show();
    }

    else
        p->processingWindow->hide();
}

void setButton(void *gui){
    TestPlayerGui *p=(TestPlayerGui*) gui;
    p->processingWindow->hide();
    p->procToggle->value(0);
}

void handleWindows(void *gui){
    TestPlayerGui *p=(TestPlayerGui*) gui;
    if(!p->MainWindow->visible())
    p->processingWindow->hide();
}

void refresh(void* target){
	HWND callback_target=*(HWND*)target;
	while(callback_target){
		PostMessage(callback_target,0,0,0);
		usleep(100);
	}
}

void setFileName(void *gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	int i;
    int loc=0;
	for(i=0,i<FRAME_BLOCK_LEN;;i++){
        if('/'==(char)p->filename[i] || '\\'==(char)p->filename[i] )
            loc=i+1;
        if('\0'==(char)p->filename[i])
            break;
	}
	p->fileName->value(&p->filename[loc]);
	p->fileName->label("File");
}

int check_mpeg(char* filename){

	FILE* mp3;
    mp3=fopen(filename,"rb");
    struct stat filestat;
    int fd = fileno(mp3);
    fstat(fd, &filestat);
    unsigned long filesize=filestat.st_size;
    printf("Filesize: %u\n",filesize);
    void *fdm;
    fdm = mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if (fdm == MAP_FAILED)
        return -1;
    struct mad_decoder decoder;

    playbackData->fdm= (const unsigned char*)fdm;
    //buffer->currentFrame=p->currentPos->value();
    playbackData->length=filesize;
    //buffer->p=p;
    //buffer->callback_target=callback_target;
      /* configure input, output, and error functions */

    mad_decoder_init(&decoder, playbackData,
        read_mad_frame, 0 /* header */, 0 /* filter */, 0,
		mad_error_cb, 0 /* message */);
	mad_decoder_run(&decoder,MAD_DECODER_MODE_SYNC);
	mad_decoder_finish(&decoder);
	fclose(mp3);
	if(playbackData->numFrames<10)
		return -1;
	return 1;
}

void load(void *gui){
	static int filtVal=0;
	TestPlayerGui *p = (TestPlayerGui*) gui;
	char *currDir=p->currDir;
	p->fc->type(0); // only single files
	p->fc->filter("Audio Files (*.{mp3,wav,flac,ogg, ogv, oga, ogx, ogm, spx, opus})");
	p->fc->filter_value(filtVal);
	p->fc->directory(currDir);
	p->fc->value(currDir);
	p->fc->preview(0); // no preview
	//p->fc->rescan();
	p->fc->show();
	while (p->fc->visible())
		Fl::wait();
	strcpy(currDir,p->fc->directory());
	filtVal = p->fc->filter_value();
	if (p->fc->count() > 0 && currDir[0] != '\0') { // a file is selected and OK is pressed
		stop(p);
		PaStream* preserveAudioStream=playbackData->audioStream;
        playbackData=mpdata_init();
        playbackData->p=p;
		playbackData->audioStream=preserveAudioStream;
		//playbackData->processingWindow=p->processingWindow;
        playbackData->slider_in_use=true;
        p->slider->setMad(playbackData);
        //playbackData->processingWindow=p->processingWindow;
		strcpy (p->filename, p->fc->value());
		p->fileName->label(p->filename);

        char temp[256];
        strcpy(temp, p->filename);
		if((p->fin=sf_open(p->filename, SFM_READ, &p->input_info))>0){
			p->libToUse=1;
		}else if(check_mpeg(p->filename)>0){
			p->libToUse=2;
		}else{
			sprintf(temp,"Could not open %s \n", p->filename);
			printf("Fin %d \n", p->fin);
			p->libToUse=0;
		}
		printf("Lib  to use: %d\n", p->libToUse);
        setFileName(p);
		if(p->libToUse==1){
			p->length = (double) p->input_info.frames/(double) p->input_info.samplerate;
		}else if(p->libToUse==2){
			p->length = (double) playbackData->numFrames*playbackData->samples_per_frame / (double) playbackData->samplerate;
		}
		//sfd = psf_sndOpen(p->filename, &props, 0);
		//double length = (double)  psf_sndSize(sfd) / (double) props.srate;
        printf("Length %d \n", p->length);

		p->slider->range(0.d, p->length);
		p->duration->value(p->length);
		p->remaining->value(p->length);
		p->currentPos->value(0);
		p->slider->value(0);
		p->ffStep = p->length/20;
		p->fileName->redraw();
		sf_close(p->fin);
		//psf_sndClose(sfd);
		FILE* iniFile;
		iniFile = fopen("./settings.ini","w");
		if(iniFile)
			fwrite(currDir,FRAME_BLOCK_LEN,1,iniFile);
		fclose(iniFile);
		p->isPlaying=false;
		p->isStopped=false;
		playbackData->slider_in_use=false;
	}
}

int pa_init(){
    /** PortAudio setup */
    PaStreamParameters outputParameters;
    const PaHostApiInfo *hostApiInfo;
    printf("Initializing portaudio. Please wait...\n");
    Pa_Initialize();
    int id, error;
    id=Pa_GetDefaultOutputDevice();
    info = Pa_GetDeviceInfo(id); //get info about chosen device
    hostApiInfo = Pa_GetHostApiInfo(info->hostApi);
    printf("Default sample rate: %f\n",info->defaultSampleRate);
    printf("Max Output channels: %d\n",info->maxOutputChannels);
    printf("Struct version: %d\n",info->structVersion);
    printf("Type: %d\n",hostApiInfo->type);

    PaDeviceIndex ind  = Pa_GetDeviceCount(  );
    printf("Device id: %d\n",id);
    printf("Device count: %d\n",ind);

	//for(id=0;id<ind;id++){
	info = Pa_GetDeviceInfo(id); //get info about chosen device
    hostApiInfo = Pa_GetHostApiInfo(info->hostApi);
	printf("Device id: %d\n",id);
    printf("Default sample rate: %f\n",info->defaultSampleRate);
    printf("Max Output channels: %d\n",info->maxOutputChannels);
    printf("Struct version: %d\n",info->structVersion);
    printf("Type: %d\n",hostApiInfo->type);
    printf("Name [%s] %s\n", hostApiInfo->name, info->name);
    printf("*************************************************\n");
	//}
	//id=15;
	info = Pa_GetDeviceInfo(id); //get info about chosen device
    hostApiInfo = Pa_GetHostApiInfo(info->hostApi);

    printf("Default sample rate: %f\n",info->defaultSampleRate);
    printf("Max Output channels: %d\n",info->maxOutputChannels);
    printf("Struct version: %d\n",info->structVersion);
    printf("Type: %d\n",hostApiInfo->type);

    printf("Opening AUDIO Output device [%s] %s\n", hostApiInfo->name, info->name);
    outputParameters.device=id;
    outputParameters.channelCount=info->maxOutputChannels;
    outputParameters.sampleFormat=paInt32;
    outputParameters.suggestedLatency=info->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = NULL; //no specific info
    playbackData->audioStream=calloc(sizeof(long),1);
    int samplerate;
    if(playbackData->samplerate!=0)
        samplerate=playbackData->samplerate;
    else
        playbackData->samplerate=samplerate=info->defaultSampleRate;
    error=Pa_OpenStream(//Opening the PaStream object
                  &playbackData->audioStream,//the actual portaudio stream object
                  NULL, //input params
                  &outputParameters, //output parameters
                  (double)samplerate,
                  0, //(Interleaved) frame block length
                  paClipOff,//disabling clipping of out of range amplitude values
                  NULL,//callback function address, i. e. a pointer to function
                  NULL); //data for callback
	if(error)
        return -1;
    error=Pa_StartStream(playbackData->audioStream);
	if(error)
        return -1;
    char hz[50]={0};
    sprintf(hz, "   Available output channels: %d ",info->maxOutputChannels);

    int i;
    filtLayout = (audioData*) malloc(info->maxOutputChannels * sizeof(audioData));
    playbackData->p->filtLabel->copy_label(hz);
    playbackData->p->outputData=(int**)malloc(info->maxOutputChannels * sizeof(int*));
    for(i=0;i<info->maxOutputChannels;i++){
		playbackData->p->outputData[i]=(int*)malloc(FRAME_BLOCK_LEN * sizeof(int));
		filtLayout[i].cutoffFreq=20.;
		filtLayout[i].bandWidth=20.;
		if(i<2)
			filtLayout[i].filtType=PLAIN;
		else
			filtLayout[i].filtType=NONE;
		filtLayout[i].outputChannel=filtLayout[i].inputChannel=i%2;

    }


    byte inCh=1;
    for(i=0;i<info->maxOutputChannels;i++){
        inCh^=0x0001;
        filtLayout[i].inputChannel=inCh;
        filtLayout[i].filtType=PLAIN;
        if(i>1)
            filtLayout[i].filtType=NONE;
        filtLayout[i].outputChannel=i;
    }

    playbackData->p->inputChan->add("Left");
    playbackData->p->inputChan->add("Right");
    playbackData->p->inputChan->value(0);

    switch(info->maxOutputChannels){
	default:
		playbackData->p->middleLeftOpen->hide();
		playbackData->p->middleRightOpen->hide();
		playbackData->p->rearLeftOpen->hide();
		playbackData->p->rearRightOpen->hide();
		playbackData->p->centerOpen->hide();
		playbackData->p->subwooferOpen->hide();
        playbackData->p->processingWindow->size(610,220);
		playbackData->p->specBox->resize(5,30,600, 185);
		playbackData->p->inputSpectraL->resize(10,35,290,85);
		playbackData->p->inputSpectraR->resize(310,35,290,85);
		playbackData->p->outputSpectra0->resize(10,125,290,85);
		playbackData->p->outputSpectra1->resize(310,125,290,85);
		playbackData->p->outputSpectra2->hide();
		playbackData->p->outputSpectra3->hide();
		playbackData->p->outputSpectra4->hide();
		playbackData->p->outputSpectra5->hide();
		playbackData->p->outputSpectra6->hide();
		playbackData->p->outputSpectra7->hide();
		break;
    case 3:
		playbackData->p->middleLeftOpen->hide();
		playbackData->p->middleRightOpen->hide();
		playbackData->p->rearLeftOpen->hide();
		playbackData->p->rearRightOpen->hide();
		playbackData->p->centerOpen->hide();
        playbackData->p->processingWindow->size(610,310);
		playbackData->p->specBox->size(600, 275);
		playbackData->p->inputSpectraL->resize(10,35,290,85);
		playbackData->p->inputSpectraR->resize(310,35,290,85);
		playbackData->p->outputSpectra0->resize(10,125,290,85);
		playbackData->p->outputSpectra1->resize(310,125,290,85);
		playbackData->p->outputSpectra2->resize(10,215,290,85);
		playbackData->p->outputSpectra3->hide();
		playbackData->p->outputSpectra4->hide();
		playbackData->p->outputSpectra5->hide();
		playbackData->p->outputSpectra6->hide();
		playbackData->p->outputSpectra7->hide();
        break;
    case 4:
		playbackData->p->middleLeftOpen->hide();
		playbackData->p->middleRightOpen->hide();
		playbackData->p->centerOpen->hide();
		playbackData->p->subwooferOpen->hide();
        playbackData->p->processingWindow->size(610,310);
		playbackData->p->specBox->size(600, 275);
		playbackData->p->inputSpectraL->resize(10,35,290,85);
		playbackData->p->inputSpectraR->resize(310,35,290,85);
		playbackData->p->outputSpectra0->resize(10,125,290,85);
		playbackData->p->outputSpectra1->resize(310,125,290,85);
		playbackData->p->outputSpectra2->resize(10,215,290,85);
		playbackData->p->outputSpectra3->resize(310,215,290,85);
		playbackData->p->outputSpectra4->hide();
		playbackData->p->outputSpectra5->hide();
		playbackData->p->outputSpectra6->hide();
		playbackData->p->outputSpectra7->hide();
        break;
    case 5:
		playbackData->p->rearLeftOpen->hide();
		playbackData->p->rearRightOpen->hide();
		playbackData->p->centerOpen->hide();
        playbackData->p->processingWindow->size(610,400);
		playbackData->p->specBox->size(600, 365);
		playbackData->p->inputSpectraL->resize(10,35,290,85);
		playbackData->p->inputSpectraR->resize(310,35,290,85);
		playbackData->p->outputSpectra0->resize(10,125,290,85);
		playbackData->p->outputSpectra1->resize(310,125,290,85);
		playbackData->p->outputSpectra2->resize(10,215,290,85);
		playbackData->p->outputSpectra3->resize(310,215,290,85);
        playbackData->p->outputSpectra4->resize(10,305,290,85);
		playbackData->p->outputSpectra5->hide();
		playbackData->p->outputSpectra6->hide();
		playbackData->p->outputSpectra7->hide();
        break;
    case 6:
		playbackData->p->middleLeftOpen->hide();
		playbackData->p->middleRightOpen->hide();
        playbackData->p->processingWindow->size(610,400);
		playbackData->p->specBox->size(600, 365);
		playbackData->p->inputSpectraL->resize(10,35,290,85);
		playbackData->p->inputSpectraR->resize(310,35,290,85);
		playbackData->p->outputSpectra0->resize(10,125,290,85);
		playbackData->p->outputSpectra1->resize(310,125,290,85);
		playbackData->p->outputSpectra2->resize(10,215,290,85);
		playbackData->p->outputSpectra3->resize(310,215,290,85);
        playbackData->p->outputSpectra4->resize(10,305,290,85);
		playbackData->p->outputSpectra5->resize(310,305,290,85);
		playbackData->p->outputSpectra6->hide();
		playbackData->p->outputSpectra7->hide();
        break;
    case 7:
    	playbackData->p->centerOpen->hide();
    	playbackData->p->outputSpectra7->hide();
        break;
    case 8:
        break;
    }

	//playbackData->p->processingWindow->resizable(playbackData->p->processingWindow);
	out = (int*) malloc(FRAME_BLOCK_LEN*sizeof(int)*info->maxOutputChannels);

    //Pa_StartStream(playbackData->audioStream);
    return 0;
}

void* madPlay(void* gui){
    int error=0;
    TestPlayerGui *p = (TestPlayerGui*) gui;

	#ifdef WIN32
		HWND callback_target = fl_xid(p->MainWindow);
		HWND* target=&callback_target;
		_beginthread(refresh,0,(void*)target);
	#endif

    /** MAD Decoder setup*/
    FILE* mp3;
    mp3=fopen(p->filename,"rb");
    struct stat filestat;
    int fd = fileno(mp3);
    fstat(fd, &filestat);
    unsigned long filesize=filestat.st_size;
    void *fdm;
    fdm = mmap(0, filesize, PROT_READ, MAP_SHARED, fd, 0);
    if (fdm == MAP_FAILED){
        fl_message("Can't open the file!");
                return 0;
    }
    playbackData->fdm= (const unsigned char*)fdm;
    playbackData->length=filesize;
    playbackData->p=p;
	p->slider->setMad(playbackData);
    unsigned int currentSampleRate=playbackData->samplerate;
    mad_decoder_init(playbackData->decoder, playbackData,
        mad_input_cb, 0 /* header */, 0 /* filter */, mad_output_cb,
		mad_error_cb, 0 /* message */);
    /** End of Decoder setup*/
    if(currentSampleRate!=(unsigned int)playbackData->samplerate){
		Pa_Terminate();
		error=pa_init();
    }

    if(error){
        fl_message("Can't open audio device!");
        mad_decoder_finish(playbackData->decoder);
        p->isPlaying = false;
        p->isStopped = false;
        fclose(mp3);
        Pa_Terminate();
        playbackData->audioStream=NULL;
		//p->currentPos->value(0.d);
		//setpos(p, TXTPOS);
        return 0;
    }
    /** start playing audio*/
    p->isPlaying = true;
    mad_decoder_run(playbackData->decoder, MAD_DECODER_MODE_SYNC);
    printf("finished!\n");
    munmap(fdm, filesize);
    fclose(mp3);
    //playback_info->audioStream=NULL;
    //playback_info->callback_target=NULL;
    //p->currentPos->value(0.d);
    //setpos(p, TXTPOS);
    //Pa_Terminate();
    //_endthread();

	#ifdef WIN32
		target=NULL;
	#endif
}


void setpos(void *gui, VALUATOR val){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	double pos;
	switch(val) {
	case SLID:
		pos = p->slider->value();
		p->isStopped=true;
		if(1==p->libToUse)
			while(p->fin)
				p->isStopped=true;
		else if(2==p->libToUse)
			while(playbackData->decoder->sync)
				p->isStopped=true;
		else{
			fl_message("Not playng any file.");
			pos=0;
		}
		p->currentPos->value(pos);
		//p->slider->value(pos);
		p->remaining->value(p->duration->value() - pos );
				printf("Test: %f\n",p->duration->value());
		break;
	case TXTPOS:
		pos = p->currentPos->value();
		p->slider->value(pos);
		p->remaining->value(p->duration->value() - pos );
		break;
	}
	if (pos > p->slider->maximum())
		pos = p->slider->minimum();
	p->currentPos->value(pos);
	if(p->libToUse==2){
        unsigned long frame_number;
        frame_number = (unsigned long) pos * playbackData->samplerate / playbackData->samples_per_frame;
        playbackData->currentFrame=frame_number;
    }
	if (p->isPlaying){
		p->isPlaying=false;
		p->isStopped=false;
		play(p);
	}
}


void* playSND(void *gui){
    //p->fin=sf_open(p->filename, SFM_READ, &p->input_info);
    TestPlayerGui *p = (TestPlayerGui*) gui;
	#ifdef WIN32
		HWND callback_target = fl_xid(p->MainWindow);
		HWND* target=&callback_target;
		_beginthread(refresh,0,(void*)target);
	#endif

    volatile sf_count_t readFrames, currentFrame=0;
    SF_INFO sf_props;
    int* audio;
    int error=0;
    audio = (int*) malloc(2*FRAME_BLOCK_LEN*sizeof(int));
    double pos = p->currentPos->value();
    p->fin=sf_open(p->filename, SFM_READ, &sf_props);
    if(p->fin==NULL){
        fl_message("Could not open file %s\n",p->filename);
        return 0;
    }
    printf("Channels: %d\n", sf_props.channels);
    printf("Format: %x\n", sf_props.format);
    printf("Frames: %d\n", sf_props.frames);
    printf("Samplerate: %d\n", sf_props.samplerate);
    playbackData->samplerate=sf_props.samplerate;
    printf("Sections: %d\n", sf_props.sections);
    printf("Seekable: %d\n", sf_props.seekable);
    currentFrame=(sf_count_t) pos * sf_props.samplerate;
    sf_seek(p->fin,currentFrame,SEEK_SET);
    printf("SND Error: %d\n",error);
    p->isPlaying=true;
    p->isStopped=false;
    if(playbackData->samplerate!=sf_props.samplerate){
        Pa_Terminate();
        playbackData->samplerate=sf_props.samplerate;
        error=pa_init();
    }

    if(error){
        fl_message("Can't open audio device!");
        p->isPlaying = false;
        p->isStopped = false;
        Pa_Terminate();
        return 0;
    }

	p->slider->setMad(playbackData);

    while(readFrames>0 && currentFrame<sf_props.frames && p->isPlaying && !p->isStopped){
		double volumeLevel=p->volumeSlider->value();
        readFrames=sf_readf_int(p->fin,audio,FRAME_BLOCK_LEN);


        if(sf_props.channels==2){
			int i;
			for(i=0;i<readFrames;i++){
				p->inputData[0][i]=(int) audio[2*i];
				p->inputData[1][i]=(int) audio[2*i+1];
			}
        }else{
        	memcpy(p->inputData[0], audio, FRAME_BLOCK_LEN*sizeof(int));
        	memcpy(p->inputData[1], audio, FRAME_BLOCK_LEN*sizeof(int));
        }
		audioOutput();




        //Pa_WriteStream(playbackData->audioStream, audio, readFrames);
        currentFrame+=readFrames;
		if(!playbackData->slider_in_use){
			pos=(double) currentFrame / sf_props.samplerate;
			p->currentPos->value(pos);
			p->slider->value(pos);
			p->remaining->value(p->duration->value() - pos );
		}
    }
    printf("Volume %f\n",p->volumeSlider->value());
    sf_close(p->fin);
    p->fin=NULL;
}

void play(void *gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
    if ( p->filename[0] == '\0'   ) {  // needs a command line argument
		fl_message("Choose an audio file prior to play it!");
		return;
	}
	if(!p->isPlaying){
        if(p->libToUse==1)
            fl_create_thread(play_thread, playSND, gui);
        else if(p->libToUse==2)
            fl_create_thread(play_thread, madPlay, gui);
        else
            fl_message("Can't play files of this type!");
	}else
		fl_message("Still playing current audio file. Wait!");
}

void stop(void *gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	if(!p->isPlaying)
		return;
	if (p->isPlaying) p->isStopped=true;
    if(1==p->libToUse)
        while(p->fin)
            p->isStopped=true;
    else if(2==p->libToUse)
        while(playbackData->decoder->sync)
            p->isStopped=true;
    p->isStopped=false;
	//Pa_Terminate();
	p->isPlaying=false;
}

void rew(void *gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	if (p->isPlaying) p->isStopped=true;
	double pos = p->currentPos->value();
	pos -= p->ffStep;
	if (pos < 0) pos = 0;
	p->currentPos->value(pos);
	p->slider->value(pos);
	p->currentPos->value(pos);
}

void ff(void *gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	if (p->isPlaying) p->isStopped=true;
	double pos = p->currentPos->value();
	pos += p->ffStep;
	if (pos > p->slider->maximum()) pos = p->slider->maximum();
	p->currentPos->value(pos);
	p->slider->value(pos);
	p->currentPos->value(pos);
}

void setVolume(void* gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	int pos=(int) 10*(p->volumeSlider->value() - 1)+0.5;
	char* label=(char*)malloc(50*sizeof(char));
	sprintf(label, "\tVolume: %d\t",pos);
	//printf("  Volume: %f\n",p->volumeSlider->value());
	p->volumeSlider->label(label);
}

void showInfo(void* gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	p->infoWindow->show();
	p->okButton->take_focus();
}

void openFiltWindow(void* gui, OUTPUT t){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	p->inputChan->value(filtLayout[t].inputChannel);
	p->cutOff->value(filtLayout[t].cutoffFreq);
	p->bandWidth->value(filtLayout[t].bandWidth);
	char label[50]={0};
	sprintf(label, "\tCutoff frequency: %.0f\t", (filtLayout[t].cutoffFreq));
	p->cutOff->copy_label(label);
    sprintf(label, "\tBandwidth: %.0f\t", p->bandWidth->value());
	p->bandWidth->copy_label(label);
	p->filterChoice->value((int) filtLayout[t].filtType);
	p->data->copy_label((char*)&t);
	switch(t){
	case LEFT:
		p->filtConfigWindow->copy_label("Left");
		break;
	case RIGHT:
		p->filtConfigWindow->copy_label("Right");
		break;
	case REAR_LEFT:
		p->filtConfigWindow->copy_label("Rear Left");
		break;
	case REAR_RIGHT:
		p->filtConfigWindow->copy_label("Rear Right");
		break;
	case SUBWOOFER:
		p->filtConfigWindow->copy_label("Subwoofer");
		break;
	case CENTER:
		p->filtConfigWindow->copy_label("Center");
		break;
	case MIDDLE_LEFT:
		p->filtConfigWindow->copy_label("Middle Left");
		break;
	case MIDDLE_RIGHT:
		p->filtConfigWindow->copy_label("Middle Right");
		break;
	}
	p->filtConfigWindow->show();
	//setFreq(p);
	//p->okButton->take_focus()
}

void setFilt(void* gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	int i;
    short inChan=(short)p->inputChan->value();
    short outChan = p->data->label()[0];
    FILT_TYPE filtType = (FILT_TYPE) p->filterChoice->value();
    double cutOff = p->cutOff->value();
    double bandWidth = p->bandWidth->value();

    filtLayout[outChan].inputChannel=inChan;

    if(filtType<BW_LP){
        filtLayout[outChan].filtType=filtType;
        p->respChart->clear();
        	p->phaseChart->clear();
			for(i=0;i<512;i++){
				p->respChart->add(0,0,0);
				p->phaseChart->add(0,0,0);
			}
        p->filtConfigWindow->hide();
        return;
    }

    if(filtLayout[outChan].filtType!=filtType){
		filtLayout[outChan].filtType=filtType;
        free(filtLayout[outChan].filter);
        filtLayout[outChan].filter==NULL;
        filtLayout[outChan].filter=new_filter();
        filtLayout[outChan].cutoffFreq=cutOff;
        filtLayout[outChan].bandWidth=bandWidth;
        if(!filtLayout[outChan].filter){
            fl_message("Unable to create filter");
            return;
        }
        bw_init(filtLayout[outChan].filter, (FILTERTYPE)(filtType-2), playbackData->samplerate,cutOff,bandWidth);
        setFreq(p);
        //filtLayout[outChan].filter
    }
    BUTTERWORTH* bwf=filtLayout[outChan].filter;
    if(filtLayout[outChan].cutoffFreq!=cutOff || filtLayout[outChan].bandWidth!=bandWidth){
        bwf->func(&bwf->coeffs,cutOff,bandWidth,bwf->pidivsr,bwf->twopidivsr);
        filtLayout[outChan].cutoffFreq=cutOff;
        filtLayout[outChan].bandWidth=bandWidth;
    }
	//setFreq(p);
	p->respChart->clear();
	p->phaseChart->clear();
	for(i=0;i<512;i++){
		p->respChart->add(0,0,0);
		p->phaseChart->add(0,0,0);
	}
    p->filtConfigWindow->hide();
}

void setFreq(void* gui){
	TestPlayerGui *p = (TestPlayerGui*) gui;
	char label[50]={0};
	sprintf(label, "\tCutoff frequency: %.0f\t", p->cutOff->value());
	p->cutOff->copy_label(label);
	sprintf(label, "\tBandwidth: %.0f\t", p->bandWidth->value());
	p->bandWidth->copy_label(label);

	if(p->filterChoice->value()<2)
		return;
	BUTTERWORTH* filter=new_filter();
	if(!filter){
		fl_message("Unable to calculate filter");
		return;
	}
	bw_init(filter, (FILTERTYPE)(p->filterChoice->value()-2), playbackData->samplerate,p->cutOff->value(),p->bandWidth->value());
	double *resp,*phase;
	resp=(double*)malloc(1024*sizeof(double));
	phase=(double*)malloc(1024*sizeof(double));
	filtResponse(filter, resp, phase);
	int i;
	p->respChart->autosize(1);
	for (i=0;i<512;i++){
		p->respChart->replace(i,resp[i],0, FL_GREEN);
		p->phaseChart->replace(i,phase[i],0, FL_GREEN);
	}

}


int audioOutput(){
    int i,j;
    for(i=0;i<info->maxOutputChannels;i++){
        applyFilter(playbackData->p->inputData[filtLayout[i].inputChannel], &filtLayout[i], playbackData->p->outputData[filtLayout[i].outputChannel]);
    }
    double volume=log10(playbackData->p->volumeSlider->value());

	for(i=0;i<FRAME_BLOCK_LEN;i++){
		for(j=0;j<info->maxOutputChannels;j++)
			out[i*info->maxOutputChannels + j]=(int)playbackData->p->outputData[j][i]*volume;
	}
    PaError err= Pa_WriteStream(playbackData->audioStream, out, FRAME_BLOCK_LEN);
    return err;
}

void applyFilter(int *inputData, audioData *layout, int *outputData){
    int i;
	if(layout->filtType==PLAIN)
		memcpy(outputData,inputData,FRAME_BLOCK_LEN*sizeof(int));
    else if(layout->filtType==NONE)
		memset(outputData,1,FRAME_BLOCK_LEN*sizeof(int));
    else
        for(i=0;i<FRAME_BLOCK_LEN;i++)
            outputData[i]=bw_tick(layout->filter, inputData[i],layout->cutoffFreq, layout->bandWidth);
}

int main(){
	TestPlayerGui *gui = new TestPlayerGui;
	playbackData=mpdata_init();
	gui->slider->setMad(playbackData);
	gui->show();
	setVolume(gui);
	playbackData->p=gui;
	playbackData->processingWindow=gui->processingWindow;
    pa_init();
	setupSpectras(gui);
	_beginthread(plotSpectras,0,gui);
	return Fl::run();
}
