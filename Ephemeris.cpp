/*
 * Ephemeris.cpp
 *
 * Copyright (c) 2017 by Sebastien MARCHAND (Web:www.marscaper.com, Email:sebastien@marscaper.com)
 */
/*
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#if ARDUINO
#include <Arduino.h>
#endif
#include <stdio.h>
#include <math.h>

#include "Ephemeris.hpp"


#ifndef PI
#define PI 3.1415926535
#endif

// Trigonometry using degrees
#define SIND(value)   sin((value*PI)/180)
#define COSD(value)   cos((value*PI)/180)
#define TAND(value)   tan((value*PI)/180)
#define ASIND(value) asin((value*PI)/180)
#define ACOSD(value) acos((value*PI)/180)
#define ATAND(value) atan((value*PI)/180)

// Limit range
#define LIMIT_DEGREES_TO_360(value) value >= 0 ? (value-(long)(value/360)*360) : ((value-(long)(value/360)*360)+360)
#define LIMIT_HOURS_TO_24(value) value >= 0 ? (value-(long)(value/24)*24) : value+24

// Convert degrees
#define DEGREES_TO_RADIANS(value) (value*PI/180)
#define DEGREES_TO_FLOATING_HOURS(value) (value/15.0)
#define DEGREES_MINUTES_SECONDES_TO_SECONDS(deg,min,sec) ((float)deg*3600+(float)min*60+(float)sec)
#define DEGREES_MINUTES_SECONDS_TO_DECIMAL_DEGREES(deg,min,sec) deg >= 0 ? ((float)deg+(float)min/60+(float)sec/3600) : ((float)deg-(float)min/60-(float)sec/3600)

// Convert radians
#define RADIANS_TO_DEGREES(value) (value*180/PI)
#define RADIANS_TO_HOURS(value) (value*3.81971863)

// Convert hours
#define HOURS_TO_RADIANS(value) (value*0.261799388)
#define HOURS_MINUTES_SECONDS_TO_SECONDS(hour,min,sec) ((float)hour*3600+(float)min*60+(float)sec)
#define HOURS_MINUTES_SECONDS_TO_DECIMAL_HOURS(hour,min,sec) ((float)hour+(float)min/60+(float)sec/3600)

// Convert seconds
#define SECONDS_TO_DECIMAL_DEGREES(value) ((float)value/3600)
#define SECONDS_TO_DECIMAL_HOURS(value) ((float)value/3600)

// Observer's coordinates on Earth
static float latitudeOnEarth  = NAN;
static float longitudeOnEarth = NAN;

void Ephemeris::floatingHoursToHoursMinutesSeconds(float floatingHours, int *hours, int *minutes, float *seconds)
{
    // Calculate hour,minute,second
    if( floatingHours >= 0 )
    {
        *hours   = (int)floatingHours;
        *minutes = floatingHours*60-*hours*60;
        *seconds = floatingHours*3600-*hours*3600-*minutes*60;
    }
    else
    {
        floatingHours = floatingHours*-1;
        
        *hours   = (int)floatingHours;
        *minutes = floatingHours*60-*hours*60;
        *seconds = floatingHours*3600-*hours*3600-*minutes*60;
        
        *hours = *hours*-1;
    }
}

float Ephemeris::hoursMinutesSecondsToFloatingHours(int degrees, int minutes, float seconds)
{
    return HOURS_MINUTES_SECONDS_TO_DECIMAL_HOURS(degrees, minutes, seconds);
}

void Ephemeris::floatingDegreesToDegreesMinutesSeconds(float floatintDegrees, int *degree, int *minute, float *second)
{
    // Calculate hour,minute,second
    if( floatintDegrees >= 0 )
    {
        *degree   = (unsigned int)floatintDegrees;
        *minute = floatintDegrees*60-*degree*60;
        *second = floatintDegrees*3600-*degree*3600-*minute*60;
    }
    else
    {
        floatintDegrees = floatintDegrees*-1;
        
        *degree   = (unsigned int)floatintDegrees;
        *minute = floatintDegrees*60-*degree*60;
        *second = floatintDegrees*3600-*degree*3600-*minute*60;
        
        *degree = *degree*-1;
    }
    
    return;
}

float Ephemeris::degreesMinutesSecondsToFloatingDegrees(int degrees, int minutes, float seconds)
{
    return DEGREES_MINUTES_SECONDS_TO_DECIMAL_DEGREES(degrees,minutes,seconds);
}

float Ephemeris::apparentSideralTime(unsigned int day,  unsigned int month,  unsigned int year,
                                     unsigned int hours, unsigned int minutes, unsigned int seconds)
{
    JulianDay jd = Calendar::julianDayForDate(day, month, year);
    
    float T        = (jd.day-2451545.0+jd.time)/36525;
    float TSquared = T*T;
    float TCubed   = TSquared*T;
    
    float theta0 = 100.46061837 + T*36000.770053608 + TSquared*0.000387933  - TCubed/38710000;
    theta0 = LIMIT_DEGREES_TO_360(theta0);
    theta0 = DEGREES_TO_FLOATING_HOURS(theta0);
    
    float time = HOURS_MINUTES_SECONDS_TO_DECIMAL_HOURS(hours,minutes,seconds);
    
    float apparentSideralTime = theta0 + 1.00273790935 * time;
    apparentSideralTime = LIMIT_HOURS_TO_24(apparentSideralTime);
    
    return apparentSideralTime;
}

float Ephemeris::obliquityAndNutationForT(float T, float *deltaObliquity, float *deltaNutation)
{
    float TSquared = T*T;
    float TCubed   = TSquared*T;
    
    float Ls = 280.4565 + T*36000.7698   + TSquared*0.000303;
    Ls = LIMIT_DEGREES_TO_360(Ls);
    
    float Lm = 218.3164 + T*481267.8812  - TSquared*0.001599;
    Lm = LIMIT_DEGREES_TO_360(Lm);
    
    float Ms = 357.5291 + T*35999.0503   - TSquared*0.000154;
    Ms = LIMIT_DEGREES_TO_360(Ms);
    
    float Mm = 134.9634 + T*477198.8675  + TSquared*0.008721;
    Mm = LIMIT_DEGREES_TO_360(Mm);
    
    float omega = 125.0443 - T*1934.1363 + TSquared*0.008721;
    omega = LIMIT_DEGREES_TO_360(omega);
    
    // Delta Phi
    float dNutation =
    -(17.1996 + 0.01742*T) * SIND(omega)
    -(1.3187  + 0.00016*T) * SIND(2*Ls)
    - 0.2274               * SIND(2*Lm)
    + 0.2062               * SIND(2*omega)
    +(0.1426  - 0.00034*T) * SIND(Ms)
    + 0.0712               * SIND(Mm)
    -(0.0517  - 0.00012*T) * SIND(2*Ls+Ms)
    - 0.0386               * SIND(2*Lm-omega)
    - 0.0301               * SIND(2*Lm+Mm)
    + 0.0217               * SIND(2*Ls-Ms)
    - 0.0158               * SIND(2*Ls-2*Lm+Mm)
    + 0.0129               * SIND(2*Ls-omega)
    + 0.0123               * SIND(2*Lm-Mm);
    
    if( deltaNutation )
    {
        *deltaNutation = dNutation;
    }
    
    // Delta Eps
    float dObliquity =
    +(9.2025  + 0.00089*T) * COSD(omega)
    +(0.5736  - 0.00031*T) * COSD(2*Ls)
    + 0.0977               * COSD(2*Lm)
    - 0.0895               * COSD(2*omega)
    + 0.0224               * COSD(2*Ls+Ms)
    + 0.0200               * COSD(2*Lm-omega)
    + 0.0129               * COSD(2*Lm + Mm)
    - 0.0095               * COSD(2*Ls-Ms)
    - 0.0070               * COSD(2*Ls-omega);
    
    if( deltaObliquity )
    {
        *deltaObliquity = dObliquity;
    }
    
    float eps0 = DEGREES_MINUTES_SECONDES_TO_SECONDS(23,26,21.448)-T*46.8150-TSquared*0.00059+TCubed*0.001813;
    
    float obliquity = eps0 + dObliquity;
    obliquity = SECONDS_TO_DECIMAL_DEGREES(obliquity);
    
    return obliquity;
}

EquatorialCoordinates  Ephemeris::equatorialCoordinatesForSunAtJD(JulianDay jd, float *distance, GeocentricCoordinates *gCoordinates)
{
    EquatorialCoordinates sunCoordinates;
    
    float T        = (jd.day-2451545.0+jd.time)/36525;
    float TSquared = T*T;
    
    float L0 = 280.46646 + T*36000.76983 + TSquared*0.0003032;
    L0 = LIMIT_DEGREES_TO_360(L0);
    
    float M = 357.52911 + T*35999.05029  - TSquared*0.0001537;
    M = LIMIT_DEGREES_TO_360(M);
    
    float e = 0.016708634 - T*0.000042037 - TSquared*0.0000001267;
    
    float C =
    +(1.914602 - T*0.004817 - TSquared*0.000014) * SIND(M)
    +(0.019993 - T*0.000101                    ) * SIND(2*M)
    + 0.000289                                   * SIND(3*M);
    
    float O = L0 + C;
    
    float v = M  + C;
    
    // Improved precision for O according to page 65
    {
        float Av = 351.52 + 22518.4428*T;  // Mars
        float Bv = 253.14 + 45036.8857*T;  // Venus
        float Cj = 157.23 + 32964.4673*T;  // Jupiter
        float Dm = 297.85 + 445267.1117*T; // Moon
        float E  = 252.08 + 20.19 *T;
        //float H  = 42.43  + 65928.9358*T;
        
        O +=
        + 0.00134 * COSD(Av)
        + 0.00153 * COSD(Bv)
        + 0.00200 * COSD(Cj)
        + 0.00180 * SIND(Dm)
        + 0.00196 * SIND(E);
        
        /*v +=
         + 0.00000542 * SIND(Av)
         + 0.00001576 * SIND(Bv)
         + 0.00001628 * SIND(Cj)
         + 0.00003084 * COSD(Dm)
         + 0.00000925 * SIND(H);*/
    }
    
    // R
    if( distance )
    {
        *distance = (1.000001018*(1-e*e))/(1+e*COSD(v));
    }
    
    float omega = 125.04 - 1934.136*T;
    
    float lambda = O - 0.00569 - 0.00478 * SIND(omega);
    
    float eps = obliquityAndNutationForT(T, NULL, NULL);
    
    eps += 0.00256*COSD(omega);
    
    // Alpha   (Hour=Deg/15.0)
    sunCoordinates.ra = atan(COSD(eps)*TAND(lambda))*12/PI;
    sunCoordinates.ra = LIMIT_HOURS_TO_24(sunCoordinates.ra);
    
    // Delta
    sunCoordinates.dec = asin(SIND(eps)*SIND(lambda))*180.0/PI;
    
    return sunCoordinates;
}

PlanetayOrbit Ephemeris::planetayOrbitForPlanetAndT(SolarSystemObjectIndex solarSystemObjectIndex, float T)
{
    PlanetayOrbit planetayOrbit;
    
    float TSquared = T*T;
    float TCubed   = TSquared*T;
    
    switch (solarSystemObjectIndex)
    {
        case Mercury:
            planetayOrbit.L     = 252.250906   + 149474.0722491*T + 0.00030350*TSquared   + 0.000000018*TCubed;
            planetayOrbit.a     = 0.387098310;
            planetayOrbit.e     = 0.20563175   + 0.000020407*T    - 0.0000000283*TSquared - 0.00000000018*TCubed;
            planetayOrbit.i     = 7.004986     + 0.0018215*T      - 0.00001810*TSquared   + 0.000000056*TCubed;
            planetayOrbit.omega = 48.330893    + 1.1861883*T      + 0.00017542*TSquared   + 0.000000215*TCubed;
            planetayOrbit.pi    = 77.456119    + 1.5564776*T      + 0.00029544*TSquared   + 0.000000009*TCubed;
            break;
            
        case Venus:
            planetayOrbit.L     = 181.979801   + 58519.2130302*T + 0.00031014*TSquared   + 0.000000015*TCubed;
            planetayOrbit.a     = 0.723329820;
            planetayOrbit.e     = 0.00677192   - 0.000047765*T   + 0.0000000981*TSquared + 0.00000000046*TCubed;
            planetayOrbit.i     = 3.394662     + 0.0010037*T     - 0.00000088*TSquared   - 0.000000007*TCubed;
            planetayOrbit.omega = 76.679920    + 0.9011206*T     + 0.00040618*TSquared   - 0.000000093*TCubed;
            planetayOrbit.pi    = 131.563703   + 1.4022288*T     - 0.00107618*TSquared   - 0.000005678*TCubed;
            break;
            
        case Earth:
            planetayOrbit.L     = 100.466457   + 36000.7698278*T + 0.00030322*TSquared   + 0.000000020*TCubed;
            planetayOrbit.a     = 1.000001018;
            planetayOrbit.e     = 0.01670863   - 0.000042037*T   - 0.0000001267*TSquared + 0.00000000014*TCubed;
            planetayOrbit.i     = 0;
            planetayOrbit.omega = NAN;
            planetayOrbit.pi    = 102.937348   + 1.17195366*T    + 0.00045688*TSquared   - 0.000000018*TCubed;
            break;
            
        case Mars:
            planetayOrbit.L     = 355.433000   + 19141.6964471*T + 0.00031052*TSquared   + 0.000000016*TCubed;
            planetayOrbit.a     = 1.523679342;
            planetayOrbit.e     = 0.09340065   + 0.000090484*T   - 0.0000000806*TSquared - 0.00000000025*TCubed;
            planetayOrbit.i     = 1.849726     - 0.0006011*T     + 0.00001276*TSquared   - 0.000000007*TCubed;
            planetayOrbit.omega = 49.588093    + 0.7720959*T     + 0.00001557*TSquared   + 0.000002267*TCubed;
            planetayOrbit.pi    = 336.060234   + 1.8410449*T     + 0.00013477*TSquared   + 0.000000536*TCubed;
            break;
            
        case Jupiter:
            planetayOrbit.L     = 34.351519   + 3036.3027748*T  + 0.00022330*TSquared   + 0.000000037*TCubed;
            planetayOrbit.a     = 5.202603209 + 0.0000001913*T;
            planetayOrbit.e     = 0.04849793  + 0.000163225*T   - 0.0000004714*TSquared - 0.00000000201*TCubed;
            planetayOrbit.i     = 1.303267    - 0.0054965*T     + 0.00000466*TSquared   - 0.000000002*TCubed;
            planetayOrbit.omega = 100.464407  + 1.0209774*T     + 0.00040315*TSquared   + 0.000000404*TCubed;
            planetayOrbit.pi    = 14.331207   + 1.6126352*T     + 0.00103042*TSquared   - 0.000004464*TCubed;
            break;
            
        case Saturn:
            planetayOrbit.L     = 50.077444   + 1223.5110686*T + 0.00051908*TSquared   - 0.000000030*TCubed;
            planetayOrbit.a     = 9.554909192 - 0.0000021390*T + 0.000000004*TSquared;
            planetayOrbit.e     = 0.05554814  - 0.0003446641*T - 0.0000006436*TSquared + 0.00000000340*TCubed;
            planetayOrbit.i     = 2.488879    - 0.0037362*T    - 0.00001519*TSquared   + 0.000000087*TCubed;
            planetayOrbit.omega = 113.665503  + 0.8770880*T    - 0.00012176*TSquared   - 0.000002249*TCubed;
            planetayOrbit.pi    = 93.057237   + 1.9637613*T    + 0.00083753*TSquared   + 0.000004928*TCubed;
            break;
            
        case Uranus:
            planetayOrbit.L     = 314.055005   + 429.8640561*T  + 0.00030390*TSquared     + 0.000000026*TCubed;
            planetayOrbit.a     = 19.218446062 - 0.0000000372*T + 0.00000000098*TSquared;
            planetayOrbit.e     = 0.04638122   - 0.000027293*T  + 0.0000000789*TSquared   + 0.00000000024*TCubed;
            planetayOrbit.i     = 0.773197     + 0.0007744*T    + 0.00003749*TSquared     - 0.000000092*TCubed;
            planetayOrbit.omega = 74.005957    + 0.5211278*T    + 0.00133947*TSquared     + 0.000018484*TCubed;
            planetayOrbit.pi    = 173.005291   + 1.4863790*T    + 0.00021406*TSquared     + 0.000000434*TCubed;
            break;
            
        case Neptune:
            planetayOrbit.L     = 304.348665   + 219.8833092*T  + 0.00030882*TSquared     + 0.000000018*TCubed;
            planetayOrbit.a     = 30.110386869 - 0.0000001663*T + 0.00000000069*TSquared;
            planetayOrbit.e     = 0.00945575   + 0.000006033*T                            - 0.00000000005*TCubed;
            planetayOrbit.i     = 1.769953     - 0.0093082*T    - 0.00000708*TSquared     + 0.000000027*TCubed;
            planetayOrbit.omega = 131.784057   + 1.1022039*T    + 0.00025952*TSquared     - 0.000000637*TCubed;
            planetayOrbit.pi    = 48.120276    + 1.4262957*T    + 0.00038434*TSquared     + 0.000000020*TCubed;
            break;
            
        default:
            // Unknow planet
            break;
    }
    
    // Apply limitations
    planetayOrbit.L     = LIMIT_DEGREES_TO_360(planetayOrbit.L);
    planetayOrbit.i     = LIMIT_DEGREES_TO_360(planetayOrbit.i);
    planetayOrbit.omega = LIMIT_DEGREES_TO_360(planetayOrbit.omega);
    planetayOrbit.pi    = LIMIT_DEGREES_TO_360(planetayOrbit.pi);
    
    // Mean anomalie
    planetayOrbit.M = planetayOrbit.L - planetayOrbit.pi;
    planetayOrbit.M = LIMIT_DEGREES_TO_360(planetayOrbit.M);
    
    planetayOrbit.w = planetayOrbit.pi-planetayOrbit.omega;
    planetayOrbit.w = LIMIT_DEGREES_TO_360(planetayOrbit.w);
    
    return planetayOrbit;
}

float Ephemeris::kepler(float M, float e)
{
    M = DEGREES_TO_RADIANS(M);
    
    float E = M;
    
    float previousE = E+1;
    
    for(int i=0; i<10; i++ )
    {
        E = E + (M+e*sin(E)-E)/(1-e*cos(E));
        
        // Optimize iterations
        if( previousE == E )
        {
            // No more iteration needed.
            break;
        }
        else
        {
            // Next iteration
            previousE = E;
        }
    }
    
    return RADIANS_TO_DEGREES(E);
}

float Ephemeris::sumVSOP87Coefs(const VSOP87Coefficient *valuePlanetCoefficients, int coefCount, float T)
{
    // Parse each value in coef table
    float value = 0;
    for(int numCoef=0; numCoef<coefCount; numCoef++)
    {
        // Get coef
        VSOP87Coefficient coef;
        
#if ARDUINO
        // We limit SRAM usage by using flash memory (PROGMEM)
        memcpy_P(&coef, &valuePlanetCoefficients[numCoef], sizeof(VSOP87Coefficient));
#else
        coef = valuePlanetCoefficients[numCoef];
#endif
        
        float res = cos(coef.B + coef.C*T);
        
        // To avoid out of range issue with single precision
        // we've stored sqrt(A) and not A. As a result we need to square it back.
        res *= coef.A;
        res *= coef.A;
        
        value += res;
    }
    
    return value;
}

HorizontalCoordinates Ephemeris::equatorialToHorizontal(float H, float delta, float phi)
{
    HorizontalCoordinates coordinates;
    
    coordinates.azi = atan2(SIND(H), COSD(H)*SIND(phi)-TAND(delta)*COSD(phi));
    coordinates.azi = RADIANS_TO_DEGREES(coordinates.azi)+180;
    coordinates.azi = LIMIT_DEGREES_TO_360(coordinates.azi);
    
    coordinates.alt = asin(SIND(phi)*SIND(delta) + COSD(phi)*COSD(delta)*COSD(H));
    coordinates.alt = RADIANS_TO_DEGREES(coordinates.alt);
    
    return coordinates;
}

EquatorialCoordinates Ephemeris::EclipticToEquatorial(float lambda, float beta, float epsilon)
{
    lambda  = DEGREES_TO_RADIANS(lambda);
    beta    = DEGREES_TO_RADIANS(beta);
    epsilon = DEGREES_TO_RADIANS(epsilon);
    
    EquatorialCoordinates coordinates;
    coordinates.ra = atan2(sin(lambda)*cos(epsilon) - tan(beta)*sin(epsilon), cos(lambda));
    coordinates.ra = RADIANS_TO_HOURS(coordinates.ra);
    coordinates.ra = LIMIT_HOURS_TO_24(coordinates.ra);
    
    coordinates.dec = asin(sin(beta)*cos(epsilon) + cos(beta)*sin(epsilon)*sin(lambda));
    coordinates.dec = RADIANS_TO_DEGREES(coordinates.dec );
    
    return coordinates;
}

RectangularCoordinates Ephemeris::HeliocentricToRectangular(HeliocentricCoordinates hc, HeliocentricCoordinates hc0)
{
    RectangularCoordinates coordinates;
    
    coordinates.x = hc.radius * COSD(hc.lat) * COSD(hc.lon) - hc0.radius * COSD(hc0.lat) * COSD(hc0.lon);
    coordinates.y = hc.radius * COSD(hc.lat) * SIND(hc.lon) - hc0.radius * COSD(hc0.lat) * SIND(hc0.lon);
    coordinates.z = hc.radius * SIND(hc.lat)                - hc0.radius * SIND(hc0.lat);
    
    return coordinates;
}

float Ephemeris::meanGreenwichSiderealTimeAtDateAndTime(unsigned int day,   unsigned int month,   unsigned int year,
                                                        unsigned int hours, unsigned int minutes, unsigned int seconds)
{
    float meanGreenwichSiderealTime;
    
    JulianDay jd0 = Calendar::julianDayForDateAndTime(day, month, year, 0, 0, 0);
    float T0        = jd0.day/36525.0-2451545.0/36525.0+jd0.time/36525.0;
    float T0Squared = T0*T0;
    float T0Cubed   = T0Squared*T0;
    
    // Sideral time at midnight
    float theta0 = 100.46061837 + (36000.770053608*T0) + (0.000387933*T0Squared) - (T0Cubed/38710000);
    theta0 = LIMIT_DEGREES_TO_360(theta0);
    theta0 = DEGREES_TO_FLOATING_HOURS(theta0);
    
    // Sideral time of day
    float thetaH = 1.00273790935*HOURS_MINUTES_SECONDS_TO_SECONDS(hours,minutes,seconds);
    thetaH = SECONDS_TO_DECIMAL_HOURS(thetaH);
    
    // Add time at midnight and time of day
    meanGreenwichSiderealTime = theta0 + thetaH;
    
    return LIMIT_HOURS_TO_24(meanGreenwichSiderealTime);
}

SolarSystemObject Ephemeris::solarSystemObjectAtDateAndTime(SolarSystemObjectIndex solarSystemObjectIndex,
                                                            unsigned int day,   unsigned int month,   unsigned int year,
                                                            unsigned int hours, unsigned int minutes, unsigned int seconds)
{
    SolarSystemObject solarSystemObject;
    
    JulianDay jd = Calendar::julianDayForDateAndTime(day, month, year, hours, minutes, seconds);
    
    float T = jd.day/36525.0-2451545.0/36525.0+jd.time/36525.0;
    
    
    // Equatorial coordinates
    if( solarSystemObjectIndex == Sun )
    {
        solarSystemObject.equaCoordinates = equatorialCoordinatesForSunAtJD(jd,
                                                                            &solarSystemObject.distance,
                                                                            NULL);
    }
    else
    {
        solarSystemObject.equaCoordinates = equatorialCoordinatesForPlanetAtJD(solarSystemObjectIndex,
                                                                               jd,
                                                                               &solarSystemObject.distance,
                                                                               NULL);
    }
    
    // Apparent diameter at a distance of 1 astronomical unit.
    float diameter = 0;
    switch (solarSystemObjectIndex)
    {
        case Mercury:
            diameter = 6.728;
            break;
            
        case Venus:
            diameter = 16.688;
            break;
            
        case Earth:
            diameter = NAN;
            break;
            
        case Mars:
            diameter = 9.364;
            break;
            
        case Jupiter:
            diameter = 197.146;
            break;
            
        case Saturn:
            diameter = 166.197;
            break;
            
        case Uranus:
            diameter = 70.476;
            break;
            
        case Neptune:
            diameter = 68.285;
            break;
            
        case Sun:
            diameter = 1919.26;
            break;
            
        /*case EarthsMoon:
            // TODO
            break;*/
    }
    
    // Approximate apparent diameter in arc minutes according to distance
    solarSystemObject.diameter = diameter / solarSystemObject.distance/60;
    
    float meanSideralTime = meanGreenwichSiderealTimeAtDateAndTime(day, month, year, hours, minutes, seconds);
    
    float deltaNutation;
    float epsilon = obliquityAndNutationForT(T, NULL, &deltaNutation);
    
    // Apparent sideral time in floating hours
    float theta0 = meanSideralTime + (deltaNutation/15*COSD(epsilon))/3600;
    
    if( !isnan(longitudeOnEarth) && !isnan(latitudeOnEarth) )
    {
        // Geographic longitude in floating degrees
        float L = DEGREES_TO_FLOATING_HOURS(longitudeOnEarth);
        
        // Geographic latitude in floating degrees
        float phi = latitudeOnEarth;
        
        // Local angle in floating degrees
        float H = (theta0-L-solarSystemObject.equaCoordinates.ra)*15;
        
        solarSystemObject.horiCoordinates = equatorialToHorizontal(H,solarSystemObject.equaCoordinates.dec,phi);
    }
    else
    {
        solarSystemObject.horiCoordinates.alt = NAN;
        solarSystemObject.horiCoordinates.azi = NAN;
    }
    
    return solarSystemObject;
}

void Ephemeris::setLocationOnEarth(float floatingLatitude, float floatingLongitude)
{
    latitudeOnEarth  = floatingLatitude;
    longitudeOnEarth = floatingLongitude;
}

void Ephemeris::setLocationOnEarth(float latDegrees, float latMinutes, float latSeconds,
                                   float lonDegrees, float lonMinutes, float lonSeconds)
{
    latitudeOnEarth  = DEGREES_MINUTES_SECONDS_TO_DECIMAL_DEGREES(latDegrees,latMinutes,latSeconds);
    longitudeOnEarth = DEGREES_MINUTES_SECONDS_TO_DECIMAL_DEGREES(lonDegrees,lonMinutes,lonSeconds);
}

EquatorialCoordinates  Ephemeris::equatorialCoordinatesForPlanetAtJD(SolarSystemObjectIndex solarSystemObjectIndex, JulianDay jd,
                                                                     float *distance, GeocentricCoordinates *gCoordinates)
{
    EquatorialCoordinates coordinates;
    coordinates.ra  = 0;
    coordinates.dec = 0;
    
    float T      = (jd.day-2451545.0+jd.time)/36525;
    float lastT  = 0;
    float TLight = 0;
    HeliocentricCoordinates hcPlanet;
    HeliocentricCoordinates hcEarth;
    RectangularCoordinates  rectPlanet;
    
    JulianDay jd2;
    jd2.day  = jd.day;
    jd2.time = jd.time;
    
    float x2,y2,z2;
    
    // Iterate for good precision according to light speed delay
    while (T != lastT)
    {
        lastT = T;
        
        float jd2 = (jd.day+jd.time) - TLight;
        T = (jd2-2451545.0)/36525;
        
        hcPlanet   = Ephemeris::heliocentricCoordinatesForPlanetAndT(solarSystemObjectIndex, T);
        if( isnan(hcPlanet.radius)  )
        {
            break;
        }
        
        hcEarth    = Ephemeris::heliocentricCoordinatesForPlanetAndT(Earth, T);
        
        rectPlanet = HeliocentricToRectangular(hcPlanet,hcEarth);
        
        // Precomputed square
        x2 = rectPlanet.x*rectPlanet.x;
        y2 = rectPlanet.y*rectPlanet.y;
        z2 = rectPlanet.z*rectPlanet.z;
        
        // Real distance from Earth
        float delta = sqrtf(x2+y2+z2);
        
        if( distance )
        {
            *distance = delta;
        }
        
        // Light time
        TLight = delta * 0.0057755183;
    }
    
    
    // Geocentic longitude
    float lambda = atan2(rectPlanet.y,rectPlanet.x);
    lambda = RADIANS_TO_DEGREES(lambda);
    lambda = LIMIT_DEGREES_TO_360(lambda);
    
    // Geocentric latitude
    float beta   = rectPlanet.z/sqrtf(x2+y2);
    beta = RADIANS_TO_DEGREES(beta);
    
    
    // Remove abberation
    {
        PlanetayOrbit earthOrbit = Ephemeris::planetayOrbitForPlanetAndT(Earth, T);
        
        float TSquared = T*T;
        
        float L0 = 280.46646 + T*36000.76983 + TSquared*0.0003032;
        L0 = LIMIT_DEGREES_TO_360(L0);
        
        float M = 357.52911 + T*35999.05029  - TSquared*0.0001537;
        M = LIMIT_DEGREES_TO_360(M);
        
        float C =
        +(1.914602 - T*0.004817 - TSquared*0.000014) * SIND(M)
        +(0.019993 - T*0.000101                    ) * SIND(2*M)
        + 0.000289                                   * SIND(3*M);
        
        // Sun longitude
        float O = L0 + C;
        
        // Abberation
        float k = 20.49552;
        float xAberration = (-k*COSD(O - lambda) + earthOrbit.e*k*COSD(earthOrbit.pi - lambda)) / COSD(beta)/3600;
        float yAberration = -k*SIND(beta)*(SIND(O - lambda) - earthOrbit.e*SIND(earthOrbit.pi - lambda))/3600;
        lambda -= xAberration;
        beta   -= yAberration;
    }
    
    if( gCoordinates )
    {
        gCoordinates->lon = lambda;
        gCoordinates->lat = beta;
    }
    
    // Obliquity and Nutation
    float deltaNutation;
    float epsilon = Ephemeris::obliquityAndNutationForT(T, NULL, &deltaNutation);
    
    // Intergrate nutation
    lambda += deltaNutation/3600;
    
    return EclipticToEquatorial(lambda, beta, epsilon);
}

HeliocentricCoordinates  Ephemeris::heliocentricCoordinatesForPlanetAndT(SolarSystemObjectIndex solarSystemObjectIndex, float T)
{
    HeliocentricCoordinates coordinates;
    
    T = T/10;
    float TSquared = T*T;
    float TCubed   = TSquared*T;
    float T4       = TCubed*T;
    float T5       = T4*T;
    
    int SizeOfVSOP87Coefficient = sizeof(VSOP87Coefficient);
    
    float l0=0,l1=0,l2=0,l3=0,l4=0,l5=0;
    float b0=0,b1=0,b2=0,b3=0,b4=0,b5=0;
    float r0=0,r1=0,r2=0,r3=0,r4=0,r5=0;
    
    switch (solarSystemObjectIndex)
    {
        case Sun:
            break;
            
        case Mercury:
            
            l0 = sumVSOP87Coefs(L0MercuryCoefficients,sizeof(L0MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1MercuryCoefficients,sizeof(L1MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2MercuryCoefficients,sizeof(L2MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3MercuryCoefficients,sizeof(L3MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4MercuryCoefficients,sizeof(L4MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            l5 = sumVSOP87Coefs(L5MercuryCoefficients,sizeof(L5MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0MercuryCoefficients,sizeof(B0MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1MercuryCoefficients,sizeof(B1MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            b2 = sumVSOP87Coefs(B2MercuryCoefficients,sizeof(B2MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            b3 = sumVSOP87Coefs(B3MercuryCoefficients,sizeof(B3MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            b4 = sumVSOP87Coefs(B4MercuryCoefficients,sizeof(B4MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0MercuryCoefficients,sizeof(R0MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1MercuryCoefficients,sizeof(R1MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2MercuryCoefficients,sizeof(R2MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3MercuryCoefficients,sizeof(R3MercuryCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
            
        case Venus:
            
            l0 = sumVSOP87Coefs(L0VenusCoefficients,sizeof(L0VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1VenusCoefficients,sizeof(L1VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2VenusCoefficients,sizeof(L2VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3VenusCoefficients,sizeof(L3VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4VenusCoefficients,sizeof(L4VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            l5 = sumVSOP87Coefs(L5VenusCoefficients,sizeof(L5VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0VenusCoefficients,sizeof(B0VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1VenusCoefficients,sizeof(B1VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            b2 = sumVSOP87Coefs(B2VenusCoefficients,sizeof(B2VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            b3 = sumVSOP87Coefs(B3VenusCoefficients,sizeof(B3VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            b4 = sumVSOP87Coefs(B4VenusCoefficients,sizeof(B4VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0VenusCoefficients,sizeof(R0VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1VenusCoefficients,sizeof(R1VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2VenusCoefficients,sizeof(R2VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3VenusCoefficients,sizeof(R3VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            r4 = sumVSOP87Coefs(R4VenusCoefficients,sizeof(R4VenusCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
            
        case Earth:
            
            l0 = sumVSOP87Coefs(L0EarthCoefficients,sizeof(L0EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1EarthCoefficients,sizeof(L1EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2EarthCoefficients,sizeof(L2EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3EarthCoefficients,sizeof(L3EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4EarthCoefficients,sizeof(L4EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            l5 = sumVSOP87Coefs(L5EarthCoefficients,sizeof(L5EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0EarthCoefficients,sizeof(B0EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1EarthCoefficients,sizeof(B1EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0EarthCoefficients,sizeof(R0EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1EarthCoefficients,sizeof(R1EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2EarthCoefficients,sizeof(R2EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3EarthCoefficients,sizeof(R3EarthCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
            
        case Mars:
            
            l0 = sumVSOP87Coefs(L0MarsCoefficients,sizeof(L0MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1MarsCoefficients,sizeof(L1MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2MarsCoefficients,sizeof(L2MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3MarsCoefficients,sizeof(L3MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4MarsCoefficients,sizeof(L4MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            l5 = sumVSOP87Coefs(L5MarsCoefficients,sizeof(L5MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0MarsCoefficients,sizeof(B0MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1MarsCoefficients,sizeof(B1MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            b2 = sumVSOP87Coefs(B2MarsCoefficients,sizeof(B2MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            b3 = sumVSOP87Coefs(B3MarsCoefficients,sizeof(B3MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            b4 = sumVSOP87Coefs(B4MarsCoefficients,sizeof(B4MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0MarsCoefficients,sizeof(R0MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1MarsCoefficients,sizeof(R1MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2MarsCoefficients,sizeof(R2MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3MarsCoefficients,sizeof(R3MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            r4 = sumVSOP87Coefs(R4MarsCoefficients,sizeof(R4MarsCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
            
        case Jupiter:
            
            l0 = sumVSOP87Coefs(L0JupiterCoefficients,sizeof(L0JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1JupiterCoefficients,sizeof(L1JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2JupiterCoefficients,sizeof(L2JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3JupiterCoefficients,sizeof(L3JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4JupiterCoefficients,sizeof(L4JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            l5 = sumVSOP87Coefs(L5JupiterCoefficients,sizeof(L5JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0JupiterCoefficients,sizeof(B0JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1JupiterCoefficients,sizeof(B1JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            b2 = sumVSOP87Coefs(B2JupiterCoefficients,sizeof(B2JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            b3 = sumVSOP87Coefs(B3JupiterCoefficients,sizeof(B3JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            b4 = sumVSOP87Coefs(B4JupiterCoefficients,sizeof(B4JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            b5 = sumVSOP87Coefs(B5JupiterCoefficients,sizeof(B5JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0JupiterCoefficients,sizeof(R0JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1JupiterCoefficients,sizeof(R1JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2JupiterCoefficients,sizeof(R2JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3JupiterCoefficients,sizeof(R3JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            r4 = sumVSOP87Coefs(R4JupiterCoefficients,sizeof(R4JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            r5 = sumVSOP87Coefs(R5JupiterCoefficients,sizeof(R5JupiterCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
            
        case Saturn:
            
            l0 = sumVSOP87Coefs(L0SaturnCoefficients,sizeof(L0SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1SaturnCoefficients,sizeof(L1SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2SaturnCoefficients,sizeof(L2SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3SaturnCoefficients,sizeof(L3SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4SaturnCoefficients,sizeof(L4SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            l5 = sumVSOP87Coefs(L5SaturnCoefficients,sizeof(L5SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0SaturnCoefficients,sizeof(B0SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1SaturnCoefficients,sizeof(B1SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            b2 = sumVSOP87Coefs(B2SaturnCoefficients,sizeof(B2SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            b3 = sumVSOP87Coefs(B3SaturnCoefficients,sizeof(B3SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            b4 = sumVSOP87Coefs(B4SaturnCoefficients,sizeof(B4SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            b5 = sumVSOP87Coefs(B5SaturnCoefficients,sizeof(B5SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0SaturnCoefficients,sizeof(R0SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1SaturnCoefficients,sizeof(R1SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2SaturnCoefficients,sizeof(R2SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3SaturnCoefficients,sizeof(R3SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            r4 = sumVSOP87Coefs(R4SaturnCoefficients,sizeof(R4SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            r5 = sumVSOP87Coefs(R5SaturnCoefficients,sizeof(R5SaturnCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
            
        case Uranus:
            
            l0 = sumVSOP87Coefs(L0UranusCoefficients,sizeof(L0UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1UranusCoefficients,sizeof(L1UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2UranusCoefficients,sizeof(L2UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3UranusCoefficients,sizeof(L3UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4UranusCoefficients,sizeof(L4UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0UranusCoefficients,sizeof(B0UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1UranusCoefficients,sizeof(B1UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            b2 = sumVSOP87Coefs(B2UranusCoefficients,sizeof(B2UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            b3 = sumVSOP87Coefs(B3UranusCoefficients,sizeof(B3UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            b4 = sumVSOP87Coefs(B4UranusCoefficients,sizeof(B4UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0UranusCoefficients,sizeof(R0UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1UranusCoefficients,sizeof(R1UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2UranusCoefficients,sizeof(R2UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3UranusCoefficients,sizeof(R3UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            r4 = sumVSOP87Coefs(R4UranusCoefficients,sizeof(R4UranusCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
            
        case Neptune:
            
            l0 = sumVSOP87Coefs(L0NeptuneCoefficients,sizeof(L0NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            l1 = sumVSOP87Coefs(L1NeptuneCoefficients,sizeof(L1NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            l2 = sumVSOP87Coefs(L2NeptuneCoefficients,sizeof(L2NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            l3 = sumVSOP87Coefs(L3NeptuneCoefficients,sizeof(L3NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            l4 = sumVSOP87Coefs(L4NeptuneCoefficients,sizeof(L4NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            
            b0 = sumVSOP87Coefs(B0NeptuneCoefficients,sizeof(B0NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            b1 = sumVSOP87Coefs(B1NeptuneCoefficients,sizeof(B1NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            b2 = sumVSOP87Coefs(B2NeptuneCoefficients,sizeof(B2NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            b3 = sumVSOP87Coefs(B3NeptuneCoefficients,sizeof(B3NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            b4 = sumVSOP87Coefs(B4NeptuneCoefficients,sizeof(B4NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            
            r0 = sumVSOP87Coefs(R0NeptuneCoefficients,sizeof(R0NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            r1 = sumVSOP87Coefs(R1NeptuneCoefficients,sizeof(R1NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            r2 = sumVSOP87Coefs(R2NeptuneCoefficients,sizeof(R2NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            r3 = sumVSOP87Coefs(R3NeptuneCoefficients,sizeof(R3NeptuneCoefficients)/SizeOfVSOP87Coefficient,T);
            
            break;
            
        default:
            // Do not work for Moon...
            l0 = NAN;
            l1 = NAN;
            l2 = NAN;
            l3 = NAN;
            l4 = NAN;
            l5 = NAN;
            
            b0 = NAN;
            b1 = NAN;
            b2 = NAN;
            b3 = NAN;
            b4 = NAN;
            b5 = NAN;
            
            r0 = NAN;
            r1 = NAN;
            r2 = NAN;
            r3 = NAN;
            r4 = NAN;
            r5 = NAN;
            
            break;
    }
    
    // L
    coordinates.lon  = (l0 + l1*T + l2*TSquared + l3*TCubed + l4*T4 + l5*T5)/100000000.0;
    coordinates.lon  = RADIANS_TO_DEGREES(coordinates.lon);
    coordinates.lon  = LIMIT_DEGREES_TO_360(coordinates.lon);
    
    // B
    coordinates.lat  = (b0 + b1*T + b2*TSquared + b3*TCubed + b4*T4 + b5*T5)/100000000.0;
    coordinates.lat  = RADIANS_TO_DEGREES(coordinates.lat);
    
    // R
    coordinates.radius = (r0 + r1*T + r2*TSquared + r3*TCubed + r4*T4 + r5*T5)/100000000.0;
    
    return coordinates;
}
