/***************************************************************************
* box.C    (primitive object)                                              *
*                                                                          *
* The "Box" object defines an axis-aligned box.  Its constructor takes     *
* two vectors, which are taken to be the "min" and "max" corners of the    *
* box (i.e. the three min coords, and the three max coords).               *
*                                                                          *
* History:                                                                 *
*   10/10/2004  Broken out of objects.C file.                              *
*	10/22/2004	Kevin Brandon - took out declerations					   *
*				and put them in Box.h									   *
*                                                                          *
***************************************************************************/

#include "Box.h"

Box::Box( const Vec3 &Min_, const Vec3 &Max_ )
    {
    Min  = Min_;
    Max  = Max_;
    }

Object *Box::ReadString( const char *params ) // Read params from string.
    {
    float a, b, c, d, e, f;
    if( sscanf( params, "box (%f,%f,%f) (%f,%f,%f)", &a, &b, &c, &d, &e, &f ) == 6 )
        return new Box( Vec3( a, b, c ), Vec3( d, e, f ) );
    return NULL;
    }

Box3 Box::GetBounds() const // Returns a bounding box (which is just the box itself).
    {
    Box3 box;
    box.X = Interval( Min.x, Max.x );
    box.Y = Interval( Min.y, Max.y );
    box.Z = Interval( Min.z, Max.z );
    return box;
    }

// Jim Arvo, EECS204, Fall 2004

// Intersect a ray with an axis-aligned box.  Feel free to use
// this itersector in your toy tracer for assignments #2 and
// beyond.
bool Box::Intersect( const Ray &ray, HitInfo &hitinfo ) const
    {
    Vec3 R = ray.direction;
    Vec3 Q = ray.origin;
    Vec3 N;

    // Maintain an interval (min, max) that is inside the box as we process
    // successive slabs.  Start with min=0 and max=hitinfo.distance, to
    // represent the entire available portion of the ray.

    double min = 0.0;
    double max = hitinfo.distance;

    // For each component of the ray direction, check to see which way it is
    // pointed and see if it is directed toward or away from the slab.  If away,
    // return immediately, reporting a miss.  Each time min is updated set the
    // normal N as well.  The normal will then be set according to the face that
    // is most distant (which is the face of the actual box, the intersection
    // of three slabs).

    if( R.x >= 0 ) // Looking down positive X-axis.
        {
        if( Q.x > Max.x ) return false;
        double s = ( Min.x - Q.x ) / R.x;
        double t = ( Max.x - Q.x ) / R.x;
        if( s > min ) { min = s; N = Vec3( -1, 0, 0 ); }
        if( t < max ) { max = t;                       }
        }
    else // Looking down negative X-axis.
        {
        if( Q.x < Min.x ) return false;
        double s = ( Max.x - Q.x ) / R.x;
        double t = ( Min.x - Q.x ) / R.x;
        if( s > min ) { min = s; N = Vec3( 1, 0, 0 ); }
        if( t < max ) { max = t;                      }
        }
    if( min > max ) return false; // Degenerate interval.

    if( R.y >= 0 ) // Looking down positive Y-axis.
        {
        if( Q.y > Max.y ) return false;
        double s = ( Min.y - Q.y ) / R.y;
        double t = ( Max.y - Q.y ) / R.y;
        if( s > min ) { min = s; N = Vec3( 0, -1, 0 ); }
        if( t < max ) { max = t;                       }
        }
    else // Looking down negative Y-axis.
        {
        if( Q.y < Min.y ) return false;
        double s = ( Max.y - Q.y ) / R.y;
        double t = ( Min.y - Q.y ) / R.y;
        if( s > min ) { min = s; N = Vec3( 0, 1, 0 ); }
        if( t < max ) { max = t;                      }
        }
    if( min > max ) return false; // Degenerate interval.

    if( R.z >= 0 ) // Looking down positive Z-axis.
        {
        if( Q.z > Max.z ) return false;
        double s = ( Min.z - Q.z ) / R.z;
        double t = ( Max.z - Q.z ) / R.z;
        if( s > min ) { min = s; N = Vec3( 0, 0, -1 ); }
        if( t < max ) { max = t;                       }
        }
    else // Looking down negative Z-axis.
        {
        if( Q.z < Min.z ) return false;
        double s = ( Max.z - Q.z ) / R.z;
        double t = ( Min.z - Q.z ) / R.z;
        if( s > min ) { min = s; N = Vec3( 0, 0, 1 ); }
        if( t < max ) { max = t;                      }
        }
    if( min > max ) return false; // Degenerate interval.

    // We have an actual hit.  Fill in all the geometric information so
    // that the shader can shade this point.  Compute the point of
    // intersection using the ray and the distance to the point along
    // the ray.

    hitinfo.distance = min;
    hitinfo.point    = ray.origin + min * R;
    hitinfo.normal   = N;
    hitinfo.ray      = ray;
    hitinfo.object   = this;
    return true;
    }
/*
bool Box::Intersect( const Ray &ray, HitInfo &hitinfo ) const
    {
    Vec3 R = ray.direction;
    Vec3 Q = ray.origin;
    Vec3 N, N_max, N_min;

    // Maintain an interval (min, max) that is inside the box as we process
    // successive slabs.  Start with min=0 and max=hitinfo.distance, to
    // represent the entire available portion of the ray.

    double min = -Infinity;
    double max = hitinfo.distance;

    // For each component of the ray direction, check to see which way it is
    // pointed and see if it is directed toward or away from the slab.  If away,
    // return immediately, reporting a miss.  Each time min is updated set the
    // normal N as well.  The normal will then be set according to the face that
    // is most distant (which is the face of the actual box, the intersection
    // of three slabs).

	Vec2 uv_min();
	Vec2 uv_max();

    if( R.x >= 0 ) // Looking down positive X-axis.
    {
        if( Q.x > Max.x ) return false;
        double s = ( Min.x - Q.x ) / R.x;
        double t = ( Max.x - Q.x ) / R.x;
        if( s > min ) { min = s; N_min = Vec3( -1, 0, 0 );  }
        if( t < max ) { max = t; N_max = Vec3( 1, 0, 0 ); }
    }
    else // Looking down negative X-axis.
        {
        if( Q.x < Min.x ) return false;;
        double s = ( Max.x - Q.x ) / R.x;
        double t = ( Min.x - Q.x ) / R.x;
        if( s > min ) { min = s; N_min = Vec3( 1, 0, 0 ); }
        if( t < max ) { max = t; N_max = Vec3( -1, 0, 0 ); }
        }
    if( min > max ) return false; // Degenerate interval.

    if( R.y >= 0 ) // Looking down positive Y-axis.
        {
        if( Q.y > Max.y ) return false;
        double s = ( Min.y - Q.y ) / R.y;
        double t = ( Max.y - Q.y ) / R.y;
        if( s > min ) { min = s; N_min = Vec3( 0, -1, 0 ); }
        if( t < max ) { max = t; N_max = Vec3( 0, 1, 0 ); }
        }
    else // Looking down negative Y-axis.
        {
        if( Q.y < Min.y ) return false;
        double s = ( Max.y - Q.y ) / R.y;
        double t = ( Min.y - Q.y ) / R.y;
        if( s > min ) { min = s; N_min = Vec3( 0, 1, 0 ); }
        if( t < max ) { max = t; N_max = Vec3( 0, -1, 0 ); }
        }
    if( min > max ) return false; // Degenerate interval.

    if( R.z >= 0 ) // Looking down positive Z-axis.
        {
        if( Q.z > Max.z ) return false;
        double s = ( Min.z - Q.z ) / R.z;
        double t = ( Max.z - Q.z ) / R.z;
        if( s > min ) { min = s; N_min = Vec3( 0, 0, -1 ); }
        if( t < max ) { max = t; N_max = Vec3( 0, 0, 1 ); }
        }
    else // Looking down negative Z-axis.
        {
        if( Q.z < Min.z ) return false;
        double s = ( Max.z - Q.z ) / R.z;
        double t = ( Min.z - Q.z ) / R.z;
        if( s > min ) { min = s; N_min = Vec3( 0, 0, 1 ); }
        if( t < max ) { max = t; N_max = Vec3( 0, 0, -1 ); }
        }
    if( min > max ) return false; // Degenerate interval.

    // We have an actual hit.  Fill in all the geometric information so
    // that the shader can shade this point.  Compute the point of
    // intersection using the ray and the distance to the point along
    // the ray.

	double distance;

	if( min >= 0 )		
	{
		distance = min;
		N = N_min;
	}
	else				
	{
		distance = max;
		N = N_max;
	}

	Vec3 point( ray.origin + distance * R );
	



	hitinfo.uv = Vec2( 0, 0 );

	hitinfo.distance = distance;
	hitinfo.point    = point;

    hitinfo.normal   = N;
    hitinfo.ray      = ray;
    hitinfo.object   = this;
    return true;
    }

*/

//Set t_near = -infinity  and t_far = infinity  
//Let <x_d, y_d, z_d> the ray direction vector and (x_b1, y_b1, z_b1), (x_b2, y_b2, z_b2_  opposite corners defining the box 
//If x_d = 0, the ray is parallel to the x planes, so if x_o < x_b1 or x_o > x_b2 no intersection 
//Otherwise, the two intersections with the x planes of the box are t1 = (x_b1 - x_o)/x_d and t2 = (x_b2 - x_o)/x_d  
//If t1 > t2 swap (t1, t2)
//If t1 > t_near, then t_near = t1  
//If t2 < t_Far, then t_Far = t_2  
//If t_near > t_Far, then the box is missed 
//If t_far < 0, the box is behind the ray, so it is missed 
//Repeat for y and z pairs of parallel planes 


/*
bool Box::Intersect( const Ray &ray, HitInfo &hitinfo ) const
{
	double Smin[3], Smax[3], biggest = 0;
	Vec3 normal[3], finalNormal;
	double maxOfMin = -Infinity, minOfMax = Infinity;

	Smin[0] = ( Min.x - ray.origin.x ) / ray.direction.x;
	Smax[0] = ( Max.x - ray.origin.x ) / ray.direction.x;

	Smin[1] = ( Min.y - ray.origin.y ) / ray.direction.y;
	Smax[1] = ( Max.y - ray.origin.y ) / ray.direction.y;

	Smin[2] = ( Min.z - ray.origin.z ) / ray.direction.z;
	Smax[2] = ( Max.z - ray.origin.z ) / ray.direction.z;

	// normal for the x plane
	normal[0] = Vec3(1, 0, 0);
	// normal for the y plane
	normal[1] = Vec3(0, 1, 0);
	// normal for the z plane
	normal[2] = Vec3(0, 0, 1);

	int count = 0;
	for(int i = 0; i < 3; i++)
	{	
		bool swap;
		if(Smin[i] > Smax[i])
		{
			//	making sure the min is actually smaller than the max	
			double temp;
			temp = Smin[i];
			Smin[i] = Smax[i];
			Smax[i] = temp;
			swap = true;
		}	
		else swap = false;

		//	check to make sure we get all cases
		if(Smin[i] < 0 && Smax[i] > 0)
		{
			Smin[i] = 0;
			if(swap)	swap = false;
		}

		//	find the biggest min
		if(Smin[i] > maxOfMin)	
		{
			maxOfMin = Smin[i];
			//	update the normal
			if(swap)	finalNormal = (normal[i]);
			else		finalNormal = (-1 * normal[i]);  
		}

		//	find the smallest max
		if(Smax[i] < minOfMax)	
		{
			minOfMax = Smax[i];
		}
	}

	if(maxOfMin > minOfMax)	return false;	// no intersection
	
	if(maxOfMin > hitinfo.distance) return false;  // there was an intersection here, but we already know of one closer

	hitinfo.distance	= maxOfMin;
	hitinfo.point		= ray.origin + maxOfMin * ray.direction;
	hitinfo.normal		= finalNormal;
	hitinfo.ray.origin  = ray.origin;	
	hitinfo.object		= this;
	return true;

}
*/
int Box::GetSamples( const Vec3 &P, const Vec3 &N, Sample *samples, int n ) const
    {
    // To be filled in later.
    return 0;
    }
