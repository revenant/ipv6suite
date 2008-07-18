
//Copyright (c) 2003, Nicola Concer
//All rights reserved.

#ifndef MOBILITY_H
#define MOBILITY_H

#define MAX_MOVEMENTS 500000

// #include <omnetpp.h>
#include "costants.h"

//#include "physic.h"

class RandomWP // : public cSimpleModule
{
       friend class MobilityRandomWP;
       friend class MobilityRandomPattern;

       //Macro that contains the costructor,destructor
       //and other Omnett++ stuff
       //Module_Class_Members(RandomWP,cSimpleModule,0)

       // virtual void initialize();
       // virtual void handleMessage(cMessage* );
       // virtual void finish();

 protected:
       RandomWP(void);
        ~RandomWP(void){}

        //returns the time intervall
        //to the next move
        double randomWaypoint(double&, double&);
        void generateMovements(void);
        
        double moveInterval;
        double minSpeed;
        double maxSpeed;
        int distance;
        double pauseTime;

//      cPar* moveInterval;
//      cPar* pauseTime;
//      cPar* moveKind;

//      cPar* maxSpeed;
//      cPar* minSpeed;

        //pointer of the physic module wich
        //store the actual <x,y> position
//      Physic* physic;

        //size of the movement field
        int minX,maxX,minY,maxY;

        //direction flag
        double dX, dY;
        //number of steps to reach
        //the destination
        int steps;

        //statistics vars
        int stepsNum;
        double partial;

        double destX[MAX_MOVEMENTS];
        double destY[MAX_MOVEMENTS];
        double speed[MAX_MOVEMENTS];

        int moveIndex;
};

#endif
