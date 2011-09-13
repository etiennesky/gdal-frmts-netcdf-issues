/************************************************************************/
/*                             CreateCopy()                             */
/************************************************************************/

static GDALDataset*
NCDFCreateCopy( const char * pszFilename, GDALDataset *poSrcDS, 
                int bStrict, char ** papszOptions, 
                GDALProgressFunc pfnProgress, void * pProgressData )

{
    int  nBands = poSrcDS->GetRasterCount();
    int  nXSize = poSrcDS->GetRasterXSize();
    int  nYSize = poSrcDS->GetRasterYSize();
    int  bProgressive = FALSE;
    int  iBand;

    int  anBandDims[ NC_MAX_DIMS ];
    int  anBandMap[  NC_MAX_DIMS ];

    int    bWriteGeoTransform = FALSE;
    char  pszNetcdfProjection[ NC_MAX_NAME ];

    char   pszXDimName[ MAX_STR_LEN ];
    char   pszYDimName[ MAX_STR_LEN ];

    if (nBands == 0)
    {
        CPLError( CE_Failure, CPLE_NotSupported, 
                  "NetCDF driver does not support source dataset with zero band.\n");
        return NULL;
    }

    for( iBand=1; iBand <= nBands; iBand++ )
    {
        GDALRasterBand *poSrcBand = poSrcDS->GetRasterBand( iBand );
        GDALDataType eDT = poSrcBand->GetRasterDataType();
        if (eDT == GDT_Unknown || GDALDataTypeIsComplex(eDT))
        {
            CPLError( CE_Failure, CPLE_NotSupported, 
                      "NetCDF driver does not support source dataset with band of complex type.");
            return NULL;
        }
    }

    if( !pfnProgress( 0.0, NULL, pProgressData ) )
        return NULL;


    bProgressive = CSLFetchBoolean( papszOptions, "PROGRESSIVE", FALSE );

/* -------------------------------------------------------------------- */
/*      Get Projection ref for netCDF data CF-1 Convention              */
/* -------------------------------------------------------------------- */

    OGRSpatialReference oSRS;
    char *pszWKT = (char *) poSrcDS->GetProjectionRef();

    if( pszWKT != NULL )
        oSRS.importFromWkt( &pszWKT );

/* -------------------------------------------------------------------- */
/*      Create the dataset.                                             */
/* -------------------------------------------------------------------- */

    int fpImage;
    int status;
    int nXDimID = 0;
    int nYDimID = 0;
    GDALDataType eDT;
    int bBottomUp = FALSE;

    status = nc_create( pszFilename, NC_CLOBBER,  &fpImage );

    if( status != NC_NOERR )
    {
        CPLError( CE_Failure, CPLE_OpenFailed, 
                  "Unable to create netCDF file %s.\n", 
                  pszFilename );
        return NULL;
    }

    if( ! oSRS.IsProjected() ) /* If not Projected assume geographic */
    {
	strcpy( pszXDimName, "lon" );
	strcpy( pszYDimName, "lat" );
    }
    else 
    {
	strcpy( pszXDimName, "x" ); 
	strcpy( pszYDimName, "y" );
    }

    status = nc_def_dim( fpImage, pszXDimName, nXSize, &nXDimID );
    CPLDebug( "GDAL_netCDF", "status nc_def_dim %s = %d\n", pszXDimName, status );
    
    status = nc_def_dim( fpImage, pszYDimName, nYSize, &nYDimID );
    CPLDebug( "GDAL_netCDF", "status nc_def_dim %s = %d\n", pszYDimName, status );
    
    CPLDebug( "GDAL_netCDF", "nYDimID = %d\n", nXDimID );
    CPLDebug( "GDAL_netCDF", "nXDimID = %d\n", nYDimID );
    CPLDebug( "GDAL_netCDF", "nXSize = %d\n", nXSize );
    CPLDebug( "GDAL_netCDF", "nYSize = %d\n", nYSize );
    
    CopyMetadata((void *) poSrcDS, fpImage, NC_GLOBAL );

    // if( oSRS.IsGeographic() ) {
    /* If not Projected assume Geographic to catch grids without Datum */
    if( ! oSRS.IsProjected() ) { 

        int status;
        int i;
        int NCDFVarID=0;
        
        double dfNN, dfSN=0.0, dfEE=0.0, dfWE=0.0;
        double adfGeoTransform[6];
        char   szGeoTransform[ MAX_STR_LEN ];
        char   szTemp[ MAX_STR_LEN ];
        
        double dfX0=0.0, dfDX=0.0, dfY0=0.0, dfDY=0.0;
        double dfTemp=0.0;
        double *pafLonLat  = NULL;
        int    anLatLonDims[1];
        size_t startLonLat[1];
        size_t countLonLat[1];
	
        /* netcdf standard is bottom-up */
        bBottomUp = TRUE;

/* -------------------------------------------------------------------- */
/*      Copy GeoTransform array from source                             */
/* -------------------------------------------------------------------- */
        poSrcDS->GetGeoTransform( adfGeoTransform );
        /* files without a Datum will not have Geographics attributes written */
        if ( oSRS.IsGeographic() ) 
            bWriteGeoTransform = TRUE;
        else
            bWriteGeoTransform = FALSE;

        *szGeoTransform = '\0';
        for( i=0; i<6; i++ ) {
            sprintf( szTemp, "%.16g ",
                     adfGeoTransform[i] );
            strcat( szGeoTransform, szTemp );
        }
        CPLDebug( "GDAL_netCDF", "szGeoTranform = %s", szGeoTransform );

        dfNN = adfGeoTransform[3];
        dfSN = ( adfGeoTransform[5] * nYSize ) + dfNN;
        dfWE = adfGeoTransform[0];
        dfEE = ( adfGeoTransform[1] * nXSize ) + dfWE;

/* -------------------------------------------------------------------- */
/*      Write CF-1.x compliant Geographics attributes                   */
/*      Note: WKT information will not be preserved (e.g. WGS84)        */
/*      Don't write custom GDAL values any more as they are not CF-1.x  */
/*      compliant, this is still under discussion                       */
/* -------------------------------------------------------------------- */
        
        if( bWriteGeoTransform == TRUE ) 
 	    {

            strcpy( pszNetcdfProjection, "crs" );
            nc_def_var( fpImage, 
                        pszNetcdfProjection, 
                        NC_CHAR, 
                        0, NULL, &NCDFVarID );
            nc_put_att_text( fpImage, 
                             NCDFVarID, 
                             GRD_MAPPING_NAME,
                             strlen("latitude_longitude"),
                             "latitude_longitude" );
            dfTemp = oSRS.GetPrimeMeridian();
            nc_put_att_double( fpImage,
                               NCDFVarID, 
                               "longitude_of_prime_meridian",
                               NC_DOUBLE,
                               1,
                               &dfTemp );
            dfTemp = oSRS.GetSemiMajor();
            nc_put_att_double( fpImage,
                               NCDFVarID, 
                               "semi_major_axis",
                               NC_DOUBLE,
                               1,
                               &dfTemp );
            dfTemp = oSRS.GetInvFlattening();
            nc_put_att_double( fpImage,
                               NCDFVarID, 
                               "inverse_flattening",
                               NC_DOUBLE,
                               1,
                               &dfTemp );
            // pszWKT = (char *) poSrcDS->GetProjectionRef();
            // nc_put_att_text( fpImage, 
            //                  NCDFVarID, 
            //                  "spatial_ref",
            //                  strlen( pszWKT ),
            //                  pszWKT );
            // nc_put_att_text( fpImage, 
            //                  NCDFVarID, 
            //                  "GeoTransform",
            //                  strlen( szGeoTransform ),
            //                  szGeoTransform );
        }

/* -------------------------------------------------------------------- */
/*      Write latitude attributes                                       */
/* -------------------------------------------------------------------- */
        anLatLonDims[0] = nYDimID;
        status = nc_def_var( fpImage, "lat", NC_DOUBLE, 1, anLatLonDims, &NCDFVarID );
        nc_put_att_text( fpImage,
                         NCDFVarID,
                         "standard_name",
                         8,
                         "latitude" );
        nc_put_att_text( fpImage,
                         NCDFVarID,
                         "long_name",
                         8,
                         "latitude" );
        nc_put_att_text( fpImage,
                         NCDFVarID,
                         "units",
                         13,
                         "degrees_north" );

/* -------------------------------------------------------------------- */
/*      Write latitude values                                           */
/* -------------------------------------------------------------------- */
        if ( ! bBottomUp )
            dfY0 = adfGeoTransform[3];
        else /* invert latitude values */ 
            dfY0 = adfGeoTransform[3] + ( adfGeoTransform[5] * nYSize );
        dfDY = adfGeoTransform[5];
        
        pafLonLat = (double *) CPLMalloc( nYSize * sizeof( double ) );
        for( i=0; i<nYSize; i++ ) {
            /* The data point is centered inside the pixel */
            if ( ! bBottomUp )
                pafLonLat[i] = dfY0 + (i+0.5)*dfDY ;
            else /* invert latitude values */ 
                pafLonLat[i] = dfY0 - (i+0.5)*dfDY ;
        }
        
        startLonLat[0] = 0;
        countLonLat[0] = nYSize;
        
        /* Temporarily switch to data mode and write data */
        status = nc_enddef( fpImage );
        status = nc_put_vara_double( fpImage, NCDFVarID, startLonLat,
                                     countLonLat, pafLonLat);
        status = nc_redef( fpImage );
        
        
/* -------------------------------------------------------------------- */
/*      Write longitude attributes                                      */
/* -------------------------------------------------------------------- */
        anLatLonDims[0] = nXDimID;
        status = nc_def_var( fpImage, "lon", NC_DOUBLE,
                             1, anLatLonDims, &NCDFVarID );
        nc_put_att_text( fpImage,
                         NCDFVarID,
                         "standard_name",
                         9,
                         "longitude" );
        nc_put_att_text( fpImage,
                         NCDFVarID,
                         "long_name",
                         9,
                         "longitude" );
        nc_put_att_text( fpImage,
                         NCDFVarID,
                         "units",
                         12,
                         "degrees_east" );
        
/* -------------------------------------------------------------------- */
/*      Write longitude values                                          */	
/* -------------------------------------------------------------------- */
        dfX0 = adfGeoTransform[0];
        dfDX = adfGeoTransform[1];
        
        pafLonLat = (double *) CPLRealloc( pafLonLat, nXSize * sizeof( double ) );
        for( i=0; i<nXSize; i++ ) {
            /* The data point is centered inside the pixel */
            pafLonLat[i] = dfX0 + (i+0.5)*dfDX ;
        }
        
        startLonLat[0] = 0;
        countLonLat[0] = nXSize;
        
        /* Temporarily switch to data mode and write data */
        status = nc_enddef( fpImage );
        status = nc_put_vara_double( fpImage, NCDFVarID, startLonLat,
                                     countLonLat, pafLonLat);
        status = nc_redef( fpImage );
        
        /* free lonlat values */
        CPLFree( pafLonLat );
    }
    
    //if( oSRS.IsProjected() )
    else /* Projected */ 
    {
        const char *pszParamStr, *pszParamVal;
        const OGR_SRSNode *poPROJCS = oSRS.GetAttrNode( "PROJCS" );
        int status;
        int i;
        int NCDFVarID;
        const char  *pszProjection;
        double dfNN=0.0;
        double dfSN=0.0;
        double dfEE=0.0;
        double dfWE=0.0;
        double adfGeoTransform[6];
        char   szGeoTransform[ MAX_STR_LEN ];
        char   szTemp[ MAX_STR_LEN ];

        /* netcdf standard is bottom-up, but leave it top first for now */
        bBottomUp = FALSE;

        poSrcDS->GetGeoTransform( adfGeoTransform );
        
        *szGeoTransform = '\0';
        for( i=0; i<6; i++ ) {
            sprintf( szTemp, "%.16g ",
                     adfGeoTransform[i] );
            strcat( szGeoTransform, szTemp );
        }

        CPLDebug( "GDAL_netCDF", "szGeoTranform = %s", szGeoTransform );

        pszProjection = oSRS.GetAttrValue( "PROJECTION" );
        bWriteGeoTransform = TRUE;

        for(i=0; poNetcdfSRS[i].netCDFSRS != NULL; i++ ) {
            if( EQUAL( poNetcdfSRS[i].SRS, pszProjection ) ) {
                CPLDebug( "GDAL_netCDF", "PROJECTION = %s", 
                          poNetcdfSRS[i].netCDFSRS);
                strcpy( pszNetcdfProjection, poNetcdfSRS[i].netCDFSRS );

                break;
            }
        }
        
        status = nc_def_var( fpImage, 
                             poNetcdfSRS[i].netCDFSRS, 
                             NC_CHAR, 
                             0, NULL, &NCDFVarID );
        
        dfNN = adfGeoTransform[3];
        dfSN = ( adfGeoTransform[5] * nYSize ) + dfNN;
        dfWE = adfGeoTransform[0];
        dfEE = ( adfGeoTransform[1] * nXSize ) + dfWE;
        
        status = nc_put_att_double( fpImage,
                                    NCDFVarID, 
                                    "Northernmost_Northing",
                                    NC_DOUBLE,
                                    1,
                                    &dfNN );
        status = nc_put_att_double( fpImage,
                                    NCDFVarID, 
                                    "Southernmost_Northing",
                                    NC_DOUBLE,
                                    1,
                                    &dfSN );
        status = nc_put_att_double( fpImage,
                                    NCDFVarID,
                                    "Easternmost_Easting",
                                    NC_DOUBLE,
                                    1,
                                    &dfEE );
        status = nc_put_att_double( fpImage,
                                    NCDFVarID,
                                    "Westernmost_Easting",
                                    NC_DOUBLE,
                                    1,
                                    &dfWE );
        pszWKT = (char *) poSrcDS->GetProjectionRef() ;
        nc_put_att_text( fpImage, 
                         NCDFVarID, 
                         "spatial_ref",
                         strlen( pszWKT ),
                         pszWKT );
        nc_put_att_text( fpImage, 
                         NCDFVarID, 
                         "GeoTransform",
                         strlen( szGeoTransform ),
                         szGeoTransform );
        nc_put_att_text( fpImage, 
                         NCDFVarID, 
                         GRD_MAPPING_NAME,
                         strlen( pszNetcdfProjection ),
                         pszNetcdfProjection );

        for( int iChild = 0; iChild < poPROJCS->GetChildCount(); iChild++ )
        {
            const OGR_SRSNode    *poNode;
            float fValue;

            poNode = poPROJCS->GetChild( iChild );
            if( !EQUAL(poNode->GetValue(),"PARAMETER") 
                || poNode->GetChildCount() != 2 )
                continue;

/* -------------------------------------------------------------------- */
/*      Look for projection attributes                                  */
/* -------------------------------------------------------------------- */
            pszParamStr = poNode->GetChild(0)->GetValue();
            pszParamVal = poNode->GetChild(1)->GetValue();
	    

            for(i=0; poNetcdfSRS[i].netCDFSRS != NULL; i++ ) {
                if( EQUAL( poNetcdfSRS[i].SRS, pszParamStr ) ) {
                    CPLDebug( "GDAL_netCDF", "%s = %s", 
                              poNetcdfSRS[i].netCDFSRS, 
                              pszParamVal );
                    break;
                }
            }
/* -------------------------------------------------------------------- */
/*      Write Projection attribute                                      */
/* -------------------------------------------------------------------- */
            sscanf( pszParamVal, "%f", &fValue );
            if( poNetcdfSRS[i].netCDFSRS != NULL ) {
                nc_put_att_float( fpImage, 
                                  NCDFVarID, 
                                  poNetcdfSRS[i].netCDFSRS, 
                                  NC_FLOAT,
                                  1,
                                  &fValue );

            }	
        }
    }

/* -------------------------------------------------------------------- */
/*      Initialize Band Map                                             */
/* -------------------------------------------------------------------- */

    for(int j=1; j <= nBands; j++ ) {
        anBandMap[j-1]=j;
    }
    
/* -------------------------------------------------------------------- */
/*      Create netCDF variable                                          */
/* -------------------------------------------------------------------- */

    for( int i=1; i <= nBands; i++ ) {

        char      szBandName[ NC_MAX_NAME ];
        GByte     *pabScanline  = NULL;
        GInt16    *pasScanline  = NULL;
        GInt32    *panScanline  = NULL;
        float     *pafScanline  = NULL;
        double    *padScanline  = NULL;
        int       NCDFVarID;
        size_t    start[ GDALNBDIM ];
        size_t    count[ GDALNBDIM ];
        double    dfNoDataValue;
        unsigned char      cNoDataValue;
        float     fNoDataValue;
        int       nlNoDataValue;
        short     nsNoDataValue;
        GDALRasterBandH	hBand;
        const char *tmpMetadata;
        char      szLongName[ NC_MAX_NAME ];

        GDALRasterBand *poSrcBand = poSrcDS->GetRasterBand( i );
        hBand = GDALGetRasterBand( poSrcDS, i );

        /* Get var name from NETCDF_VARNAME */
        tmpMetadata = poSrcBand->GetMetadataItem("NETCDF_VARNAME");
       	if( tmpMetadata != NULL) {
            if(nBands>1) sprintf(szBandName,"%s%d",tmpMetadata,i);
            else strcpy( szBandName, tmpMetadata );
            // poSrcBand->SetMetadataItem("NETCDF_VARNAME","");
        }
        else 
            sprintf( szBandName, "Band%d", i );

        /* Get long_name from <var>#long_name */
        sprintf(szLongName,"%s#long_name",poSrcBand->GetMetadataItem("NETCDF_VARNAME"));
        tmpMetadata = poSrcDS->GetMetadataItem(szLongName);
        if( tmpMetadata != NULL) 
            strcpy( szLongName, tmpMetadata);
        else 
            sprintf( szLongName, "GDAL Band Number %d", i); 

/* -------------------------------------------------------------------- */
/*      Get Data type                                                   */
/* -------------------------------------------------------------------- */
 
        eDT = poSrcDS->GetRasterBand(i)->GetRasterDataType();
        anBandDims[0] = nYDimID;
        anBandDims[1] = nXDimID;
        CPLErr      eErr = CE_None;

        dfNoDataValue = poSrcBand->GetNoDataValue(0);

        if( eDT == GDT_Byte ) {
            CPLDebug( "GDAL_netCDF", "%s = GDT_Byte ", szBandName );

            status = nc_def_var( fpImage, szBandName, NC_BYTE, 
                                 GDALNBDIM, anBandDims, &NCDFVarID );


/* -------------------------------------------------------------------- */
/*      Write data line per line                                        */
/* -------------------------------------------------------------------- */

            pabScanline = (GByte *) CPLMalloc( nBands * nXSize );
            for( int iLine = 0; iLine < nYSize && eErr == CE_None; iLine++ )  {

/* -------------------------------------------------------------------- */
/*      Read data from band i                                           */
/* -------------------------------------------------------------------- */
                eErr = poSrcBand->RasterIO( GF_Read, 0, iLine, nXSize, 1, 
                                            pabScanline, nXSize, 1, GDT_Byte,
                                            0,0);

/* -------------------------------------------------------------------- */
/*      Write Fill Value                                                */
/* -------------------------------------------------------------------- */
                
                cNoDataValue=(unsigned char) dfNoDataValue;
                nc_put_att_uchar( fpImage,
                                  NCDFVarID,
                                  _FillValue,
                                  NC_CHAR,
                                  1,
                                  &cNoDataValue );
			   
/* -------------------------------------------------------------------- */
/*      Write Data from Band i                                          */
/* -------------------------------------------------------------------- */
                if ( ! bBottomUp )
                    start[0]=iLine;
                else /* invert latitude values */
                    start[0]=nYSize - iLine - 1;
                start[1]=0;
                count[0]=1;
                count[1]=nXSize;
                
/* -------------------------------------------------------------------- */
/*      Put NetCDF file in data mode.                                   */
/* -------------------------------------------------------------------- */
                status = nc_enddef( fpImage );
                status = nc_put_vara_uchar (fpImage, NCDFVarID, start,
                                            count, pabScanline);
/* -------------------------------------------------------------------- */
/*      Put NetCDF file back in define mode.                            */
/* -------------------------------------------------------------------- */
                status = nc_redef( fpImage );
		
            }
            CPLFree( pabScanline );
/* -------------------------------------------------------------------- */
/*      Int16                                                           */
/* -------------------------------------------------------------------- */

        } else if( ( eDT == GDT_UInt16 ) || ( eDT == GDT_Int16 ) ) {
            CPLDebug( "GDAL_netCDF", "%s = GDT_Int16 ",szBandName );
            status = nc_def_var( fpImage, szBandName, NC_SHORT, 
                                 GDALNBDIM, anBandDims, &NCDFVarID );

            pasScanline = (GInt16 *) CPLMalloc( nBands * nXSize *
                                                sizeof( GInt16 ) );
/* -------------------------------------------------------------------- */
/*      Write Fill Value                                                */
/* -------------------------------------------------------------------- */
            nsNoDataValue= (GInt16) dfNoDataValue;
            nc_put_att_short( fpImage,
                              NCDFVarID,
                              _FillValue,
                              NC_SHORT,
                              1,
                              &nsNoDataValue );
            for( int iLine = 0; iLine < nYSize && eErr == CE_None; iLine++ )  {

                eErr = poSrcBand->RasterIO( GF_Read, 0, iLine, nXSize, 1, 
                                            pasScanline, nXSize, 1, GDT_Int16,
                                            0,0);

                if ( ! bBottomUp )
                    start[0]=iLine;
                else /* invert latitude values */
                    start[0]=nYSize - iLine - 1;
                start[1]=0;
                count[0]=1;
                count[1]=nXSize;


                status = nc_enddef( fpImage );
                status = nc_put_vara_short( fpImage, NCDFVarID, start,
                                            count, pasScanline);
                status = nc_redef( fpImage );
            }
            CPLFree( pasScanline );
/* -------------------------------------------------------------------- */
/*      Int32                                                           */
/* -------------------------------------------------------------------- */

        } else if( (eDT == GDT_UInt32) || (eDT == GDT_Int32) ) {
            CPLDebug( "GDAL_netCDF", "%s = GDT_Int32 ",szBandName );
            status = nc_def_var( fpImage, szBandName, NC_INT, 
                                 GDALNBDIM, anBandDims, &NCDFVarID );

            panScanline = (GInt32 *) CPLMalloc( nBands * nXSize *
                                                sizeof( GInt32 ) );
/* -------------------------------------------------------------------- */
/*      Write Fill Value                                                */
/* -------------------------------------------------------------------- */
            nlNoDataValue= (GInt32) dfNoDataValue;

            nc_put_att_int( fpImage,
                            NCDFVarID,
                            _FillValue,
                            NC_INT,
                            1,
                            &nlNoDataValue );


            for( int iLine = 0; iLine < nYSize && eErr == CE_None; iLine++ )  {

                eErr = poSrcBand->RasterIO( GF_Read, 0, iLine, nXSize, 1, 
                                            panScanline, nXSize, 1, GDT_Int32,
                                            0,0);

                if ( ! bBottomUp )
                    start[0]=iLine;
                else /* invert latitude values */
                    start[0]=nYSize - iLine - 1;
                start[1]=0;
                count[0]=1;
                count[1]=nXSize;


                status = nc_enddef( fpImage );
                status = nc_put_vara_int( fpImage, NCDFVarID, start,
                                          count, panScanline);
                status = nc_redef( fpImage );
            }
            CPLFree( panScanline );
/* -------------------------------------------------------------------- */
/*      float                                                           */
/* -------------------------------------------------------------------- */
        } else if( (eDT == GDT_Float32) ) {
            CPLDebug( "GDAL_netCDF", "%s = GDT_Float32 ",szBandName );
            status = nc_def_var( fpImage, szBandName, NC_FLOAT, 
                                 GDALNBDIM, anBandDims, &NCDFVarID );

            pafScanline = (float *) CPLMalloc( nBands * nXSize *
                                               sizeof( float ) );

/* -------------------------------------------------------------------- */
/*      Write Fill Value                                                */
/* -------------------------------------------------------------------- */
            fNoDataValue= (float) dfNoDataValue;
            nc_put_att_float( fpImage,
                              NCDFVarID,
                              _FillValue,
                              NC_FLOAT,
                              1,
                              &fNoDataValue );
			   
            for( int iLine = 0; iLine < nYSize && eErr == CE_None; iLine++ )  {

                eErr = poSrcBand->RasterIO( GF_Read, 0, iLine, nXSize, 1, 
                                            pafScanline, nXSize, 1, 
                                            GDT_Float32,
                                            0,0);

                if ( ! bBottomUp )
                    start[0]=iLine;
                else /* invert latitude values */
                    start[0]=nYSize - iLine - 1;
                start[1]=0;
                count[0]=1;
                count[1]=nXSize;


                status = nc_enddef( fpImage );
                status = nc_put_vara_float( fpImage, NCDFVarID, start,
                                            count, pafScanline);
                status = nc_redef( fpImage );
            }
            CPLFree( pafScanline );
/* -------------------------------------------------------------------- */
/*      double                                                          */
/* -------------------------------------------------------------------- */
        } else if( (eDT == GDT_Float64) ) {
            CPLDebug( "GDAL_netCDF", "%s = GDT_Float64 ",szBandName );
            status = nc_def_var( fpImage, szBandName, NC_DOUBLE, 
                                 GDALNBDIM, anBandDims, &NCDFVarID );

            padScanline = (double *) CPLMalloc( nBands * nXSize *
                                                sizeof( double ) );

/* -------------------------------------------------------------------- */
/*      Write Fill Value                                                */
/* -------------------------------------------------------------------- */
		
            nc_put_att_double( fpImage,
                               NCDFVarID,
                               _FillValue,
                               NC_DOUBLE,
                               1,
                               &dfNoDataValue );

            for( int iLine = 0; iLine < nYSize && eErr == CE_None; iLine++ )  {

                eErr = poSrcBand->RasterIO( GF_Read, 0, iLine, nXSize, 1, 
                                            padScanline, nXSize, 1, 
                                            GDT_Float64,
                                            0,0);

                if ( ! bBottomUp )
                    start[0]=iLine;
                else /* invert latitude values */
                    start[0]=nYSize - iLine - 1;
                start[1]=0;
                count[0]=1;
                count[1]=nXSize;


                status = nc_enddef( fpImage );
                status = nc_put_vara_double( fpImage, NCDFVarID, start,
                                             count, padScanline);
                status = nc_redef( fpImage );
            }
            CPLFree( padScanline );
        }
	
/* -------------------------------------------------------------------- */
/*      Write Projection for band                                       */
/* -------------------------------------------------------------------- */
        if( bWriteGeoTransform == TRUE ) {
            /*	    nc_put_att_text( fpImage, NCDFVarID, 
                    COORDINATES,
                    7,
                    LONLAT );
            */
            nc_put_att_text( fpImage, NCDFVarID, 
                             GRD_MAPPING,
                             strlen( pszNetcdfProjection ),
                             pszNetcdfProjection );
        }

/* -------------------------------------------------------------------- */
/*      Copy Metadata for band                                          */
/* -------------------------------------------------------------------- */

        nc_put_att_text( fpImage, 
                         NCDFVarID, 
                         "long_name", 
                         strlen( szLongName ),
                         szLongName );

        CopyMetadata( (void *) hBand, fpImage, NCDFVarID );
    }

    //    poDstDS->SetGeoTransform( adfGeoTransform );


/* -------------------------------------------------------------------- */
/*      Cleanup and close.                                              */
/* -------------------------------------------------------------------- */
//    CPLFree( pabScanline );

    nc_close( fpImage );
/* -------------------------------------------------------------------- */
/*      Re-open dataset, and copy any auxilary pam information.         */
/* -------------------------------------------------------------------- */
    netCDFDataset *poDS = (netCDFDataset *) GDALOpen( pszFilename, GA_ReadOnly );

    if( poDS )
        poDS->CloneInfo( poSrcDS, GCIF_PAM_DEFAULT );

    return poDS;
}

