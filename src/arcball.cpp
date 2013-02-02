/** KempoApi: The Turloc Toolkit *****************************/
/** *    *                                                  **/
/** **  **  Filename: arcball.cpp                           **/
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
/**   12/28/2012 - sss, adapted for gameplay
/*************************************************************/
/*
 MIT License:
 
 Permission is hereby granted, free of charge, to any person obtaining a copy
 of this software and associated documentation files (the "Software"), to deal
 in the Software without restriction, including without limitation the rights
 to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:
 
 The above copyright notice and this permission notice shall be included in
 all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 THE SOFTWARE.
 */

#include "arcball.h"

//Arcball sphere constants:
//Diameter is       2.0f
//Radius is         1.0f
//Radius squared is 1.0f

//Create/Destroy
ArcBall::ArcBall(float NewWidth, float NewHeight)
{
    //Clear initial values
    this->StVec.x     =
    this->StVec.y     =
    this->StVec.z     =
    
    this->EnVec.x     =
    this->EnVec.y     =
    this->EnVec.z     = 0.0f;
    
    //Set initial bounds
    this->setBounds(NewWidth, NewHeight);
}

void ArcBall::_mapToSphere(const Vector2 * NewPt, Vector3 * NewVec) const
{
    Vector2 TempPt;
    float length;
    
    //Copy paramter into temp point
    TempPt = *NewPt;
    
    //Adjust point coords and scale down to range of [-1 ... 1]
    TempPt.x  =        (TempPt.x * this->AdjustWidth)  - 1.0f;
    TempPt.y  = 1.0f - (TempPt.y * this->AdjustHeight);
    
    //Compute the square of the length of the vector to the point from the center
    length      = (TempPt.x * TempPt.x) + (TempPt.y * TempPt.y);
    
    //If the point is mapped outside of the sphere... (length > radius squared)
    if (length > 1.0f)
    {
        float norm;
        
        //Compute a normalizing factor (radius / sqrt(length))
        norm    = 1.0f / sqrt(length);
        
        //Return the "normalized" vector, a point on the sphere
        NewVec->x = TempPt.x * norm;
        NewVec->y = TempPt.y * norm;
        NewVec->z = 0.0f;
    }
    else    //Else it's on the inside
    {
        //Return a vector to a point mapped inside the sphere sqrt(radius squared - length)
        NewVec->x = TempPt.x;
        NewVec->y = TempPt.y;
        NewVec->z = sqrt(1.0f - length);
    }
}

//Mouse down
void    ArcBall::click(Vector2 pt)
{
    //Map the point to the sphere
    this->_mapToSphere(&pt, &this->StVec);
}

#define Epsilon 1.0e-5

//Mouse drag, calculate rotation
void    ArcBall::drag(Vector2 pt, Quaternion* NewRot)
{
    //Map the point to the sphere
    this->_mapToSphere(&pt, &this->EnVec);
    
    //Return the quaternion equivalent to the rotation
    if (NewRot)
    {
        //Compute the vector perpendicular to the begin and end vectors
        //Vector3fCross(&Perp, &this->StVec, &this->EnVec);
        Vector3  Perp (StVec);
        Perp.cross(EnVec);
        
        //Compute the length of the perpendicular vector
        if (Perp.length() > Epsilon)    //if its non-zero
        {
            //We're ok, so return the perpendicular vector as the transform after all
            NewRot->x = Perp.x;
            NewRot->y = Perp.y;
            NewRot->z = Perp.z;
            //In the quaternion values, w is cosine (theta / 2), where theta is rotation angle
            
            NewRot->w = StVec.dot(EnVec);
        }
        else                                    //if its zero
        {
            //The begin and end vectors coincide, so return an identity transform
            NewRot->x =
            NewRot->y =
            NewRot->z =
            NewRot->w = 0.0f;
        }
    }
}