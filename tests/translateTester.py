#!/usr/bin/env python

import os, sys, subprocess
from subprocess import PIPE
import datetime

#Force GDAL tags to be written to make testing easier, with preserved datum etc
ncCoOpts = "-co WRITEGDALTAGS=yes"

# Tuple structure:
#  0: Short code (eg AEA)
#  1: official name from CF-1 conventions
#  2: EPSG code, or WKT, to tell GDAL to do reprojection
#  3: Actual attribute official name of grid mapping
#  4: List of required attributes to define projection
#  5: List of required coordinate variable standard name attributes

PROJ_DEF_TUPLES = [
    ("AEA", "Albers Equal Area", "EPSG:3577", "albers_conical_equal_area",
        ['standard_parallel', 'longitude_of_central_meridian',
         'latitude_of_projection_origin', 'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate']),
    ("AZE", "Azimuthal Equidistant",
        #Didn't have EPSG suitable for AU
        "+proj=aeqd +lat_0=-37 +lon_0=145 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        "azimuthal_equidistant",
        ['longitude_of_projection_origin',
         'latitude_of_projection_origin', 'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate']),
    ("LAZEA", "Lambert azimuthal equal area",
        #Specify proj4 since no approp LAZEA for AU
        #"+proj=laea +lat_0=0 +lon_0=134 +x_0=0 +y_0=0 +ellps=GRS80 +towgs84=0,0,0,0,0,0,0 +units=m +no_defs",
        "+proj=laea +lat_0=-37 +lon_0=145 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        "lambert_azimuthal_equal_area",
        ['longitude_of_projection_origin',
         'latitude_of_projection_origin', 'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate']),
    ("LC_2SP", "Lambert conformal", "EPSG:3112", "lambert_conformal_conic",
        ['standard_parallel',
         'longitude_of_central_meridian',
         'latitude_of_projection_origin', 'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate']),
    # TODO: Test LCC with 1SP
    ("LCEA", "Lambert Cylindrical Equal Area",
        "+proj=cea +lat_ts=-37 +lon_0=145 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        "lambert_cylindrical_equal_area",
        ['longitude_of_central_meridian',
         'standard_parallel', # TODO: OR 'scale_factor_at_projection_origin' 
         'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate']),
    # 2 entries for Mercator, since attribs different for 1SP or 2SP
    ("M-1SP", "Mercator",
        "+proj=merc +lon_0=145 +k_0=1 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        
        "mercator",
        ['longitude_of_projection_origin',
         'scale_factor_at_projection_origin',
         'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate']),
    # Commented out as it seems GDAL itself's support of Mercator with 2SP
    #  is a bit dodgy
    ("M-2SP", "Mercator",
        #"ESPG:3388", # As suggested by http://trac.osgeo.org/gdal/ticket/2744
        #GDAL Doesn't seem to recognise proj4 string properly for 2SP
        "+proj=merc +lat_ts=-45 +lon_0=145 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        # Trying with full WKT:
        #"""PROJCS["unnamed", GEOGCS["WGS 84", DATUM["WGS_1984", SPHEROID["WGS 84",6378137,298.257223563, AUTHORITY["EPSG","7030"]], AUTHORITY["EPSG","6326"]], PRIMEM["Greenwich",0], UNIT["degree",0.0174532925199433], AUTHORITY["EPSG","4326"]], PROJECTION["Mercator_2SP"], PARAMETER["central_meridian",146], PARAMETER["standard_parallel_1",-45], PARAMETER["latitude_of_origin",0], PARAMETER["false_easting",0], PARAMETER["false_northing",0], UNIT["metre",1, AUTHORITY["EPSG","9001"]]]""",
        "mercator",
        ['longitude_of_projection_origin',
         'standard_parallel',   #2 values?
         'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate']),
    ("Ortho", "Orthographic",
        "+proj=ortho +lat_0=-37 +lon_0=145 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        "orthographic",
        ['longitude_of_projection_origin',
         'latitude_of_projection_origin',
         'false_easting', 'false_northing'],
         ['projection_x_coordinate', 'projection_y_coordinate']),
    # Seems GDAL may have problems with Polar stereographic, as it 
    #  considers these "local coordinate systems"
    ("PSt", "Polar stereographic",
        "+proj=stere +lat_ts=-37 +lat_0=-90 +lon_0=145 +k_0=1.0 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        "polar_stereographic",
        ['straight_vertical_longitude_from_pole',
        'latitude_of_projection_origin',
         'scale_factor_at_projection_origin',
         'false_easting', 'false_northing'],
         ['projection_x_coordinate', 'projection_y_coordinate']),
    ("St", "Stereographic",
        #"+proj=stere +lat_0=-37 +lon_0=145 +x_0=0 +y_0=0 +ellps=WGS84 +datum=WGS84 +units=m +no_defs",
        'PROJCS["unnamed", GEOGCS["WGS 84", DATUM["WGS_1984", SPHEROID["WGS 84",6378137,298.257223563, AUTHORITY["EPSG","7030"]], AUTHORITY["EPSG","6326"]], PRIMEM["Greenwich",0], UNIT["degree",0.0174532925199433], AUTHORITY["EPSG","4326"]], PROJECTION["Stereographic"], PARAMETER["latitude_of_origin",-37.5], PARAMETER["central_meridian",145], PARAMETER["scale_factor",1], PARAMETER["false_easting",0], PARAMETER["false_northing",0], UNIT["metre",1, AUTHORITY["EPSG","9001"]]]',
        "stereographic",
        ['longitude_of_projection_origin',
        'latitude_of_projection_origin',
         'scale_factor_at_projection_origin',
         'false_easting', 'false_northing'],
         ['projection_x_coordinate', 'projection_y_coordinate']),
    #Note: Rotated Pole not in this list, as seems not GDAL-supported
    ("TM", "Transverse Mercator", "EPSG:32655", #UTM Zone 55N
        "transverse_mercator",
        [
         'scale_factor_at_central_meridian',
        'longitude_of_central_meridian',
        'latitude_of_projection_origin',
         'false_easting', 'false_northing'],
         ['projection_x_coordinate','projection_y_coordinate'])
    ]

# Note: rotated_pole projection seems not currently supported by GDAL, thus
#  not in the list above.
#  See http://www.osgeo.org/pipermail/gdal-dev/2007-June/013282.html
#  An effort to _import_ from RP in a NetCDF CF-1 compliant file also seems
#  to have failed: http://osgeo-org.1803224.n2.nabble.com/Re-rotated-pole-ob-tran-help-needed-td5149504.html

def testGeoTiffToNetCdf(projTuples, origTiff, outPath, resFilename):
    """Test a Geotiff file can be converted to NetCDF, and projection in 
    CF-1 conventions can be successfully maintained. Save results to file.
    
    :arg: projTuples - list of tuples
    :arg: outPath - path to save output
    :arg: resFilename - results filename to write to.

    """
    if not os.path.exists(outPath):
        os.makedirs(outPath)
    resFile = open(os.path.join(outPath, resFilename), "w")

    if not os.path.exists(outPath):
        os.makedirs(outPath)

    heading = "Testing GDAL translation results to NetCDF\n"
    resFile.write(heading)
    resFile.write(len(heading)*"="+"\n")

    now = datetime.datetime.now()
    resFile.write("*Date/time:* %s\n" % (now.strftime("%Y-%m-%d %H:%M")))
    p = subprocess.Popen("which gdal_translate", shell=True, stdout=subprocess.PIPE, 
        stderr=subprocess.STDOUT)
    resFile.write("*GDAL ver used path:* %s\n" % (p.communicate()[0]))
    resFile.write("\n")

    resPerProj = {}

    for proj in projTuples:
        # Our little results data structures

        print "Testing %s (%s) translation:" % (proj[0], proj[1])

        print "About to create GeoTiff in chosen SRS"
        projTiff = os.path.join(outPath, "%s_%s.tif" % \
            (origTiff.rstrip('.tif'), proj[0] ))
        projOpts = "-t_srs '%s'" % (proj[2])
        cmd = " ".join(['gdalwarp', projOpts, origTiff, projTiff])
        print cmd
        p = subprocess.Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
        p.communicate()
        print "Warped %s to %s" % (proj[0], projTiff)

        projNc = os.path.join(outPath, "%s_%s.nc" % \
            (origTiff.rstrip('.tif'), proj[0] ))
        cmd = " ".join(['gdal_translate', "-of netCDF", ncCoOpts, projTiff,
            projNc])
        print "About to translate to NetCDF"
        print cmd
        p = subprocess.Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
        p.communicate()
        print "Translated to %s" % (projNc)
        
        projNcDump = "%s.ncdump" % projNc
        cmd = " ".join(['ncdump -h', projNc, ">", projNcDump])
        p = subprocess.Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
        p.communicate()

        transWorked, resDetails = testNetcdfValidCF1(proj, projNc, projNcDump)
        resPerProj[proj[0]] = resDetails

        resFile.write("%s (%s): " % (proj[0], proj[1]))
        if transWorked:
            resFile.write("OK\n")
        else:
            resFile.write("BAD\n")
            if 'missingProjName' in resPerProj[proj[0]]:
                resFile.write("\tMissing proj name '%s'\n" % \
                    (resPerProj[proj[0]]['missingProjName']))
            for attrib in resPerProj[proj[0]]['missingAttrs']:
                resFile.write("\tMissing attrib '%s'\n" % (attrib))
            for cVarStdName in resPerProj[proj[0]]['missingCoordVarStdNames']:
                resFile.write("\tMissing coord var with std name '%s'\n" \
                    % (cVarStdName))

    resFile.close()
    print "\n" + "*" * 80
    print "Saved results to file %s" % (os.path.join(outPath, resFilename))


def testNetcdfValidCF1(proj, projNc, dumpFile):
    """"Test an NC file has valid conventions according to passed-in proj tuple.
    
    Note: current testing strategy is a fairly simple attribute search.
    
    """
    transWorked = True

    dumpFile = open(dumpFile, "r")
    dumpStr = dumpFile.read()
    
    resDetails = {}
    resDetails['missingAttrs'] = []
    resDetails['missingCoordVarStdNames'] = []
    if (':grid_mapping_name = "%s"' % (proj[3])) not in dumpStr:
        transWorked = False
        resDetails['missingProjName'] = proj[3]
    # Check attributes in the projection are included
    for attrib in proj[4]:
        # The ':' prefix and ' ' suffix is to help check for exact name,
        # eg to catch the standard_parallel_1 and 2 issue.
        if (":"+attrib+" ") not in dumpStr:
            transWorked = False
            resDetails['missingAttrs'].append(attrib)
            print "**Error for proj '%s': CF-1 attrib '%s' not found.**" % \
                (proj[0], attrib)
    # Now we check the required X and Y attributes are included (e.g. Rotated Pole
    #  has special names required here.
    for coordVarStdName in proj[5]:
        if coordVarStdName not in dumpStr:
            transWorked = False
            resDetails['missingCoordVarStdNames'].append(coordVarStdName)
    return transWorked, resDetails

if __name__ == "__main__":
    #origTiff = "CONUS.week01.2010.h10v10.v1.5.Band1_TOA_REF_small.tif"
    #outPath = os.path.join(".", "out")
    #origTiff = "melb-small.tif"
    if len(sys.argv) != 3:
        print "Usage: %s origTiff outPath" % (sys.argv[0])
        print "E.g. %s melb.tif savedResults/Melb" % (sys.argv[0])
        sys.exit(1)
    origTiff = sys.argv[1]
    outPath = sys.argv[2]
    resFilename = "translate_results.txt"
    testGeoTiffToNetCdf(PROJ_DEF_TUPLES, origTiff, outPath, resFilename)
