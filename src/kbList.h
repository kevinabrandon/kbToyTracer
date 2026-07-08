/***************************************************************************
* kbList.h (aggregate object)                                              *
*																		   *   
*		Header file for list.cpp										   *
*                                                                          *
* History:                                                                 *
*	10/20/2004	Broken out of list.c	                                   *
*                                                                          *
***************************************************************************/
#ifndef KB_LIST_H
#define KB_LIST_H

#include "toytracer.h"

// Define a small structure that will be used to create a simple linked list.
// Most aggregates will need at least one additional class such as this in
// order to create a tree, or a grid, etc.

struct List_node 
{
    Object    *object;
    Mat3x4     matrix;       // The affine transformation applied to this object.
    Mat3x4     inverse;      // The inverse of the affine transformation.
    bool       transformed;  // A flag indicating that a matrix has been specified.
    List_node *next;
};

// Define the actual List object as a sub-class of the "Aggregate" class.
// This sub-class must define all the necessary virtual methods as well as
// any special data members that are specific to this type of object.
// (ALL access to this object will be through these virtual methods.)

struct List : public Aggregate 
{ 
    List();
   ~List();
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "List"; }
    virtual void AddChild( Object * );
    virtual void Transform( const Mat3x4 & );
    List_node *first;
    List_node *last;
};


// Register the new object with the toytracer.  When this module is linked in, the 
// toytracer will automatically recognize the new objects and read them from sdf files.

REGISTER_OBJECT( List );

#endif
