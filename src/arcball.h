/** KempoApi: The Turloc Toolkit *****************************/
/** *    *                                                  **/
/** **  **  Filename: ArcBall.h                             **/
/**   **    Version:  Common                                **/
/**   **                                                    **/
/**                                                         **/
/**  Arcball class for mouse manipulation.                  **/
/**                                                         **/
/**                                                         **/
/**                                                         **/
/**                                                         **/
/**                              (C) 1999-2003 Tatewake.com **/
/**   History:                                              **/
/**   08/17/2003 - (TJG) - Creation                         **/
/**   09/23/2003 - (TJG) - Bug fix and optimization         **/
/**   09/25/2003 - (TJG) - Version for NeHe Basecode users  **/
/**                                                         **/
/**   12/28/2012 - sss, adapted for gameplay                **/
/*************************************************************/

#ifndef _ArcBall_h
#define _ArcBall_h

#include "gameplay.h"

using namespace gameplay;

class ArcBall
{
protected:
    inline void _mapToSphere(const Vector2 * NewPt, Vector3 * NewVec) const;
    
public:
    //Create/Destroy
    ArcBall(float NewWidth, float NewHeight);
    ArcBall() { /* nothing to do */ };
    
    //Set new bounds
    inline
    void    setBounds(float NewWidth, float NewHeight)
    {
        //Set adjustment factor for width/height
        this->AdjustWidth  = 1.0f / ((NewWidth  - 1.0f) * 0.5f);
        this->AdjustHeight = 1.0f / ((NewHeight - 1.0f) * 0.5f);
    }
    
    //Mouse down
    void    click(Vector2 pt);
    
    //Mouse drag, calculate rotation
    void    drag(Vector2 pt, Quaternion* NewRot);
    
protected:
    Vector3   StVec;          //Saved click vector
    Vector3   EnVec;          //Saved drag vector
    float     AdjustWidth;    //Mouse bounds width
    float     AdjustHeight;   //Mouse bounds height
    
};

#endif
