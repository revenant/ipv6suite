
//Copyright (c) 2003, Nicola Concer
//All rights reserved.

#ifndef RANDOM_WALK_H
#define RANDOM_WALK_H

//#include "costants.h"
// #include "physic.h"
#include "randomWP.h"

class RandomWalk : public RandomWP //: public cSimpleModule
{
  friend class MobilityRandomWalk;
  
	//Macro that contains the costructor,destructor
	//and other Omnett++ stuff
//  Module_Class_Members(RandomWalk,MobilityHandler,0);
  RandomWalk(void);
  ~RandomWalk(void){}

  
//	virtual void initialize();
//	virtual void handleMessage(cMessage* );
//	virtual void finish();
	//
	
   private:
	//implement the mvement that sims a torus
	bool torus(int&, int&);

	// implement the rebound movement
	bool rebound(int&, int&);
    
	//returns the time intervall
	//to the next move
	double randomWalk(int&, int&);

    bool moveKind;

//	cPar* pauseTime;
//	cPar* moveKind;
//	cPar* distance;
//  cPar* moveInterval;
//  cPar* minSpeed;
//	cPar* maxSpeed;

    //direction angle 
	double alfa;

	double speed;
	
	//pointer of the physic module wich
	//list of the neighbours
	//store the actual <x,y> position
//	Physic* physic;
};
	
#endif	
