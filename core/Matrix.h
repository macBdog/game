#ifndef _CORE_MATRIX_
#define _CORE_MATRIX_
#pragma once

#include <math.h>

#include "Vector.h"

//\brief Basic matrix with translation class
class Matrix
{
public:

	inline float & operator () ( unsigned int Row, unsigned int Col )          { return m[Row][Col]; }
	inline float &  operator () ( unsigned int Row, unsigned int Col ) const    { return m[Row][Col]; }

private:
	
   union 
   {
      float f[16];
      struct
      {
         Vector		right;
         Vector		look;
         Vector		up;
         Vector		pos;
      };
      float m[4][4];
      struct
      {
         float row0;
         float row1;
         float row2;
         float row3;
      };
      Vector row[4];	
   };

   inline void SetWColumnToIdentity()
   {
      m[0][3] = 0.0f;
      m[1][3] = 0.0f;
      m[2][3] = 0.0f;
      m[3][3] = 1.0f;
   }

   static const Matrix sc_identity;
};

