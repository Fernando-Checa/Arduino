/*
  MyBuffer.cpp - Library for mysensors queue.
*/

#include "Arduino.h"
#include "MyBuffer.h"

  //Constructor
 MyBuffer::MyBuffer() {
  	myCounter=0;
   // mySize=0;
    maxSize=10; //limit to max 10 messages
  }

  bool MyBuffer::push(int sensor, int type) {
  	if(myCounter < maxSize){
  		 myCounter++;
  		 String msg = String(sensor) + ";" + String(type);
  		 myMsg[myCounter] = msg;
  		return true;
  	} else return false;
  }

  String MyBuffer::pop() {
  	if(myCounter>0){
  		myCounter--;
  		return myMsg[(myCounter + 1)]; 		
   	} else return "-1";
  }
  
  int MyBuffer::getSize() {
   	return myCounter;
  }
  
  bool MyBuffer::isEmpty() {
   if(myCounter==0) return true;
   return false;
  }