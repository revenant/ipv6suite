
#include "randomWP.h"
#include <omnetpp.h>

//Define_Module_Like(RandomWP,Mobility);

double RandomWP::randomWaypoint(double& x, double& y)
{
  double distance;//,angle;
        double currentSpeed;
        double a,b,c,d;
        bool pause = false;

        a = x;
        b = y;

        //if the node has reached its destination,define anotherone
        //and rest for pauseTime
        if(steps == 0)
        {
                pause = true;
                currentSpeed = speed[moveIndex];
                c = destX[moveIndex];
                d = destY[moveIndex];

                distance = sqrt((double)((c-a)*(c-a))+((d-b)*(d-b))) ;

//              d("DISTANCE = "<<distance);
                if ( currentSpeed !=0 && moveIndex < MAX_MOVEMENTS )
                {
                        steps = (int)( (distance / currentSpeed ) / moveInterval);
                        steps = steps > 0 ? steps : 1; // avoid 0!
                        dX =((c-a) / steps);
                        dY =((d-b) / steps);

                }
                else
                {
                        steps = (int) intuniform(1,100, 0);
                        dX = 0;
                        dY = 0;
                }

                stepsNum += steps;
                partial += steps * currentSpeed;
                moveIndex++;
        }

        //define new <x,y>
        x = (x + dX) ;
        x = x > maxX ? minX : x;
        x = x < minX ? maxX : x;

        y = (y + dY);
        y = y > maxY ? minY : y;
        y = y < minY ? maxY : y;


        //do not go outside the map

        if(pause)
            return (double)pauseTime;
        else
        {
            steps--;
            return (double)moveInterval;
        }

}

void RandomWP::generateMovements(void)
{
  for (int i = 0; i < MAX_MOVEMENTS; i++)
  {
    destX[i] = intuniform(minX, maxX, 1);
    destY[i] = intuniform(minY, maxY, 2);
    speed[i] = uniform(minSpeed, maxSpeed, 3);
  }
}

RandomWP::RandomWP()
{
/*
        d("Random Way Point");
        cGate *g = gate("out");

        //pointer to the physic module that
        //store tje actual position

        physic =(Physic*) g->toGate()->ownerModule();

        minX = 5;
        maxX = (int)parentModule()->par("Xbound") -5;
        minY = 5;
        maxY = (int)parentModule()->par("Ybound") -5;


        moveInterval = &par("moveInterval");
        pauseTime = &par("pauseTime");
        moveKind = &par("movKind");
        maxSpeed = &par("maxSpeed");
        minSpeed = &par("minSpeed");

        cMessage *moveMsg = new cMessage("Move");

        //start miving
        scheduleAt(simTime()+0.01, moveMsg);*/

        steps = 0;

        //statistical variables
        stepsNum =0;
        partial =0;
        moveIndex = 0;
}


/*

void RandomWP::handleMessage(cMessage *msg)
{
        int x,y;

        d(" random WP");
        //get the current position from the physic module
        physic->getPos(x, y);

        //calcolate the new position
        double time =  randomWaypoint(x,y);

        cMessage *moveMsg = new cMessage("Move",MOVE);

        moveMsg->addPar("x") = x;
        moveMsg->addPar("y") = y;

        //inform to the physic module about
        //the new position so it can be displayed
        send(moveMsg,"out");

        //tell to the physic module to move
        scheduleAt(simTime()+time, msg);
}



void RandomWP::finish()
{
         FILE* fout = fopen("collcectedData.dat","a");

        d("Speed avarage..........:"<<partial/stepsNum);
        fprintf(fout,"\nSpeed avatage............... %.2f\n",partial/stepsNum);

        fclose(fout);
}

*/
