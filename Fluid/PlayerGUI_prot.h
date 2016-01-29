#ifndef GUI_PROT
#define GUI_PROT
#include <FL/Fl_File_Chooser.H>
#include "../filters.h"

enum VALUATOR { SLID, TXTPOS, TXTDUR, ENDED};
enum SPECTRA {INSPEC, OUTSPEC };
enum FILT_TYPE { NONE, PLAIN, BW_LP, BW_HP, BW_BP, BW_BR, CT1, CT2, EL, AP};
enum OUTPUT { LEFT, RIGHT, REAR_LEFT, REAR_RIGHT, SUBWOOFER, CENTER ,MIDDLE_LEFT, MIDDLE_RIGHT};

struct audioData{
    short inputChannel;
    FILT_TYPE filtType;
    short outputChannel;
    double cutoffFreq;
    double bandWidth;
    BUTTERWORTH* filter;
};

void load(void *);
void play(void *);
void stop(void *);
void ff(void *);
void rew(void *);
void setFileName(void *);
void setVolume(void*);
void setpos(void *, VALUATOR t);
void toggleProcWindow(void *);
void setButton(void *);
void handleWindows(void *);
void showInfo(void *);
void openFiltWindow(void *, OUTPUT t);
void setFilt(void *);
void applyFilter(int *inputData, audioData *layout, int *outputData);
void setFreq(void*);
int audioOutput(void);
#endif
