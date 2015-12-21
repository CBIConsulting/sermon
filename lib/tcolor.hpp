/* @(#)tcolor.hpp
 */

#ifndef _TCOLOR_HPP
#define _TCOLOR_HPP 1

#include <iostream>

class TColor
{
 public:
  TColor(unsigned short c)
    {
      setColor(c);
      background=-1;
    }

  TColor(unsigned short fc, unsigned short bc)
    {
      setColor(fc);
      background=(bc<BRILLO_MIN)?ansicolors[bc & 15]+10:-1;
    }

  friend std::ostream& operator <<(std::ostream& out, TColor c)
    {
      if (c.background>-1)
	out<<"\033["<<c.background<<"m";

      out<<"\033["<<c.attributes<<";"<<c.foreground<<"m";
      return out;
    }

  static const unsigned short  BLACK          = 0;
  static const unsigned short  BLUE           = 1;
  static const unsigned short  GREEN          = 2;
  static const unsigned short  CYAN           = 3;
  static const unsigned short  RED            = 4;
  static const unsigned short  MAGENTA        = 5;
  static const unsigned short  BROWN          = 6;
  static const unsigned short  LIGHTGRAY      = 7;
  static const unsigned short  DARKGRAY       = 8;
  static const unsigned short  LIGHTBLUE      = 9;
  static const unsigned short  LIGHTGREEN     = 10;
  static const unsigned short  LIGHTCYAN      = 11;
  static const unsigned short  LIGHTRED       = 12;
  static const unsigned short  LIGHTMAGENTA   = 13;
  static const unsigned short  YELLOW         = 14;
  static const unsigned short  WHITE          = 15;
  /* Special attributes */
  static const unsigned short  UNDERLINE      = 64;
  static const unsigned short  BLINK          = 128;

 private:
  inline void setColor(unsigned short c)
  {
    attributes=0;
    if (c & UNDERLINE) 
      attributes=UNDERLINE_ATTR;
    else if (c & BLINK)
      attributes=BLINK_ATTR;
    else if (c>=BRILLO_MIN)
      attributes=BRIGHT_ATTR;

    foreground=ansicolors[c & 15];      /* Eliminamos los atributos */
  }
  /* Color codes */
  static const short ansicolors[16];

  /* Special attributes for letters */
  static const unsigned short  UNDERLINE_ATTR = 4;
  static const unsigned short  BLINK_ATTR     = 5;
  static const unsigned short  BRIGHT_ATTR    = 1;

  /* Bright colors start on 9th color */
  static const unsigned short  BRILLO_MIN     = 9;

  unsigned short foreground;
  short background;
  unsigned short attributes;
};

inline std::ostream& endColor(std::ostream &s)
{
  return s<<"\033[00m";
}

#endif /* _TCOLOR_HPP */

