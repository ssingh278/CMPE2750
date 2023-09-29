// version 1.2 
// Jan 2023    - updated to use lean build
//             - updated to new method for PROGMEM buffer access
// Feb 09 2023 - Diarmid Rendell added option/configuration for 128 x 32 display
// Feb 13 2023 - Adjusted directives to include 128x32 and 128x64 implementations
//               Needs to be adjusted to make this easier to maintain, but also
//                require appropriate backbuffer storage.

// SSD1306 - OLED Display : 7-bit Address 0x3C as _SSD1306_ADDRESS
// this library has only been tested with 128 x 64 SSD1306 devices
//  tested with 128 x 32 as of Feb 09 2023
//  if you intend to use a colour display or a display with different
//  dimensions, then the library will need to be modified!
#include "I2C.h"
#include "SSD1306.h"
#include <avr/io.h>
#include <stdlib.h>
#include <math.h>

// the back-buffer for the OLED display
// bits in here map to pixels on the display
// no direct writes to display memory should occur
// setting bits via functions will dirty flags for redraw
#ifdef _SSD1306_DisplaySize128x64
static unsigned char _DispBuff [8 * 128] = { 0 };

// dirty flags for render management (vertical banks)
static unsigned char _DispDirty [8] = { 0 };
#endif

#ifdef _SSD1306_DisplaySize128x32
static unsigned char _DispBuff [4 * 128] = { 0 };

// dirty flags for render management (vertical banks)
static unsigned char _DispDirty [4] = { 0 };
#endif

// char map needs to cover characters ASCII 32 to 126, so 95 character mappings
// functions will expect normal ASCII values, but map will offset correctly
// 1 through 31 are special characters that need to be defined
// don't attempt to use characters that are not in the map
// should be OK to shove this in flash
const unsigned char _CharMap [96 * 5] PROGMEM =
{
  // special characters
  0x03, 0x03, 0x78, 0x48, 0x48,   // 31 (degrees C) \x1F
  
  // standard ASCII characters
  0x00, 0x00, 0x00, 0x00, 0x00,	// 32 (space)
  0x00, 0x00, 0x5F, 0x00, 0x00,	// 33 ! +
  0x00, 0x03, 0x00, 0x03, 0x00,	// 34 " +
  0x12, 0x7F, 0x12, 0x7F, 0x12,	// 35 # +
  0x04, 0x2A, 0x7F, 0x2A, 0x10,	// 36 $ +
  0x23, 0x13, 0x08, 0x64, 0x62,	// 37 % +
  0x36, 0x49, 0x55, 0x22, 0x50,	// 38 & +
  0x00, 0x00, 0x03, 0x00, 0x00,	// 39 ' +
  0x00, 0x1C, 0x22, 0x41, 0x00,	// 40 ( +
  0x00, 0x41, 0x22, 0x1C, 0x00,	// 41 ) +
  0x14, 0x08, 0x3E, 0x08, 0x14,	// 42 * +
  0x00, 0x08, 0x1C, 0x08, 0x00,	// 43 + +
  0x00, 0x50, 0x30, 0x00, 0x00,	// 44 , +
  0x00, 0x08, 0x08, 0x08, 0x00,	// 45 - +
  0x00, 0x60, 0x60, 0x00, 0x00,	// 46 . +
  0x20, 0x10, 0x08, 0x04, 0x02,	// 47 / +
  0x3E, 0x41, 0x49, 0x41, 0x3E,	// 48 0 +
  0x41, 0x41, 0x7F, 0x40, 0x40,	// 49 1 +
  0x62, 0x51, 0x51, 0x4A, 0x44,	// 50 2 +
  0x41, 0x49, 0x49, 0x5D, 0x22,	// 51 3 +
  0x04, 0x0A, 0x09, 0x08, 0x7F,	// 52 4 +
  0x27, 0x49, 0x49, 0x49, 0x31,	// 53 5 +
  0x3E, 0x49, 0x49, 0x49, 0x32,	// 54 6 +
  0x03, 0x01, 0x71, 0x09, 0x07,	// 55 7 +
  0x36, 0x49, 0x49, 0x49, 0x36,	// 56 8 +
  0x26, 0x49, 0x49, 0x49, 0x3E,	// 57 9 +
  0x00, 0x36, 0x36, 0x00, 0x00,	// 58 : +
  0x00, 0x56, 0x36, 0x00, 0x00,	// 59 ; +
  0x08, 0x14, 0x22, 0x41, 0x00,	// 60 < +
  0x00, 0x14, 0x14, 0x14, 0x00,	// 61 = +
  0x00, 0x41, 0x22, 0x14, 0x08,	// 62 > +
  0x02, 0x01, 0x51, 0x09, 0x06,	// 63 ? +
  0x32, 0x49, 0x79, 0x41, 0x3E,	// 64 @ +
  0x7C, 0x0A, 0x09, 0x0A, 0x7C,	// 65 A +
  0x7F, 0x49, 0x49, 0x49, 0x36,	// 66 B +
  0x3E, 0x41, 0x41, 0x41, 0x22,	// 67 C +
  0x7F, 0x41, 0x41, 0x41, 0x3e,	// 68 D +
  0x7f, 0x49, 0x49, 0x41, 0x41,	// 69 E +
  0x7f, 0x09, 0x09, 0x01, 0x01,	// 70 F +
  0x3E, 0x41, 0x41, 0x49, 0x3A,	// 71 G +
  0x7F, 0x08, 0x08, 0x08, 0x7F,	// 72 H +
  0x41, 0x41, 0x7F, 0x41, 0x41,	// 73 I +
  0x21, 0x41, 0x41, 0x3F, 0x01,	// 74 J +
  0x7F, 0x0C, 0x12, 0x21, 0x40,	// 75 K +
  0x7F, 0x40, 0x40, 0x40, 0x40,	// 76 L +
  0x7F, 0x02, 0x04, 0x02, 0x7F,	// 77 M +
  0x7F, 0x04, 0x08, 0x10, 0x7F,	// 78 N +
  0x3E, 0x41, 0x41, 0x41, 0x3e,	// 79 O +
  0x7F, 0x09, 0x09, 0x09, 0x06,	// 80 P +
  0x1E, 0x41, 0x51, 0x21, 0x5E,	// 81 Q +
  0x7E, 0x09, 0x19, 0x29, 0x46,	// 82 R +
  0x66, 0x49, 0x49, 0x49, 0x33,	// 83 S +
  0x01, 0x01, 0x7F, 0x01, 0x01,	// 84 T +
  0x3F, 0x40, 0x40, 0x40, 0x3F,	// 85 U +
  0x07, 0x38, 0x40, 0x38, 0x07,	// 86 V +
  0x3F, 0x40, 0x30, 0x40, 0x3F,	// 87 W +
  0x41, 0x36, 0x08, 0x36, 0x41,	// 88 X +
  0x01, 0x06, 0x78, 0x06, 0x01,	// 89 Y +
  0x63, 0x51, 0x49, 0x47, 0x63,	// 90 Z +
  0x00, 0x7F, 0x41, 0x41, 0x00,	// 91 [ +
  0x02, 0x04, 0x08, 0x10, 0x20,	// 92 backslash +
  0x00, 0x41, 0x41, 0x7F, 0x00,	// 93 ] +
  0x04, 0x02, 0x01, 0x02, 0x04,	// 94 ^ +
  0x40, 0x40, 0x40, 0x40, 0x40,	// 95 _ +
  0x01, 0x02, 0x00, 0x00, 0x00,	// 96 ` +
  0x24, 0x54, 0x54, 0x54, 0x78,	// 97  a +
  0x7F, 0x44, 0x44, 0x44, 0x78,	// 98  b +
  0x38, 0x44, 0x44, 0x44, 0x28,	// 99  c +
  0x38, 0x44, 0x44, 0x44, 0x7F,	// 100 d +
  0x38, 0x54, 0x54, 0x54, 0x28,	// 101 e +
  0x7E, 0x09, 0x09, 0x01, 0x02,	// 102 f +
  0x0E, 0x51, 0x51, 0x51, 0x3E,	// 103 g +
  0x7F, 0x04, 0x04, 0x04, 0x78,	// 104 h +
  0x00, 0x00, 0x7D, 0x00, 0x00,	// 105 i +
  0x20, 0x40, 0x40, 0x3D, 0x00,	// 106 j +
  0x7F, 0x10, 0x28, 0x40, 0x00,	// 107 k +
  0x00, 0x00, 0x7F, 0x00, 0x00,	// 108 l +
  0x7C, 0x04, 0x18, 0x04, 0x7C,	// 109 m +
  0x7C, 0x04, 0x04, 0x04, 0x78,	// 110 n +
  0x38, 0x44, 0x44, 0x44, 0x38,	// 111 o +
  0x7C, 0x14, 0x14, 0x14, 0x08,	// 112 p +
  0x08, 0x14, 0x14, 0x78, 0x40,	// 113 q +
  0x7C, 0x04, 0x04, 0x04, 0x08,	// 114 r +
  0x08, 0x54, 0x54, 0x54, 0x20,	// 115 s +
  0x08, 0x08, 0x7E, 0x48, 0x08,	// 116 t +
  0x3C, 0x40, 0x40, 0x40, 0x3C,	// 117 u +
  0x0C, 0x30, 0x40, 0x30, 0x0C,	// 118 v +
  0x3C, 0x40, 0x30, 0x40, 0x3C,	// 119 w +
  0x44, 0x28, 0x10, 0x28, 0x44,	// 120 x +
  0x27, 0x48, 0x38, 0x08, 0x07,	// 121 y +
  0x00, 0x64, 0x54, 0x4C, 0x44,	// 122 z +
  0x00, 0x08, 0x77, 0x41, 0x00,	// 123 { +
  0x00, 0x00, 0x7F, 0x00, 0x00,	// 124 | +
  0x00, 0x41, 0x77, 0x08, 0x00,	// 125 } +
  0x08, 0x04, 0x08, 0x10, 0x08	// 126 ~ +
};

#ifdef _SSD1306_DisplaySize128x64
// are any flags for dirty set?
int SSD1306_IsDirty (void)
{
  for (int i = 0; i < 8; ++i)
    if (_DispDirty[i])
      return 1;
  return 0;
}
#endif

#ifdef _SSD1306_DisplaySize128x32
// are any flags for dirty set?
int SSD1306_IsDirty (void)
{
  for (int i = 0; i < 4; ++i)
    if (_DispDirty[i])
      return 1;
  return 0;
}
#endif

void SSD1306_Command8 (unsigned char command)
{
  // send device address, intent to write
  if (I2C_Start(_SSD1306_ADDRESS, I2C_WRITE))
    return;
  
  // write command, more data, so no stop
  if (I2C_Write8(0x00, I2C_NOSTOP))
    return;
  
  // write the command
  if (I2C_Write8(command, I2C_STOP))
    return;
}

void SSD1306_Command16 (unsigned char commandA, unsigned char commandB)
{
  // send device address, intent to write
  if (I2C_Start(_SSD1306_ADDRESS, I2C_WRITE))
    return;
  
  // write command, more data, so no stop
  if (I2C_Write8(0x00, I2C_NOSTOP))
    return;
  
  // write the command
  if (I2C_Write8(commandA, I2C_NOSTOP))
    return;

  // write the command
  if (I2C_Write8(commandB, I2C_STOP))
    return;
}

void SSD1306_Data (unsigned char * data, unsigned int iCount)
{
  // send device address, intent to write
  if (I2C_Start(_SSD1306_ADDRESS, I2C_WRITE))
    return;

  // write data, more data, so no stop
  if (I2C_Write8(0x40, I2C_NOSTOP))
    return;
  
  // do sequential writes, no stop until last byte
  for (unsigned int i = 0; i < iCount - 1; ++i)
  {
    if (I2C_Write8(data[i], I2C_NOSTOP))
      return;
  }
  
  // last byte so issue stop
  if (I2C_Write8(data[iCount - 1], I2C_STOP))
    return;
}

#ifdef _SSD1306_DisplaySize128x64
void SSD1306_DispInit (SSD1306_Orientation screen_dir)
{
  SSD1306_Command16 (0xA8, 0b10111111);   // set multiplex ratio P31 (default) (dim)
  //SSD1306_Command16 (0xA8, 0b10001111);   // set multiplex ratio P31 (16)
  
  SSD1306_Command16 (0xD3, 0x00); // set display offset P31
  SSD1306_Command8 (0x40);        // set display start line
  
  if (screen_dir)
  {
    SSD1306_Command8 (0xA1);        // set segment remap
    SSD1306_Command8 (0xC8);        // set com output map direction
  }
  else
  {
    SSD1306_Command8 (0xA0);        // set segment remap
    SSD1306_Command8 (0xC0);        // set com output map direction
  }
  
  SSD1306_Command16 (0xDA, 0b00010010); // set com pins hardware config (alternative) (default?)

  SSD1306_Command16 (0x81, 0x7F); // set contrast (1/2 level)
  SSD1306_Command8 (0xA4);        // display on, use RAM
  SSD1306_Command8 (0xA6);        // set normal display (1 == pixel on)
  
  SSD1306_Command16 (0xD5, 0x80); // set display clock to defaults
  SSD1306_Command16 (0x8D, 0x14); // this is critical, final pages (separate charge pump section)
  
  //SSD1306_Command16 (0xD9, 0b00100010); // pre-charge period (default)
  //SSD1306_Command16 (0xD9, 0b01000100); // pre-charge period (double?) not sure if this changed much
  
  SSD1306_Command8 (0xAF);        // display on, normal mode
  
  SSD1306_Command16 (0x20, 0x02); // page mode
  
  SSD1306_Clear();                // ram will be scrambled eggs, so clear display
}
#endif

#ifdef _SSD1306_DisplaySize128x32
void SSD1306_DispInit (SSD1306_Orientation screen_dir)
{
  SSD1306_Command16 (0xA8, 0x1F);   // set multiplex ratio P31 (default) (dim)
  
  SSD1306_Command16 (0xD3, 0x00); // set display offset P31
  SSD1306_Command8 (0x40);        // set display start line

  if (screen_dir)
  {
    SSD1306_Command8 (0xA1);        // set segment remap
    SSD1306_Command8 (0xC8);        // set com output map direction
  }
  else
  {
    SSD1306_Command8 (0xA0);        // set segment remap
    SSD1306_Command8 (0xC0);        // set com output map direction
  }
  
  SSD1306_Command16 (0xDA, 0x02); // set com pins hardware config (alternative) (default?)

  SSD1306_Command16 (0x81, 0x7F); // set contrast (1/2 level)
  SSD1306_Command8 (0xA4);        // display on, use RAM
  SSD1306_Command8 (0xA6);        // set normal display (1 == pixel on)
  
  SSD1306_Command16 (0xD5, 0x80); // set display clock to defaults
  SSD1306_Command16 (0x8D, 0x14); // this is critical, final pages (separate charge pump section)
  
  SSD1306_Command8 (0xAF);        // display on, normal mode
  
  SSD1306_Command16 (0x20, 0x02); // page mode
  
  SSD1306_Clear();                // ram will be scrambled eggs, so clear display
}
#endif

// no charge pump change (yet)
void SSD1306_DisplayOn (void)
{
  SSD1306_Command8 (0xAF);        // display on, normal mode
}

// no charge pump change (yet)
void SSD1306_DisplayOff (void)
{
  SSD1306_Command8 (0xAE);        // display sleep
}

#ifdef _SSD1306_DisplaySize128x64
// fill in ram with random junk
void SSD1306_Noise (void)
{
  for (int i = 0; i < 1024; ++i)
    _DispBuff[i] = rand() % 256;
  
  for (int i = 0; i < 8; ++i)
    _DispDirty [i] = 1;
  
  SSD1306_Render();
}
#endif

#ifdef _SSD1306_DisplaySize128x32
// fill in ram with random junk
void SSD1306_Noise (void)
{
  for (int i = 0; i < 512; ++i)
    _DispBuff[i] = rand() % 256;
  
  for (int i = 0; i < 4; ++i)
    _DispDirty [i] = 1;
  
  SSD1306_Render();
}
#endif

#ifdef _SSD1306_DisplaySize128x64
void SSD1306_Clear (void)
{
  for (int i = 0; i < 1024; ++i)
    _DispBuff[i] = 0;

  for (int i = 0; i < 8; ++i)
    _DispDirty [i] = 1;

  SSD1306_Render ();
}
#endif

#ifdef _SSD1306_DisplaySize128x32
void SSD1306_Clear (void)
{
  for (int i = 0; i < 512; ++i)
    _DispBuff[i] = 0;

  for (int i = 0; i < 4; ++i)
    _DispDirty [i] = 1;

  SSD1306_Render ();
}
#endif

#ifdef _SSD1306_DisplaySize128x64
void SSD1306_Render (void)
{
  // ensure we are at column 0 before bank output starts
  SSD1306_Command8 (0x00);    // col 0
  SSD1306_Command8 (0x10);    // col 0

  // render each page, as necessary
  for (int i = 0; i < 8; ++i)
  {
    // only update this area if dirty
    if (_DispDirty[i])
    {
      // mark as clean and render
      _DispDirty[i] = 0;
      SSD1306_Command8 (0xB0 + i);  // set bank
      SSD1306_Data(_DispBuff + i * 128, 128); // dump bank
    }
  }
}
#endif

#ifdef _SSD1306_DisplaySize128x32
void SSD1306_Render (void)
{
  // ensure we are at column 0 before bank output starts
  SSD1306_Command8 (0x00);    // col 0
  SSD1306_Command8 (0x10);    // col 0

  // render each page, as necessary
  for (int i = 0; i < 4; ++i)
  {
    // only update this area if dirty
    if (_DispDirty[i])
    {
      // mark as clean and render
      _DispDirty[i] = 0;
      SSD1306_Command8 (0xB0 + i);  // set bank
      SSD1306_Data(_DispBuff + i * 128, 128); // dump bank
    }
  }
}
#endif

#ifdef _SSD1306_DisplaySize128x64
void SSD1306_SetPage (int page, PGM_P buff)
{
    if (page < 0 || page > 7)
      return;
    
    _DispDirty[page] = 1;
    
    // copy out of flash to local display buffer
    memcpy_P (_DispBuff + page * 128, buff, 128);
    
    //for (int iScan = 0; iScan < 128; ++iScan)
    //  _DispBuff[page * 128 + iScan] = buff[iScan];
}
#endif

#ifdef _SSD1306_DisplaySize128x32
void SSD1306_SetPage (int page, PGM_P buff)
{
    if (page < 0 || page > 4)
      return;
    
    _DispDirty[page] = 1;
    
    // copy out of flash to local display buffer
    memcpy_P (_DispBuff + page * 128, buff, 128);
    
    //for (int iScan = 0; iScan < 128; ++iScan)
    //  _DispBuff[page * 128 + iScan] = buff[iScan];
}
#endif

#ifdef _SSD1306_DisplaySize128x64
void SSD1306_SetPixel (int iX, int iY)
{
  // stay in range or do nothing
  if (iX < 0 || iX > 127)
    return;
  
  if (iY < 0 || iY > 63)
    return;
  
  // figure out in memory where this is and set bit
  int iByte = iX + (iY / 8) * 128;
  _DispBuff[iByte] |= 1 << (iY % 8);
  
  // mark affected bank as dirty
  _DispDirty[iY/8] = 1;
}
#endif

#ifdef _SSD1306_DisplaySize128x32
void SSD1306_SetPixel (int iX, int iY)
{
  // stay in range or do nothing
  if (iX < 0 || iX > 127)
    return;
  
  if (iY < 0 || iY > 31)
    return;
  
  // figure out in memory where this is and set bit
  int iByte = iX + (iY / 8) * 128;
  _DispBuff[iByte] |= 1 << (iY % 8);
  
  // mark affected bank as dirty
  _DispDirty[iY/8] = 1;
}
#endif

void SSD1306_SetInverse (int IsInverse)
{
	if (IsInverse)
		return SSD1306_Command8(0xA7);
	else
		return SSD1306_Command8(0xA6);
}

int SSD1306_Max (int iA, int iB)
{
  return (iA > iB) ? iA : iB;  
}

int SSD1306_Min (int iA, int iB)
{
  return (iA < iB) ? iA : iB;
}

void SSD1306_Order (int *iA, int *iB)
{
  if (*iA < *iB)
    return;
  
  int temp = *iA;
  *iA = *iB;
  *iB = temp;
}

void SSD1306_Circle (int iXS, int iYS, float fRad)
{
  for (float fAng = 0; fAng <= 2 * M_PI; fAng += 0.025f)
  {
    SSD1306_SetPixel((int)(cos(fAng) * fRad + iXS), (int)(sin (fAng) * fRad + iYS));
  }
}

void SSD1306_Line (int iXS, int iYS, int iXE, int iYE)
{
  // validate that start and end are in range and in order
  if (iXS < 0 || iXS > 127 || iYS < 0 || iYS > 63 || iXE < 0 || iXE > 127 || iYE < 0 || iYE > 63)
    return;
  
  // now step through the larger of the two axes displacements and move in 1/2 steps
  int iXDisp = abs (iXE - iXS);
  int iYDisp = abs (iYE - iYS);

  int Steps = SSD1306_Max(iXDisp, iYDisp);
  if (!Steps)
  {
    // this is a single pixel line
    SSD1306_SetPixel(iXS, iYS);
    return;
  }
  
  // this is not a single pixel line, but it could be horizontal or vertical
  float fXStep = (iXE - iXS) / (float)Steps;
  float fYStep = (iYE - iYS) / (float)Steps;
  
  // go through the steps, increasing the axes steps and filling pixels
  float fXPos = iXS;
  float fYPos = iYS;
  
  // todo: modify for oversampling (2x?)
  for (int i = 0; i < Steps; ++i)
  {
    SSD1306_SetPixel((int)fXPos, (int)fYPos);
    
    fXPos += fXStep;
    fYPos += fYStep;
  }
}

#ifdef _SSD1306_DisplaySize128x64
// target locations are aligned to stops of 6 on the x, and 8 on the y, with 5 x 7 characters
void SSD1306_CharXY (unsigned char iX, unsigned char iY, char disp)
{
  // ensure axes in range, could be called with automation, so no error, just fix
  iX = iX % 22;
  iY = iY % 8;
  
  // keep display character in range or show as space
  if (disp < 31 || disp > 126)
    disp = ' ';
  
  // figure out where in the buffer this is
  int iStartIndex = iX * 6 + iY * 128;
  
  // copy the appropriate pixel map bits into this location
  //for (int i = 0; i < 5; ++i)
    //_DispBuff[iStartIndex++] = _CharMap [(disp - 31) * 5 + i]; // need to adjust to use full map
    // writing it this way does not appear to be necessary as the array type above is known
  //  _DispBuff[iStartIndex++] = *(const __flash unsigned char *)(_CharMap + ((disp - 31) * 5 + i)); // need to adjust to use full map
  
  // updated when switched to basic AVR code, uses program memory copy function to copy from flash
  memcpy_P (_DispBuff + iStartIndex, _CharMap + (disp - 31) * 5, 5);
  
  // mark affected page as dirty
  _DispDirty[iY] = 1;
}
#endif

#ifdef _SSD1306_DisplaySize128x32
// target locations are aligned to stops of 6 on the x, and 8 on the y, with 5 x 7 characters
void SSD1306_CharXY (unsigned char iX, unsigned char iY, char disp)
{
  // ensure axes in range, could be called with automation, so no error, just fix
  iX = iX % 22;
  iY = iY % 4;
  
  // keep display character in range or show as space
  if (disp < 31 || disp > 126)
    disp = ' ';
  
  // figure out where in the buffer this is
  int iStartIndex = iX * 6 + iY * 128;
  
  // copy the appropriate pixel map bits into this location
  //for (int i = 0; i < 5; ++i)
    //_DispBuff[iStartIndex++] = _CharMap [(disp - 31) * 5 + i]; // need to adjust to use full map
    // writing it this way does not appear to be necessary as the array type above is known
  //  _DispBuff[iStartIndex++] = *(const __flash unsigned char *)(_CharMap + ((disp - 31) * 5 + i)); // need to adjust to use full map
  
  // updated when switched to basic AVR code, uses program memory copy function to copy from flash
  memcpy_P (_DispBuff + iStartIndex, _CharMap + (disp - 31) * 5, 5);
  
  // mark affected page as dirty
  _DispDirty[iY] = 1;
}
#endif

void SSD1306_StringXY (unsigned char iX, unsigned char iY, char * pStr)
{
  while (*pStr)
  {
    // dump the current character to the display
    SSD1306_CharXY(iX, iY, *pStr);
    
    // move to the next character
    pStr = pStr + 1;
    
    // do row and column wrapping
    if (++iX > 20)
    {
      iX=0;
      if (++iY > 7)
        iY=0;
    }
  }
}