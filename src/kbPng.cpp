//
//	kbPng.cpp
//
//	PNG output (see kbPng.h), two ways:
//
//	1) If src/stb_image_write.h is present (Sean Barrett's single-header
//	   public-domain/MIT library), use it: real deflate compression and
//	   PNG row filtering, ~5x smaller files.
//
//	2) Otherwise fall back to the built-in minimal encoder below.  A PNG
//	   is a signature followed by chunks (IHDR, IDAT, IEND), each carrying
//	   a CRC-32.  The pixel data inside IDAT is a zlib stream of filtered
//	   scanlines; this encoder uses filter 0 (none) and "stored" deflate
//	   blocks (BTYPE=00), which are raw bytes with no compression.  Output
//	   is about the size of a PPM, but universally viewable.
//

#include "kbPng.h"
#include <cstdio>
#include <cstring>

#if defined(__has_include)
#  if __has_include("stb_image_write.h")
#    define KB_HAVE_STB_PNG 1
#  endif
#endif

#ifdef KB_HAVE_STB_PNG

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

bool kbWritePNG( const Image &img, const char *file_name )
{
    if( img.width <= 0 || img.height <= 0 || img.pixels == NULL ) return false;
    // Pixel is three packed bytes (r,g,b), rows contiguous: exactly what stb wants.
    return stbi_write_png( file_name, img.width, img.height, 3,
                           img.pixels, img.width * 3 ) != 0;
}

#else  // built-in fallback encoder

#include <vector>

// ---- CRC-32 (PNG chunk checksum) ------------------------------------------

static unsigned long crcTable[256];
static bool crcTableComputed = false;

static void makeCrcTable()
{
    for( unsigned long n = 0; n < 256; n++ )
    {
        unsigned long c = n;
        for( int k = 0; k < 8; k++ )
            c = ( c & 1 ) ? ( 0xEDB88320UL ^ ( c >> 1 ) ) : ( c >> 1 );
        crcTable[n] = c;
    }
    crcTableComputed = true;
}

static unsigned long updateCrc( unsigned long crc, const unsigned char *buf, size_t len )
{
    if( !crcTableComputed ) makeCrcTable();
    for( size_t n = 0; n < len; n++ )
        crc = crcTable[ ( crc ^ buf[n] ) & 0xFF ] ^ ( crc >> 8 );
    return crc;
}

// ---- Adler-32 (zlib stream checksum) ---------------------------------------

static unsigned long adler32( const unsigned char *buf, size_t len )
{
    unsigned long a = 1, b = 0;
    for( size_t n = 0; n < len; n++ )
    {
        a = ( a + buf[n] ) % 65521;
        b = ( b + a )      % 65521;
    }
    return ( b << 16 ) | a;
}

// ---- Chunk writing ----------------------------------------------------------

static void putU32( std::vector<unsigned char> &out, unsigned long v )   // big-endian
{
    out.push_back( (unsigned char)( ( v >> 24 ) & 0xFF ) );
    out.push_back( (unsigned char)( ( v >> 16 ) & 0xFF ) );
    out.push_back( (unsigned char)( ( v >>  8 ) & 0xFF ) );
    out.push_back( (unsigned char)(   v         & 0xFF ) );
}

static void writeChunk( std::vector<unsigned char> &out, const char type[4],
                        const unsigned char *data, size_t len )
{
    putU32( out, (unsigned long)len );
    size_t typeAt = out.size();
    out.insert( out.end(), type, type + 4 );
    if( len ) out.insert( out.end(), data, data + len );
    unsigned long crc = updateCrc( 0xFFFFFFFFUL, &out[typeAt], len + 4 ) ^ 0xFFFFFFFFUL;
    putU32( out, crc );
}

// ---- The encoder ------------------------------------------------------------

bool kbWritePNG( const Image &img, const char *file_name )
{
    const int w = img.width, h = img.height;
    if( w <= 0 || h <= 0 || img.pixels == NULL ) return false;

    // Raw zlib payload: each scanline is a filter byte (0 = none) + RGB bytes.
    const size_t stride = 1 + (size_t)w * 3;
    std::vector<unsigned char> raw( stride * h );
    for( int i = 0; i < h; i++ )
    {
        unsigned char *row = &raw[ i * stride ];
        row[0] = 0;   // filter: none
        memcpy( row + 1, img.pixels + (size_t)i * w, (size_t)w * 3 );
    }

    // zlib stream: 2-byte header, stored deflate blocks (max 65535 each), Adler-32.
    std::vector<unsigned char> z;
    z.reserve( raw.size() + raw.size() / 65535 * 5 + 16 );
    z.push_back( 0x78 );  // deflate, 32K window
    z.push_back( 0x01 );  // no preset dict, fastest-compression flag; FCHECK ok (0x7801 % 31 == 0)
    size_t pos = 0;
    while( pos < raw.size() )
    {
        size_t n = raw.size() - pos;
        if( n > 65535 ) n = 65535;
        bool final = ( pos + n == raw.size() );
        z.push_back( final ? 1 : 0 );                       // BFINAL, BTYPE=00 (stored)
        z.push_back( (unsigned char)(  n       & 0xFF ) );  // LEN, little-endian
        z.push_back( (unsigned char)( (n >> 8) & 0xFF ) );
        z.push_back( (unsigned char)( ~n       & 0xFF ) );  // NLEN = ~LEN
        z.push_back( (unsigned char)( (~n >> 8) & 0xFF ) );
        z.insert( z.end(), raw.begin() + pos, raw.begin() + pos + n );
        pos += n;
    }
    putU32( z, adler32( raw.data(), raw.size() ) );

    // Assemble the file: signature, IHDR, IDAT, IEND.
    std::vector<unsigned char> out;
    out.reserve( z.size() + 64 );
    static const unsigned char sig[8] = { 137, 80, 78, 71, 13, 10, 26, 10 };
    out.insert( out.end(), sig, sig + 8 );

    unsigned char ihdr[13];
    ihdr[0] = (unsigned char)( ( w >> 24 ) & 0xFF );
    ihdr[1] = (unsigned char)( ( w >> 16 ) & 0xFF );
    ihdr[2] = (unsigned char)( ( w >>  8 ) & 0xFF );
    ihdr[3] = (unsigned char)(   w         & 0xFF );
    ihdr[4] = (unsigned char)( ( h >> 24 ) & 0xFF );
    ihdr[5] = (unsigned char)( ( h >> 16 ) & 0xFF );
    ihdr[6] = (unsigned char)( ( h >>  8 ) & 0xFF );
    ihdr[7] = (unsigned char)(   h         & 0xFF );
    ihdr[8]  = 8;   // bit depth
    ihdr[9]  = 2;   // color type: truecolor RGB
    ihdr[10] = 0;   // compression: deflate
    ihdr[11] = 0;   // filter method 0
    ihdr[12] = 0;   // no interlace
    writeChunk( out, "IHDR", ihdr, 13 );
    writeChunk( out, "IDAT", z.data(), z.size() );
    writeChunk( out, "IEND", NULL, 0 );

    FILE *fp = fopen( file_name, "wb" );
    if( fp == NULL ) return false;
    size_t written = fwrite( out.data(), 1, out.size(), fp );
    fclose( fp );
    return written == out.size();
}

#endif  // KB_HAVE_STB_PNG

bool kbWriteImage( Image &img, const char *file_name )
{
    size_t len = strlen( file_name );
    if( len >= 4 && strcmp( file_name + len - 4, ".png" ) == 0 )
        return kbWritePNG( img, file_name );
    return img.Write( file_name );   // Arvo's PPM writer
}
