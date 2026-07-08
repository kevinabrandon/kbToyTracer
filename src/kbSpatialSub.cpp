//
//	kbSpatialSub.cpp
//
//	Aggregate Class - Implements a spatial subdivision acceleration method
//
//	Kevin Brandon
//

#include "kbSpatialSub.h"
#include <chrono>

Voxel::Voxel()
{
	// when a voxel is created it initially has no objects
	// in its list
	numObjs = 0;
	
	// if there are objects within the voxel, 
	// then a list will be dynamiclly created
	// until then we set the list pointer to NULL
	list = NULL;
}

SpatialSub::SpatialSub()
{
	listOfObjects = new List();
	builtLattice = false;
}

SpatialSub::~SpatialSub()
{
	// delete all lists that were 'newed'

	delete listOfObjects;
	for(int i = 0; i < N*N*N; i++)
	{
		if(voxel[i].numObjs != 0)
		{
			delete voxel[i].list;
		}
	}
}

void SpatialSub::AddChild( Object * obj )
{
	// simply add the object to the list... 
	// we will add it to the voxels later
	listOfObjects->AddChild( obj );
}

bool SpatialSub::Intersect(const Ray &ray, HitInfo &hitinfo) const
{
	// If this is the first time Intersect is called
	// then the lattic has not been built.  Here we
	// check to see if it is the first time and if
	// it is, then we go ahead and build the lattice.
	std::call_once( latticeOnce, [this]{ BuildLattice(); } );
	
	bool intersect = false;	
	
	// A list of indices
	int list[2000];
	
	// Here we get a list of indices of the voxels
	// that are along the ray.  Length is the 
	// number of voxels along the ray
	int length = ListOfVoxelsOnRay(ray, list);
	
	// for each voxel along the ray, check each object
	// within the voxel to see if there is an intersection
	for(int i = 0; i < length; i++)
	{	
		if(voxel[list[i]].list->Intersect(ray, hitinfo))
		{
			intersect = true;
		}
	}

	return intersect; 
}

void SpatialSub::BuildLattice() const
{
	// used for timing how long it takes to build the lattice
	auto liStart = std::chrono::high_resolution_clock::now();
	
	std::cout << "building lattice for the spatial subdivision!" << std::endl;

	// bounding box of the entire scene
	Box3 sceneBBox = listOfObjects->GetBounds();

	// size of the box if min was at origin
	Vec3 difference = sceneBBox.Max() - sceneBBox.Min();
	
	if(debug)
	{
		std::cout << "Scene Min: " << sceneBBox.Min() << std::endl;
		std::cout << "Scene Max: " << sceneBBox.Max() << std::endl;
		std::cout << "\nSize of whole scene: "<< difference << std::endl;
	}
	
	// size of each voxel
	difference = difference / N;
	
	if(debug)	std::cout << "Size of voxels: " << difference << std::endl;
	
	// first voxel
	voxel[0].Min = sceneBBox.Min();
	voxel[0].Max = sceneBBox.Min() + difference;
	
	if(debug)
	{
		std::cout << "Voxel #0:" << std::endl;
		std::cout <<  "Min: " << voxel[0].Min << std::endl;
		std::cout <<  "Max: " << voxel[0].Max << std::endl;
	}
	
	// the amount difference for each axis
	Vec3 Xdiff, Ydiff, Zdiff;
	Xdiff = Ydiff = Zdiff = difference;
	Xdiff.y = 0;	Xdiff.z = 0;
	Ydiff.x = 0;	Ydiff.z = 0;
	Zdiff.x = 0;	Zdiff.y = 0;

	for(int i = 1 ; i < N*N*N; i++)
	{
		if( ( (i) % (N*N) ) == 0)
		{
			// increment the z axis and decrement the x and y axises
			voxel[i].Max = voxel[i-1].Max - ((N-1) * Xdiff) - ((N-1) * Ydiff) + Zdiff;
			voxel[i].Min = voxel[i-1].Min - ((N-1) * Xdiff) - ((N-1) * Ydiff) + Zdiff;
		}
		else if( ( (i) % N ) == 0)
		{
			// increment the y axis and decrement the x axis
			voxel[i].Max = voxel[i-1].Max - ((N-1) * Xdiff) + Ydiff;
			voxel[i].Min = voxel[i-1].Min - ((N-1) * Xdiff) + Ydiff;	
		}
		else
		{
			// increment the x axis
			voxel[i].Max = voxel[i-1].Max + Xdiff;
			voxel[i].Min = voxel[i-1].Min + Xdiff;
		}

		if(debug)
		{
			std::cout << "voxel #" << i << std::endl;
			std::cout << "Min: " << voxel[i].Min << std::endl;
			std::cout << "Max: " << voxel[i].Max << std::endl;
		}
	}

	// the lattice is now built

	// here we add all the objects to the lattice	
	AddObjectsToVoxels();

	// timing 
	auto liFinish = std::chrono::high_resolution_clock::now();
	double time;
	time = std::chrono::duration<double, std::micro>(liFinish - liStart).count();
	time = time / 1000;

	std::cout << "time to construct the lattice and add all objects:"<< std::endl;
	std::cout << "\t" << time << " (msec)" << std::endl;
	
	if(debug)
	{
		for(int i=0;i< N*N*N; i++)
		{
			if(voxel[i].numObjs != 0)
			{
				std::cout << "voxel[" << i << "] contains " << voxel[i].numObjs << " objects" << std::endl;
			}
		}
	}

	// the lattice is now complete
	builtLattice = true;	
}

void SpatialSub::AddObjectsToVoxels( )	const
{
	// Figgure out which voxel each object belongs to.
	
	int count = 0;

	// loop through all objects in the list
	for( List_node *node = listOfObjects->first; node != NULL; node = node->next)
	{

		Vec3 objMax = node->object->GetBounds().Max();
		Vec3 objMin = node->object->GetBounds().Min();

		if(debug)
		{		
			count++;
			std::cout << "\nobject #" << count << " " << node->object->MyNameIs() << "\n"<<std::endl;	
			std::cout << "objMin: " << objMin << std::endl;
			std::cout << "objMax: " << objMax << "\n" << std::endl;
		}
		
		// here we go through each voxel and check to see if the
		// object is within it.  If it is then we add the object
		// to the list for that particular voxel
		for(int i = 0; i < N*N*N; i++)
		{
			// here we check to see if the object is in the voxel
			if( BoxIntersect( voxel[i].Min, voxel[i].Max, objMin, objMax ) )
			{
				// The object is within this voxel,
				// add this object to the voxel's list.
				
				if(voxel[i].numObjs == 0)
				{	
					// This is the first object for this 
					// voxel.  We need to create a list.
					voxel[i].list = new List();
				}

				// Add object to the list.
				voxel[i].list->AddChild( node->object );

				voxel[i].numObjs++;
			
				if(debug)	std::cout << "add object to voxel #" << i << std::endl;
			}
		}
	}
}

int SpatialSub::ListOfVoxelsOnRay(const Ray &ray, int *list) const
{
	int length = 0;
	
	for(int i = 0; i < N*N*N; i++)
	{
		if(voxel[i].numObjs != 0)
		{
			// new hitinfo becuase we don't want just the closest one to the origin,
			// we want all of the voxels that lay on the infinitely long ray
			HitInfo temp;
			temp.ignore = NULL;
			temp.distance = Infinity;
			if(voxel[i].Intersect(ray, temp))  // box intersector (inheared from Box.cpp)
			{
				// 2000 is the max number of objects in a single voxel... 
				// could make it larger if neccisary
				if(length >= 2000)
				{
					std::cout << "error: too many objects in voxel #" << i << std::endl;
					break;
				}
				// there is a an intersection, so add the index
				// to the list of indices at the current length
				list[length] = i;

				length++;
			}
		}
	}
	// return the length of the list of indices
	return length;
}

void SpatialSub::Transform( const Mat3x4 &mat )
{
	// no transforming yet...
}

Object *SpatialSub::ReadString( const char *params )
{
	extern bool Get( const char *line, const char *name, const char *arg);
	if( Get( params, "begin", "SSub" ) ) return new SpatialSub();
	return NULL;
}

Box3 SpatialSub::GetBounds() const
{
	Box3 box;
	box = listOfObjects->GetBounds();
	return box;
}

static bool BoxIntersect( Vec3 firstMin, Vec3 firstMax, Vec3 secondMin, Vec3 secondMax)
{	
	// check to see if x interval intersects
	bool xInt = false;
	
	if(firstMin.x < secondMin.x)
	{
		if(secondMin.x < firstMax.x)	xInt = true;
	}
	else
	{
		if(firstMin.x < secondMax.x)	xInt = true;
	}
	
	// check to see if y interval intersects
	bool yInt = false;

	if(firstMin.y < secondMin.y)
	{
		if(secondMin.y < firstMax.y)	yInt = true;
	}
	else
	{
		if(firstMin.y < secondMax.y)	yInt = true;
	}

	// check to see if z interval intersects
	bool zInt = false;
	if(firstMin.z < secondMin.z)
	{
		if(secondMin.z < firstMax.z)	zInt = true;	
	}
	else
	{
		if(firstMin.z < secondMax.z)	zInt = true;
	}

	// if all three intersect then there is an intersection
	if(xInt && yInt && zInt)	return true;
	else return false;
}   

