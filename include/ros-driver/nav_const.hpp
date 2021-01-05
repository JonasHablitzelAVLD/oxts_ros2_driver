/**
 * @file nav_const.hpp
 * Basic definitions of constants to be used across codebases
 */

#ifndef NAV_CONST_HPP
#define NAV_CONST_HPP

#include <math.h>

/**
 * Collection of useful constants
 */
namespace NAV_CONST
{
  /**
   * Convert from seconds to nanoseconds
   */
  const int SECS2NANOSECS =  1e9;
  /**
   * Convert from degrees to radians
   */
  const double DEG2RADS   =  M_PI / 180.0;
  /**
   * Convert from radians to degrees
   */
  const double RADS2DEG   =  180.0 / M_PI;


  enum GNSS_MODE
  {
    NONE            =  0,
    SEARCH          =  1,
    DOPPLER         =  2,
    SPS             =  3,
    DIFF            =  4,
    FLOAT           =  5,
    INTEGER         =  6,
    WAAS            =  7,
    OMNISTAR        =  8,
    OMNISTARHP      =  9,
    NODATA          = 10,
    BLANKED         = 11,
    PP_DOPPLER      = 12,
    PP_SPS          = 13,
    PP_DIFF         = 14,
    PP_FLOAT        = 15,
    PP_INTEGER      = 16,
    OMNISTARXP      = 17,
    CDGPS           = 18,
    NOT_KNOWN       = 19,
    GX_DOPPLER      = 20,
    GX_SPS          = 21,
    GX_DIFF         = 22,
    GX_FLOAT        = 23,
    GX_INTEGER      = 24,
    IX_DOPPLER      = 25,
    IX_SPS          = 26,
    IX_DIFF         = 27,
    IX_FLOAT        = 28,
    IX_INTEGER      = 29,
    PPP_CONVERGING  = 30,
    PPP             = 31,
    GX_SBAS         = 32,
    IX_SBAS         = 33,
    GENAID          = 34,
    SEGMENT         = 35,
    UNKNOWN         = 36
  };


}


#endif // NAV_CONST_HPP