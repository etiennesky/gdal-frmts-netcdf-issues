Refactor notes about GDAL NetCDF driver - PDS
=============================================

(Refactor notes as a reminder as work progresses.)

Layout:
-------

 * PDS: Should details of CF-1 conventions be moved to their own header/C file
    for clarity?
   * This would support future efforts to add back-compat for CF-1 conventions
     less than CF-1.5 ...

Projection: major refactors 
---------------------------

 * Should I try to refactor the SetProjection to also use the mapped arrays?

 * Should we look at the C LibCF for working with NetCDF files, or is that
   an unwanted extra dependency?
   (Well, for Geotiff etc they appear to distribute a version of libtiff 
   with GDAL).

Projection issues-specific:
---------------------------

 * Is there a good way to better connect the CF-1 standard name of 
   projections, with its required grid attributes?
 * Note: in NCDFWriteProjAttribs(), this can be cleaned up considerably 
   if/when we remove the 
 * Current export to NetCDF behaviour implied by poNetcdfSRS in Dataset.h
   is to export all projections, even if CF-1 doesn't explicitly support?
   * Pro: _if_ we also exported lat/lon, you could probably still view the
     data as some sort of grid in NetCDF, even if real proj info didn't work
   * Con: its currently giving a false impression that the projection 
      export is supported, when it isn't really.
   * Suggested: we should probably keep the ability to read WKT as a 
      'fallback' IFF no relevant grid_mapping tags found.
      (And perhaps check they match if both are present.)

 * Polar stereo: requires some remapping / manual manipulation?
 * Mercator: 1SP
 
General TODOs
-------------

 * Update test to:
   * Access some point locations in gdal with both
   * Use a CF-1 compliance checker optionally?
   * Import back in to TIFF again, check file still same projection, extents,
     and several locations are still valid.
   * Open the files in NetCDF Java, and test they are gridded files?

 * GDAL Followups
   
   * (O) Mercator - 1SP (Sent to mailing list ... )
   * (O) Stereographic and Ortho_Stereographic (Reported as GDAL:#4267)

 * CF-1 followups:
   (TODO: Waiting on access to CF-1 trac to be able to report these ... )
   
   * Polar stereographic - ask for guidance on what the coords mean.
   * Lambert Cylindrical equal area ... the 'scale_factor_at_projection' not
     covered by OGC WKT or Proj4?
 
 * IDV/NetCDF Java/CF-1 followups:
   (O: Sent email to netcdf-java@unidata.ucar.edu to ask for input)

   * AZ equidistant not loading
   * LCEA not loading
   * Mercator 1SP ... loads in IDV, but then doesn't display properly (no valid projection)
