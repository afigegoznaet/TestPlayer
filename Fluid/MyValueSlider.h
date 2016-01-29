// generated by Fast Light User Interface Designer (fluid) version 1.0303

#ifndef MyValueSlider_h
#define MyValueSlider_h
#include <FL/Fl.H>
#include <FL/Fl_Slider.H>
#include <FL/Fl_Value_Output.H>
#include <FL/Fl_Double_Window.H>
#include "PlayerGUI_prot.h"

/** Obsolete
class MyWindow : public Fl_Double_Window {
	using Fl_Double_Window::Fl_Double_Window;
private:
	void* playback_data=NULL;
public:
	void setMad(void* playback_data);
	void show();
	void resize(int a,int b,int c,int d);
	void hide();
	int handle(int e);
};
*/
class MyValueSlider : public Fl_Slider {
private:
	void* playbackData=NULL;
public:
	void setMad(void* playbackData);
public:
	MyValueSlider(int x,int y,int w,int h,const char*l=0);
	int handle(int e);
};

class Fl_Time_Output : public Fl_Value_Output {
public:
	Fl_Time_Output(int X, int Y, int W, int H, const char *L = 0) ;
	int format(char *buffer);
};
#endif
