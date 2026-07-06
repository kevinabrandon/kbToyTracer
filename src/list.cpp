/***************************************************************************
* list.C   (aggregate object)                                              *
*                                                                          * 
*                                                                          *
* History:                                                                 *
*	10/22/2004	Kevin Brandon - Removed declerations and put			   *
*				them in list.h											   *
*	10/20/2004	Kevin Brandon - Allowed affine transformations			   *
*   10/16/2004  Changed the way the bounding box is computed.              *
*   10/16/2004  Added more documentation, and call to "Inverse" function.  *
*   10/06/2004  Initial coding.                                            *
*                                                                          *
***************************************************************************/

#include "List.h"


// Provide the constructor and destructor for the List object.   
List::List()
{
    parent = NULL; // Used for nesting aggregates.
    first  = NULL; // First child object added to the linked list.
    last   = NULL; // Most recently added child in the linked list.
	this->material.texture = NULL;
}

List::~List()
{
//	delete first;

	//for( List_node *node = first; node != NULL; node = node->next )
    //{
	//	if( node != NULL ) delete node;
	//}
}


// Fill in all the virtual methods for the List object...

bool List::Intersect( const Ray &ray, HitInfo &hitinfo ) const
{
    bool found_a_hit = false;
	HitInfo  tempHitInfo;

    // Each intersector is ONLY allowed to write into the "HitInfo"
    // structure if it has determined that the ray hits the object
    // at a CLOSER distance than currently recorded in HitInfo.distance.
    // When a closer hit is found, the fields of the "HitInfo" structure
    // are updated to hold the material of the object that was just hit.
    // Return "true" if any object was hit.

    for( List_node *node = first; node != NULL; node = node->next )
    {
		if(node->object == NULL) break; 
		
		if(node->object != hitinfo.ignore && node->transformed)
		{
			Ray newRay;
			// transform our ray from the "original domain" to the "transfomed domain"
			newRay.direction = Unit(node->inverse.mat * ray.direction);
			newRay.origin = node->inverse * ray.origin;
			
			// the distance will be different in the "transfomred domain"
			// so we give it a temp hitinfo, and catch each intersection 
			// and check later to see if it is actually closer than the 
			// distance in hitinfo later
			tempHitInfo.distance = Infinity;
			tempHitInfo.ignore = NULL;

			if(node->object->Intersect(newRay, tempHitInfo))
			{
				// there was an intersection in the "transformed domain"
				// we need to take that and bring it back to our "original domain"
				Vec3 tempPoint;
				Ray tempRay;
				double tempDistance;
				
				// transforming all the ray info back to "original domain"
				tempRay.direction = Unit(node->matrix.mat * tempHitInfo.ray.direction);
				tempRay.origin	= node->matrix * tempHitInfo.ray.origin;
				tempPoint = node->matrix * tempHitInfo.point;
				tempDistance = Length(tempPoint - tempRay.origin);

				// test if new distance is shorter than the older distance
				if(tempDistance < hitinfo.distance)
				{
					hitinfo.distance = tempDistance;
					hitinfo.point = tempPoint;
					// The incident ray is simply the original world-space ray. Copy the
					// whole struct so its direction, origin, AND type carry through. The old
					// code rebuilt direction/origin by round-tripping through the transform
					// (wrong for non-uniform scale) and dropped .type entirely, which broke
					// refraction's inside/outside logic on transformed primitives.
					hitinfo.ray = ray;
					// transform the normal to the "original domain"
					hitinfo.normal =  Unit(Inverse(Transpose(node->matrix .mat)) * tempHitInfo.normal );
					hitinfo.object = tempHitInfo.object;
					hitinfo.uv = tempHitInfo.uv;
	
					found_a_hit = true;	
				}
			}
			
		}
		else 	if( node->object != hitinfo.ignore && node->object->Intersect( ray, hitinfo ) )
		{
		    // We've hit an object, but continue looking as there might be a closer hit.
		    found_a_hit = true; 
		}
	}
	return found_a_hit;
}

Box3 List::GetBounds() const
{
    Box3 bbox;

    // Make the bbox degenerate so that the first valid box will replace it.
    bbox.X = Interval( Infinity, -Infinity );
    bbox.Y = Interval( Infinity, -Infinity );
    bbox.Z = Interval( Infinity, -Infinity );

    // Compute the box by running through all the children, transforming the boxes if necessary.
    for( List_node *node = first; node != NULL; node = node->next )
	{
        Box3 child_bbox = node->object->GetBounds();
        if( node->transformed ) bbox += node->matrix * child_bbox;
        else bbox += child_bbox;
	}

    return bbox;
}

Object *List::ReadString( const char *params )
{
    extern bool Get( const char *line, const char *name, const char *arg );
    // Every aggregate object should look for a "begin" preceeding its name.
    // An aggregate can also accept additional parameters on this line (e.g. the
    // size of a uniform grid, maximum depth of a BSP tree, etc.).  It is recommended
    // that missing parameters default to something reasonable.
    if( Get( params, "begin", "List" ) ) return new List();
    return NULL;
}

void List::Transform( const Mat3x4 &mat )
{
    // Store the transformation with the last object that was added.
    if( last != NULL )
	{
        last->matrix  = mat;
        last->inverse = Inverse( mat );
        last->transformed = true;
    }
}

void List::AddChild( Object *obj )
{
    // Create a new "List_node" that will point to the new object, and splice it into
    // the linked list.
    List_node *node = new List_node;
    node->object = obj;
    node->next = NULL;
    node->transformed = false;  // Set to true when a matrix is applied.
    if( first == NULL )
    {
        // This is the first object added.
        first = node;
    }
    else
    {
        // This is not the first object added, so link it in at the end.
        last->next = node;
    }
    last = node;
}



