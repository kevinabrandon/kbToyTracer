//
//	SpacialSub.h
//
//	Aggregate Class
//
//	Kevin Brandon
//
#ifndef _SPACIALSUB_H_
#define _SPACIALSUB_H_

#include "toytracer.h"
#include "List.h"
#include "Box.h"
#include <mutex>

#define N 12	//	N^3 is the number of voxels in the scene
#define debug 0

struct Voxel : public Box
{
	Voxel();

	//	each voxel contains a list of all 
	//	the objectes within the voxel
	List *list;

	//	The number of objects inside the voxel
	int numObjs;

};

struct SpacialSub : public Aggregate 
{ 
    SpacialSub();
   ~SpacialSub();
    virtual bool Intersect( const Ray &ray, HitInfo & ) const;
    virtual Box3 GetBounds() const;
    virtual Object *ReadString( const char *params );
    virtual const char *MyNameIs() const { return "Spacial Subdivision"; }
    virtual void AddChild( Object * );
    virtual void Transform( const Mat3x4 & );
	
	//	BuildLattice builds the "lattice".
	//	must be declared const becuase it is called
	//	in Intersect() which is also declared const
	void BuildLattice() const;	

	//	Loops through all the objects in listOfObjects
	//	and puts each Object in the Voxels it belongs to
	//	must be const becuase it is called in BuildLattice() 
	//	which is also declared const
	void AddObjectsToVoxels( ) const;

	//	A flag that keeps track if the lattice has been
	//	built yet.  (The constructor sets it to false
	//	and it is set to true when BuildLattice() is called.
	//	mutable becuase must be changed in const methods
	mutable bool builtLattice;
	mutable std::once_flag latticeOnce;   // build the lattice exactly once, thread-safely
	
	//	The Voxels
	//	mutable becuase must be changed in const methods
	mutable Voxel voxel[N*N*N];	
	

	//	This returns a list of indices of the voxels that
	//	intersect the ray.  It returns the length of the list
	int ListOfVoxelsOnRay(const Ray &ray, int *list) const;

	//	The list of objects that each 
	//	object is initially stored into.
	List *listOfObjects;
};

// Here is another box intersector... this was needed so
// that I could also intersect a single point with a box.
// This is done by making both a min and max equal to the point.
static bool BoxIntersect(Vec3 firstMin, Vec3 firstMax, Vec3 secondMin, Vec3 secondMax);

// Register the new object with the toytracer.
// When this module is linked in, the toytracer 
// will automatically recognize the new objects 
// and read them from sdf files.
REGISTER_OBJECT( SpacialSub );

#endif
