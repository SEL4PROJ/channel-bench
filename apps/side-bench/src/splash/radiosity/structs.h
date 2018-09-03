#line 124 "/home/qiang/src/splash2/splash2-master/codes/null_macros/c.m4.null.POSIX_sel4"

#line 1 "structs.H"
#ifndef _STRUCTS_H
#define _STRUCTS_H

/************************************************************************
*
*     Vertex    -  3D coordinate
*
*************************************************************************/

typedef struct {
	float x, y, z ;
} Vertex;

/************************************************************************
*
*     Color (R,G,B)
*
*************************************************************************/

typedef struct {
	float r, g, b ;
} Rgb;

/************************************************************************
*
*     Ray       -  3D coordinate
*
*************************************************************************/

typedef struct {
	float x, y, z;
} Ray;

#endif
