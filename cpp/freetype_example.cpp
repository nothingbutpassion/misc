/* example1.c                                                      */
/*                                                                 */
/* This small program shows how to print a rotated string with the */
/* FreeType 2 library.                                             */


#include <stdio.h>
#include <string.h>
#include <math.h>

#include <ft2build.h>
#include FT_FREETYPE_H


#define WIDTH   160
#define HEIGHT  48


/* origin is the upper left corner */
unsigned char image[HEIGHT][WIDTH];


/* Replace this function with something useful. */

void
draw_bitmap( FT_Bitmap*  bitmap,
             FT_Int      x,
             FT_Int      y)
{
  FT_Int  i, j, p, q;
  FT_Int  x_max = x + bitmap->width;
  FT_Int  y_max = y + bitmap->rows;

  printf("x=%d, y=%d, x_max=%d, y_max=%d\n", x, y, x_max, y_max);

  /* for simplicity, we assume that `bitmap->pixel_mode' */
  /* is `FT_PIXEL_MODE_GRAY' (i.e., not a bitmap font)   */

  for ( i = x, p = 0; i < x_max; i++, p++ )
  {
    for ( j = y, q = 0; j < y_max; j++, q++ )
    {
      if ( i < 0      || j < 0       ||
           i >= WIDTH || j >= HEIGHT )
        continue;

      image[j][i] |= bitmap->buffer[q * bitmap->width + p];
    }
  }
}


void
show_image( void )
{
  int  i, j;


  for ( i = 0; i < HEIGHT; i++ )
  {
    for ( j = 0; j < WIDTH; j++ )
      putchar( image[i][j] == 0 ? ' '
                                : image[i][j] < 128 ? '+'
                                                    : '*' );
    putchar( '\n' );
  }
}


int
main( int     argc,
      char**  argv )
{
  FT_Library    library;
  FT_Face       face;

  FT_GlyphSlot  slot;
  FT_Error      error;

  char*         filename;
  char*         text;

  int           n, num_chars;


  if ( argc != 3 )
  {
    fprintf ( stderr, "usage: %s font sample-text\n", argv[0] );
    exit( 1 );
  }

  filename      = argv[1];                           /* first argument     */
  text          = argv[2];                           /* second argument    */
  num_chars     = strlen( text );

  error = FT_Init_FreeType( &library );              /* initialize library */
  /* error handling omitted */

  error = FT_New_Face( library, filename, 0, &face );/* create face object */
  /* error handling omitted */

  /* use 50pt at 100dpi */
  //error = FT_Set_Char_Size( face, 50 * 64, 0,
  //                          100, 0 );                /* set character size */
  FT_Set_Pixel_Sizes(face, 0, 32);
  /* error handling omitted */

  slot = face->glyph;

  FT_Int x = 8;
  FT_Int base = 32;
  for ( n = 0; n < num_chars; n++ )
  {

    /* load glyph image into the slot (erase previous one) */
    error = FT_Load_Char( face, text[n], FT_LOAD_RENDER );
    if ( error )
      continue;                 /* ignore errors */

    /* now, draw to our target surface (convert position) */
    draw_bitmap( &slot->bitmap,
                 x + slot->bitmap_left,
                 base - slot->bitmap_top);

    /* increment pen position */
    x += slot->advance.x/64;
  }

  show_image();

  FT_Done_Face    ( face );
  FT_Done_FreeType( library );

  return 0;
}

/* EOF */
