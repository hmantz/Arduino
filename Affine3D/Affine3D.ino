
#include <Gamebuino-Meta.h>

#include "Picture.h"
#include "RaceMap.h"
#include "KartLego.h"

// Defines a union type giving access to the data as 8, 16 or 32 bits integers
union FixedPoint
{
  int32_t  asInt32;
  int16_t  asInt16[2];
  uint16_t asUInt16[2];
  int8_t   asInt8[4];
  uint8_t  asUInt8[4];
};

// Definition of 32-bits fixed-point type for better readability
typedef int32_t FP32;

// FP32 macro to convert from float to FP32
#define FP32_FROM_FLOAT(a) ( (FP32) (65536.0*(a)) )

// FP32 macro to extract least significant unsigned byte of the integer part of an FP32 value
//#define FP32_LSBYTE(A) ( (uint8_t) ( (uint8_t*)&(A))[2] )
#define FP32_LSBYTE(A) ( (uint8_t) ( (FixedPoint({(A)})).asUInt8[2] ) )

// FP32 macro to extract most significant signed byte of the integer part of an FP32 value
//#define FP32_MSBYTE(A) ( (int8_t) ( ((int8_t*)&(A))[3]) )
#define FP32_MSBYTE(A) ( (int8_t) ( (FixedPoint({(A)})).asInt8[3] ) )

// FP32 macro to multiply two FP32 values
#define FP32_MUL(A,B) ( (FP32) (( ((int64_t)(A)) * ((int64_t)(B)) ) >> 16L) )

// FP32 macro to divide two FP32 values
#define FP32_DIV(A,B) ( (FP32) (( ((int64_t)(A)) << 16L )/(B)) )

// Macros to repeat 80 times the same text
#define REPEAT20(X) X X X X X X X X X X X X X X X X X X X X
#define REPEAT80(X) REPEAT20(X) REPEAT20(X) REPEAT20(X) REPEAT20(X)

// Draws one row of 80 pixels at destination using pixel colors from the source picture
// at (x,y) and incremented at each pixel by (dx, dy)
Color* drawRow(Color* source, Color* destination, FP32 x, FP32 y, FP32 dx, FP32 dy,
               bool infinite=true, Color background=BLACK)
{
  // Repeat 80 times the same instructions without boundaries test if infinite
  if (infinite)
  {
    REPEAT80(
      *destination++ = source[ FP32_LSBYTE(x) + (FP32_LSBYTE(y)<<8) ];
      x += dx; y += dy;
    )
  }

  // Repeat 80 times the same instructions with boundaries test
  else
  {
    REPEAT80(
      if (FP32_MSBYTE(x)!=0 || FP32_MSBYTE(y)!=0)
        *destination++ = background;
      else
        *destination++ = source[ FP32_LSBYTE(x) + (FP32_LSBYTE(y)<<8) ];
      x += dx; y += dy;
    )
  }

  // Returns the last value of destination, now pointing on the first pixel of the next row
  return destination;
}


// Draws one row of 80 pixels at destination using pixel colors from the tilemap 16x16 tiles of 16x16 pixels,
// starting at (x,y) in the virtual picture and incremented at each pixel by (dx, dy)
Color* drawRowTileMap(Color* tileset, int8_t* tilemap, Color* destination, FP32 x, FP32 y, FP32 dx, FP32 dy,
                      bool infinite=true, Color background=BLACK)
{
  // Repeat 80 times the same instructions without boundaries test if infinite
  if (infinite)
  {
    REPEAT80(
      *destination++ = tileset[ (((int16_t)tilemap[(FP32_LSBYTE(x)>>4) + (FP32_LSBYTE(y) & 0b11110000)])<<8)
                                 + (FP32_LSBYTE(x) & 0b00001111) + ((FP32_LSBYTE(y) & 0b00001111)<<4) ];
      x += dx; y += dy;
    )
  }
  
  // Repeat 80 times the same instructions with boundaries test
  else
  {
    REPEAT80(
      if (FP32_MSBYTE(x)!=0 || FP32_MSBYTE(y)!=0)
        *destination++ = background;
      else
        *destination++ = tileset[ (((int16_t)tilemap[(FP32_LSBYTE(x)>>4) + (FP32_LSBYTE(y) & 0b11110000)])<<8)
                                 + (FP32_LSBYTE(x) & 0b00001111) + ((FP32_LSBYTE(y) & 0b00001111)<<4) ];
      x += dx; y += dy;
    )
  }

  // Returns the last value of destination, now pointing on the first pixel of the next row
  return destination;
}


// Draws row with bump map - Not included because effect is not convincing
// destination = drawRowBump(source, destination, startx, starty, incx, incy,
//                           -cosaFP*3, sinaFP*3, BLACK, infinite, background);

// Draws one row of 80 pixels at destination using pixel colors from the source picture
// at (x,y) and incremented at each pixel by (dx, dy)
Color* drawRowBump(Color* source, Color* destination, FP32 x, FP32 y, FP32 dx, FP32 dy,
                   FP32 bmdx, FP32 bmdy, Color bump, bool infinite=true, Color background=BLACK)
{
  Color color;

  // Repeat 80 times the same instructions without boundaries test if infinite
  if (infinite)
  {
    REPEAT80(
      color = source[ FP32_LSBYTE(x) + (FP32_LSBYTE(y)<<8) ];
      if (color == bump)
        color = source[ FP32_LSBYTE(x+bmdx) + (FP32_LSBYTE(y+bmdy)<<8) ];
      *destination++ = color;
      x += dx; y += dy;
    )
  }

  // Repeat 80 times the same instructions with boundaries test
  else
  {
    REPEAT80(
      if (FP32_MSBYTE(x)!=0 || FP32_MSBYTE(y)!=0)
        color = background;
      else
      {
        color = source[ FP32_LSBYTE(x) + (FP32_LSBYTE(y)<<8) ];
        if (color == bump)
        {
          FP32 tx = x+bmdx; FP32 ty = y+bmdy;
          if (FP32_MSBYTE(tx)!=0 || FP32_MSBYTE(ty)!=0)
            color = background;
          else
            color = source[ FP32_LSBYTE(tx) + (FP32_LSBYTE(ty)<<8) ];
        }
      }
      *destination++ = color;
      x += dx; y += dy;
    )
  }

  // Returns the last value of destination, now pointing on the first pixel of the next row
  return destination;
}


void setup()
{
  // Starts Gamebuino library and sets target frame rate
  gb.begin();
  gb.setFrameRate(50);
}



// Considers boundaries of source picture or wrapped infinitely
bool infinite=false;

// Part of the tutorial we are looking at, and total number of parts
int part=0, numparts = 6, previousPart = -1;

// 3D view parameters
float s, h, w, Ox, Oy, a;
int firstRow = 1;

// Time measurement variable
unsigned long previousStartTime = micros();

// Kart variables
float kartSpeed, kartDirection, kartX, kartY;


void loop()
{
  // Inits pointers on source and destination pixel arrays
  Color* source = (Color*)(PictureData+6);
  Color* destination = (Color*)(gb.display._buffer);
  Color* tileset = (Color*)(raceTileSet+6);
  int8_t* tilemap = (int8_t*)(raceTileMap);

  // Defines other variables
  Color background;
  FP32 startx, starty, incx, incy;

  // If the user just selected a new part, then sets default parameters
  if (part!=previousPart)
  {
    // Default view parameters
    s = 100;
    w = 40;

    // Kart parameters
    kartSpeed = 0;
    kartDirection=PI/2.0;
    kartX = 24;
    kartY = 136;
  }

  // Waits for screen refresh
  while(!gb.update());

  // Gets the time at start of the computation
  unsigned long startTime = micros();

  // Time in seconds
  float time = (float)startTime/1e6;

  // Sets dynamic view parameters
  if (part==5)
  {
    // Kart part
    a = kartDirection;
    Ox = kartX - 10*cos(kartDirection);
    Oy = kartY + 10*sin(kartDirection);
    h = 5;
  }
  else
  {
    // Standard part
    a = time;
    Ox = 128;
    Oy = 128;
    h = 25+10*sin(time/3);
  }

  // Optimized view parameters and pre-computed values
  float sina = sin(a);
  float cosa = cos(a);
  float Ax = h*(s*cosa-w*sina); 
  float Ay = h*(-s*sina-w*cosa);

  // Inits fixed-point equivalent variables for the frame
  FP32 sinaFP = FP32_FROM_FLOAT(sina);
  FP32 cosaFP = FP32_FROM_FLOAT(cosa);
  FP32 AxFP =   FP32_FROM_FLOAT(Ax);
  FP32 AyFP =   FP32_FROM_FLOAT(Ay);
  FP32 OxFP =   FP32_FROM_FLOAT(Ox);
  FP32 OyFP =   FP32_FROM_FLOAT(Oy);
  FP32 hFP =    FP32_FROM_FLOAT(h);

  // Selects the draw procedure
  switch(part)
  {
    case 0: // Sine deformation ---------------------------------------------------------------
    {
      // Loops on each row
      for(int py=0; py<64; py++)
      {
        // Creates background color for this row, blue shade from light to dark
        background = gb.createColor(0,0,(64-py)*4-1);

        startx = FP32_FROM_FLOAT(-50);
        starty = FP32_FROM_FLOAT((float)py*4.0);
        incx =   FP32_FROM_FLOAT((1.5+cos(time+(float)py/20.0))*2.0);
        incy =   FP32_FROM_FLOAT((1.5+sin(time+(float)py/20.0))*2.0);

        destination = drawRow(source, destination, startx, starty, incx, incy, infinite, background);
      }

      break;
    }

    case 1: // Non-optimized 3D display -------------------------------------------------------
    {
      // Loops on each row
      for(int py=0; py<64; py++)
      {
        // Creates background color for this row, blue shade from light to dark
        background = gb.createColor(0,0,(64-py)*4-1);

        // First lines are filled with background to avoid division by zero and moire effect
        if(py<firstRow)
          for(int px=0; px<80; px++)
            *destination++ = background;
        else
        {
          // Computes start point and increment as floats
          float y = (float)py;
          float Dx = Ox + s*h*cos(a)/y - w*h*sin(a)/y;
          float Dy = Oy - s*h*sin(a)/y - w*h*cos(a)/y;
          float Ix = h*sin(a)/y;
          float Iy = h*cos(a)/y;

          // Transforms floats into fixed-point variables
          startx = FP32_FROM_FLOAT(Dx);
          starty = FP32_FROM_FLOAT(Dy);
          incx =   FP32_FROM_FLOAT(Ix);
          incy =   FP32_FROM_FLOAT(Iy);

          // Draws row
          destination = drawRow(source, destination, startx, starty, incx, incy, infinite, background);
        }
      }

      break;
    }

    case 2: // 3D display floats --------------------------------------------------------------
    {
      // Loops on each row
      for(int py=0; py<64; py++)
      {
        // Creates background color for this row, blue shade from light to dark
        Color background = gb.createColor(0,0,(64-py)*4-1);

        // First lines are filled with background to avoid division by zero and moire effect
        if(py<firstRow)
          for(int px=0; px<80; px++)
            *destination++ = background;
        else
        {
          // Computes values for the row
          float factor = 1/(float)py;
          startx = FP32_FROM_FLOAT(Ox + Ax*factor);
          starty = FP32_FROM_FLOAT(Oy + Ay*factor);
          factor *= h;
          incx = FP32_FROM_FLOAT(sina*factor);
          incy = FP32_FROM_FLOAT(cosa*factor);

          // Draws row
          destination = drawRow(source, destination, startx, starty, incx, incy, infinite, background);
        }
      }
      
      break;
    }

    case 3: // 3D display fixed point --------------------------------------------------------------
    {
      // Loops on each row
      for(int py=0; py<64; py++)
      {
        // Direct creation of the background color (Blue part on 5 bits)
        Color background = (Color)((62-py)>>1);

        // First lines are filled with background to avoid division by zero and moire effect
        if(py<firstRow)
          for(int px=0; px<80; px++)
            *destination++ = background;
        else
        {
          // Computes values for the row
          FP32 factorFP = FP32_DIV(1<<16 , py<<16);
          //FP32 factorFP = FP32_FROM_FLOAT(1/(float)py);
          startx = OxFP + FP32_MUL(AxFP, factorFP);
          starty = OyFP + FP32_MUL(AyFP, factorFP);
          factorFP = FP32_MUL(factorFP, hFP);
          incx = FP32_MUL(sinaFP, factorFP);
          incy = FP32_MUL(cosaFP, factorFP);

          // Draws row
          destination = drawRow(source, destination, startx, starty, incx, incy, infinite, background);
        }
      }
      
      break;
    }

    case 4: // 3D display TileMap --------------------------------------------------------------
    {
      // Loops on each row
      for(int py=0; py<64; py++)
      {
        // Direct creation of the background color (Blue part on 5 bits)
        Color background = (Color)((62-py)>>1);

        // First lines are filled with background to avoid division by zero and moire effect
        if(py<firstRow)
          for(int px=0; px<80; px++)
            *destination++ = background;
        else
        {
          // Computes values for the row
          FP32 factorFP = FP32_DIV(1<<16 , py<<16);
          startx = OxFP + FP32_MUL(AxFP, factorFP);
          starty = OyFP + FP32_MUL(AyFP, factorFP);
          factorFP = FP32_MUL(factorFP, hFP);
          incx = FP32_MUL(sinaFP, factorFP);
          incy = FP32_MUL(cosaFP, factorFP);

          // Draws row
          destination = drawRowTileMap(tileset, tilemap, destination, startx, starty,
                                       incx, incy, infinite, background);
        }
      }
      
      break;
    }

    case 5: // 3D display fixed point TileMap with controllable Kart ----------------------------------------
    {
      // Loops on each row
      for(int py=0; py<64; py++)
      {
        // Direct creation of the background color (Blue part on 5 bits)
        Color background = (Color)((62-py)>>1);

        // First lines are filled with background to avoid division by zero and moire effect
        if(py<firstRow)
          for(int px=0; px<80; px++)
            *destination++ = background;
        else
        {
          // Computes values for the row
          FP32 factorFP = FP32_DIV(1<<16 , py<<16);
          startx = OxFP + FP32_MUL(AxFP, factorFP);
          starty = OyFP + FP32_MUL(AyFP, factorFP);
          factorFP = FP32_MUL(factorFP, hFP);
          incx = FP32_MUL(sinaFP, factorFP);
          incy = FP32_MUL(cosaFP, factorFP);

          // Draws row
          destination = drawRowTileMap(tileset, tilemap, destination, startx, starty,
                                       incx, incy, infinite, background);
        }
      }

      gb.display.drawImage(40-16, 30, KartLego);

      // Manages speed
      if (gb.buttons.repeat(BUTTON_A,0))
        kartSpeed += 0.12;
      if (gb.buttons.repeat(BUTTON_UP,0))
        kartSpeed += 0.15;
      if (gb.buttons.repeat(BUTTON_DOWN,0))
        kartSpeed -= 0.05;
      kartSpeed *= 0.9;

      // Manages direction
      if (gb.buttons.repeat(BUTTON_LEFT,0))
        kartDirection += 0.06;
      if (gb.buttons.repeat(BUTTON_RIGHT,0))
        kartDirection -= 0.06;

      // Updates position
      kartX +=  kartSpeed*cos(kartDirection);
      kartY += -kartSpeed*sin(kartDirection);

      break;
    }

    default: break;
  }

  // Computes real FPS
  int realFPS = (int)floor(1e6/(startTime-previousStartTime));
  previousStartTime = startTime;

  // Computes and displays duration
  unsigned long duration = micros()-startTime;
  gb.display.clearTextVars();
  gb.display.setColor(WHITE, BLACK);
  gb.display.print(duration); gb.display.print(" \xB5s");

  // Displays FPS and optimization level if B button is pressed
  if(gb.buttons.repeat(BUTTON_MENU, 0))
  {
    gb.display.print(" - ");
    gb.display.print(realFPS);  gb.display.println(" FPS");
    gb.display.println(part);
  }

  // Retains current part for later check
  previousPart = part;

  // Goes to next part if A button is pressed
  if(part !=5 && gb.buttons.pressed(BUTTON_A))
    part = (part+1) % numparts;
  // Goes to previous part if B button is pressed
  if(gb.buttons.pressed(BUTTON_B))
    part = (part+numparts-1) % numparts;

  // Changes source picture wrapping if Menu is released
  if(gb.buttons.released(BUTTON_MENU))
    infinite = !infinite;

}
