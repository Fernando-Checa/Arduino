/*
  MyBuffer.h - Library for queuing MySensor messages.
*/
#ifndef MyBuffer_h
#define MyBuffer_h

#include "Arduino.h"


class MyBuffer {
  // desc
	public:
	   MyBuffer();
	  bool push(int sensor, int type);
	  String pop();
	  int getSize();
	  bool isEmpty();

	private:	
	  int myCounter;
	  int maxSize;
	  String myMsg[11];
};

#endif