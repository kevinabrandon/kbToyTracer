#include "SolidNoise.h"
#include <stdlib.h>
#include <stdio.h>


SolidNoise::SolidNoise( )
{
   int i;

   grad[ 0 ] = Vec3( 1, 1, 0 );
   grad[ 1 ] = Vec3(-1, 1, 0 );
   grad[ 2 ] = Vec3( 1,-1, 0 );
   grad[ 3 ] = Vec3(-1,-1, 0 );

   grad[ 4 ] = Vec3( 1, 0, 1 );
   grad[ 5 ] = Vec3(-1, 0, 1 );
   grad[ 6 ] = Vec3( 1, 0,-1 );
   grad[ 7 ] = Vec3(-1, 0,-1 );

   grad[ 8 ] = Vec3( 0, 1, 1 );
   grad[ 9 ] = Vec3( 0,-1, 1 );
   grad[ 10 ] = Vec3( 0, 1,-1 );
   grad[ 11 ] = Vec3( 0,-1,-1 );

   grad[ 12 ] = Vec3( 1, 1, 0 );
   grad[ 13 ] = Vec3(-1, 1, 0 );
   grad[ 14 ] = Vec3( 0,-1, 1 );
   grad[ 15 ] = Vec3( 0,-1,-1 );

   for( i = 0; i < 16; i++ )	phi[i] = i;

	// shuffle phi
	for( i = 14; i >= 0; i-- ) 
	{
		int target = ( int ) ( rand( ) / ( float ) RAND_MAX * i );
		int temp = phi[ i + 1 ];
		phi[ i + 1 ] = phi[ target ];
		phi[ target ] = temp;
	}
}

SolidNoise::SolidNoise( int seed )
{
   int i;
	
   grad[ 0 ] = Vec3( 1, 1, 0);
   grad[ 1 ] = Vec3(-1, 1, 0);
   grad[ 2 ] = Vec3( 1,-1, 0);
   grad[ 3 ] = Vec3(-1,-1, 0);

   grad[ 4 ] = Vec3( 1, 0, 1);
   grad[ 5 ] = Vec3(-1, 0, 1);
   grad[ 6 ] = Vec3( 1, 0,-1);
   grad[ 7 ] = Vec3(-1, 0,-1);

   grad[ 8 ] = Vec3( 0, 1, 1);
   grad[ 9 ] = Vec3( 0,-1, 1);
   grad[ 10 ] = Vec3( 0, 1,-1);
   grad[ 11 ] = Vec3( 0,-1,-1);

   grad[ 12 ] = Vec3( 1, 1, 0);
   grad[ 13 ] = Vec3(-1, 1, 0);
   grad[ 14 ] = Vec3( 0,-1, 1);
   grad[ 15 ] = Vec3( 0,-1,-1);

	for( i = 0; i < 16; i++ )	phi[i] = i;

	// shuffle phi
	for( i = 14; i >= 0; i-- )
	{
		int target = ( int ) ( rand( ) / ( float ) RAND_MAX * i );
		int temp = phi[ i + 1 ];
		phi[ i + 1 ] = phi[ target ];
		phi[ target ] = temp;
	}
}

float SolidNoise::turbulence( const Vec3 &P, int depth ) const
{
   float sum = 0.0f;
   float weight = 1.0f;
   Vec3 ptemp( P );

   sum = fabs( noise( ptemp ) );
   
   for( int i = 1; i < depth; i++ )
   {
      weight = weight * 2;
      ptemp.x = ( P.x * weight );
      ptemp.y = ( P.y * weight );
      ptemp.z = ( P.z * weight );
      
      sum += fabs( noise( ptemp ) ) / weight;
   }
   return sum;
}

float SolidNoise::dturbulence( const Vec3 &P, int depth, float d ) const 
{
   float sum = 0.0f;
   float weight = 1.0f;
   Vec3 ptemp( P );

   sum += fabs( noise( ptemp ) ) / d;

   for( int i = 1; i < depth; i++ )
   {
       weight = weight * d;
       ptemp.x = ( P.x * weight );
       ptemp.y = ( P.y * weight );
       ptemp.z = ( P.z * weight );
   
       sum += fabs( noise( ptemp ) ) / d;
   }
   return sum;
}

Vec3 SolidNoise::vectorTurbulence( const Vec3 &P, int depth ) const 
{
   Vec3 sum( 0.0, 0.0, 0.0 );
   Vec3 ptemp = P + Vec3( 5.1, 56.2, 25.8 );
   float amp = 1.0;
   Vec3 off( 0.321, 0.112, 0.724 );

   for( int i = 0; i < depth; i++ )
   {
       sum += vectorNoise( ptemp ) * amp;
       amp = amp * 0.6f;
       ptemp.x = ( ptemp.x * 2 + off.x );
       ptemp.y = ( ptemp.y * 2 + off.y );
       ptemp.z = ( ptemp.z * 2 + off.z );
   }
   return sum;
}

Vec3 SolidNoise::vectorNoise( const Vec3 &P ) const 
{
    int i, j, k;
    int fi, fj, fk;
    Vec3 sum, v;

    sum = Vec3(0.0, 0.0, 0.0);
    fi = ( int ) ( floor( P.x ) );
    fj = ( int ) ( floor( P.y ) );
    fk = ( int ) ( floor( P.z ) );
    for( i = fi; i <= fi + 1; i++ )
	{
       for( j = fj; j <= fj + 1; j++ )
	   {
           for( k = fk; k <= fk + 1; k++ ) 
		   {
               v = Vec3( P.x - i, P.y - j, P.z - k );
               sum += vectorKnot( i, j, k, v );
		   }
	   }
	}
    return sum;
    
}

float SolidNoise::noise( const Vec3& P ) const 
{
    int fi, fj, fk;
    float fu, fv, fw, sum = 0.0f;
    Vec3 v;

    fi = ( int ) ( floor( P.x ) );
    fj = ( int ) ( floor( P.y ) );
    fk = ( int ) ( floor( P.z ) );
    fu = ( float ) P.x - fi;
    fv = ( float ) P.y - fj;
    fw = ( float ) P.z - fk;

    v = Vec3( fu, fv, fw );
    sum += knot( fi, fj, fk, v );

    v = Vec3( fu - 1, fv, fw );
    sum += knot( fi + 1, fj, fk, v );

    v = Vec3( fu, fv - 1, fw );
    sum += knot( fi, fj + 1, fk, v );

    v = Vec3( fu, fv, fw - 1 );
    sum += knot( fi, fj, fk + 1, v );

    v = Vec3( fu - 1, fv - 1, fw );
    sum += knot( fi + 1, fj + 1, fk, v );

    v = Vec3( fu - 1, fv, fw - 1 );
    sum += knot( fi + 1, fj, fk + 1, v );

    v = Vec3( fu, fv - 1, fw - 1 );
    sum += knot( fi, fj + 1, fk + 1, v );

    v = Vec3( fu - 1, fv - 1, fw - 1 );
    sum += knot( fi + 1, fj + 1, fk + 1, v );
    
    return sum;
    
}

