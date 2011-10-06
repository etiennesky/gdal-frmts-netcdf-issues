GDAL GeoTiff -> NetCDF Testing results
======================================

(Using Pat's version of GDAL-NetCDF drivers, with refactored export)

Albers Equal Area: OK
------------------------

*CF-1 compliance?:* YES
*Loads in IDV?*: YES

(Works when we set 'longitude_of_center' -> 'longitude_of_central_meridian')

Test EPSG: 3577 ("GDA94 / Australian Albers")

======================================  =============================================
GDAL:                                   CF-1 Equivalents:
======================================  =============================================
PROJECTION["Albers_Conic_Equal_Area"],  grid_mapping_name = albers_conical_equal_area
PARAMETER["standard_parallel_1",-18],   standard_parallel=-18, -36  
PARAMETER["standard_parallel_2",-36],   
PARAMETER["latitude_of_center",0],      latitude_of_projection_origin=0
PARAMETER["longitude_of_center",132],   longitude_of_central_meridian=132
======================================  =============================================

Azimuthal equidistant: Partial
------------------------------

*CF-1 compliance?:* YES
*Loads in IDV?*: NO (says it doesn't contain gridded data)

Notes:
 - (Problem loading into IDV? At least with params below)
 - (Problem loading into IDV? - complains its 'not gridded data'?)

TODO:- maybe test further in NetCDF Java.

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Azimuthal_Equidistant"],        grid_mapping_name=azimuthal_equidistant
PARAMETER["latitude_of_center",0],          latitude_of_projection_origin=0
PARAMETER["longitude_of_center",134],       longitude_of_projection_origin=134
=========================================== ===============================================


Lambert azimuthal equal area: OK
--------------------------------

*CF-1 compliance?:* YES
*Loads in IDV?*: YES (Reprojection very sensitive to input params)

(Works when we set 'longitude_of_center' -> 'longitude_of_projection_origin')

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Lambert_Azimuthal_Equal_Area"], grid_mapping_name=lambert_azimuthal_equal_area
PARAMETER["latitude_of_center",-37],        latitude_of_projection_origin=-37
PARAMETER["longitude_of_center",145],       longitude_of_projection_origin=145
=========================================== ===============================================

Lambert conformal - 2SP: OK
---------------------------

(Note: need variants with both 1SP or 2SP?)

*CF-1 compliance?:* YES
*Loads in IDV?*: YES

Test EPSG: 3112 ("GDA94 / Geoscience Australia Lambert")

==========================================  =============================================
GDAL:                                       CF-1 Equivalents
==========================================  =============================================
PROJECTION["Lambert_Conformal_Conic_2SP"],  grid_mapping_name=lambert_conformal_conic
PARAMETER["standard_parallel_1",-18],       standard_parallel=-18, -36
PARAMETER["standard_parallel_2",-36],
PARAMETER["latitude_of_origin",0],          latitude_of_projection_origin=0
PARAMETER["central_meridian",134],          longitude_of_central_meridian=134
==========================================  =============================================

Lambert Cylindrical Equal Area: Partial
---------------------------------------

*CF-1 compliance?:* YES
*Loads in IDV?*: NO (Claims its not gridded data)

(works - note also has a 'scale_factor_at_projection_origin' alternative to 'standard_parallel')

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Cylindrical_Equal_Area"],       
PARAMETER["standard_parallel_1",-37],       standard_parallel=-37
PARAMETER["central_meridian",145],          longitude_of_central_meridian=145
=========================================== ===============================================

Mercator-1SP: Partial
---------------------

*CF-1 compliance?:* YES
*Loads in IDV?*: NO - Loads, but doesn't display properly (won't locate on grid)

Note:
 - Needs 'central_meridian' -> 'longitude_of_projection_origin'
 - Needs 'scale_factor' -> 'scale_factor_at_projection_origin'

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Mercator_1SP"],                 grid_mapping_name = mercator
PARAMETER["central_meridian",145],          longitude_of_projection_origin
PARAMETER["scale_factor",1],                scale_factor_at_projection_origin
=========================================== ===============================================

Mercator-2SP: GDAL Prob
-----------------------

Not added as yet :- seems to be problems as GDAL refuses to create Mercator-2SP files ??

TODO: perhaps file a bug

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Mercator_2SP"],                 grid_mapping_name = mercator
PARAMETER["central_meridian",150],          longitude_of_projection_origin
PARAMETER["standard_parallel_1",-45],       standard_parallel (? 2 values, to cover lat of origin? Or force 1 of the SP to be at equator, as in Proj4?)  
PARAMETER["latitude_of_origin",0],          
=========================================== ===============================================

Orthographic: OK
----------------

*CF-1 compliance?:* YES
*Loads in IDV?*: YES 

Need 'central_meridian' -> 'longitude_of_projection_origin'?

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Orthographic"],                 grid_mapping_name=orthographic
PARAMETER["latitude_of_origin",-37],        latitude_of_projection_origin=-37
PARAMETER["central_meridian",145],          longitude_of_projection_origin=145
=========================================== ===============================================

Polar stereographic: No
-----------------------

(Haven't tried to match parameters yet, need to get samples/examples from CF-1 people).

Issues:
 - need "scale_factor" -> 'scale_factor_at_projection_origin'
 - think need "central_meridian" -> 'straight_vertical_longitude_from_pole'
 - not sure if need "latitude_of_origin" -> latitude_of_projection_origin? (see below)

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Polar_Stereographic"],          
PARAMETER["latitude_of_origin",0],          (?) - in proj4/WKT, this is extra to +90/-90
PARAMETER["central_meridian",134],          straight_vertical_longitude_from_pole (?)
PARAMETER["scale_factor",1],                scale_factor_at_projection_origin
                                            latitude_of_projection_origin (?)
=========================================== ===============================================

for South Pole

Sample WKT::

    PROJCS["WGS 84 / Antarctic Polar Stereographic",
        GEOGCS["WGS 84",
            DATUM["WGS_1984",
                SPHEROID["WGS 84",6378137,298.257223563,
                    AUTHORITY["EPSG","7030"]],
                AUTHORITY["EPSG","6326"]],
            PRIMEM["Greenwich",0,
                AUTHORITY["EPSG","8901"]],
            UNIT["degree",0.01745329251994328,
                AUTHORITY["EPSG","9122"]],
            AUTHORITY["EPSG","4326"]],
        UNIT["metre",1,
            AUTHORITY["EPSG","9001"]],
        PROJECTION["Polar_Stereographic"],
        PARAMETER["latitude_of_origin",-71],
        PARAMETER["central_meridian",0],
        PARAMETER["scale_factor",1],
        PARAMETER["false_easting",0],
        PARAMETER["false_northing",0],
        AUTHORITY["EPSG","3031"],
        AXIS["Easting",UNKNOWN],
        AXIS["Northing",UNKNOWN]]

Proj4 String::
    +proj=stere +lat_0=-90 +lat_ts=-71 +lon_0=0 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs 

North Pole:

Sample OGC WKT::

    PROJCS["WGS 84 / NSIDC Sea Ice Polar Stereographic North",
        GEOGCS["WGS 84",
            DATUM["WGS_1984",
                SPHEROID["WGS 84",6378137,298.257223563,
                    AUTHORITY["EPSG","7030"]],
                AUTHORITY["EPSG","6326"]],
            PRIMEM["Greenwich",0,
                AUTHORITY["EPSG","8901"]],
            UNIT["degree",0.01745329251994328,
                AUTHORITY["EPSG","9122"]],
            AUTHORITY["EPSG","4326"]],
        UNIT["metre",1,
            AUTHORITY["EPSG","9001"]],
        PROJECTION["Polar_Stereographic"],
        PARAMETER["latitude_of_origin",70],
        PARAMETER["central_meridian",-45],
        PARAMETER["scale_factor",1],
        PARAMETER["false_easting",0],
        PARAMETER["false_northing",0],
        AUTHORITY["EPSG","3413"],
        AXIS["X",UNKNOWN],
        AXIS["Y",UNKNOWN]]

Proj4 string::

    +proj=stere +lat_0=90 +lat_ts=70 +lon_0=-45 +k=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs     

Rotated pole: No GDAL Equivalent?
---------------------------------

*CF-1 compliance?:* 
*Loads in IDV?*: 

Not sure if there's a GDAL Equivalent of this?

Stereographic: Partial
----------------------

Note: GDAL seems to create an Ortho-stereographic, when we specified proj4 
string asking for regular stereographic.

*CF-1 compliance?:* YES
*Loads in IDV?*: Loads, but details don't show properly. Possibly because of GDAL Ortho_Stereographic compared to stereographic test.

Notes:
 - need "central_meridian" -> 'longitude_of_projection_origin'
 - need 'scale_factor' -> 'scale_factor_at_projection_origin'

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Oblique_Stereographic"],        grid_mapping_name = stereographic
PARAMETER["latitude_of_origin",-37],        latitude_of_projection_origin
PARAMETER["central_meridian",134],          longitude_of_projection_origin
PARAMETER["scale_factor",1],                scale_factor_at_projection_origin
=========================================== ===============================================

Transverse Mercator: OK
-----------------------

*CF-1 compliance?:* YES
*Loads in IDV?*: YES

=========================================== ===============================================
GDAL:                                       CF-1 Equivalents
=========================================== ===============================================
PROJECTION["Transverse_Mercator"],          grid_mapping_name=transverse_mercator
PARAMETER["latitude_of_origin",0],          latitude_of_projection_origin=0
PARAMETER["central_meridian",147],          longitude_of_central_meridian=147
=========================================== ===============================================
