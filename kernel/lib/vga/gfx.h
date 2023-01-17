#ifndef VGA_GFX_H
#define VGA_GFX_H

#include <math.h>

class Edge
{
public:

    uint64_t Ax, Ay, Bx, By;

    Edge(uint64_t x, uint64_t y, uint64_t x1, uint64_t y1)
    {
        this->Ax  = x;
        this->Ay  = y;
        this->Bx = x1;
        this->By = y1;
    }

    /*
    Returns 2 if the point is on a vertex, else returns 1 if the point is on the side and 0 if not.
    */

    int isPointOnEdge(uint64_t Cx, uint64_t Cy)
    {

        if(((Cx == Ax) && (Cy == Ay)) || ((Cx == Bx) && (Cy == By)))
        {
            return 2;
        }

        /*
        dist = (|Ax(By-Cy) + Bx(Cy-Ay) + Cx(Ay-By)|) / (sqrt((Bx-Ax)^2 + (By-Ay)^2))
        Where the point P is (x,y) and line AB is defined by (Ax,Ay), (Bx,By)
        */

        double result = fabs(Ax * (By - Cy) + Bx * (Cy - Ay) + Cx * (Ay - By)) / sqrt(pow(Bx - Ax, 2) + pow(By - Ay, 2));
        if(result == 0.0)
        {
            return 1;
        }

        return 0;
    }

};


#endif
