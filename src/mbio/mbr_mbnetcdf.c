/*--------------------------------------------------------------------
 *    The MB-system:	mbr_mbnetcdf.c	1/25/02
 *	$Id: mbr_mbnetcdf.c,v 5.0 2002-05-02 04:00:03 caress Exp $
 *
 *    Copyright (c) 2002 by
 *    David W. Caress (caress@mbari.org)
 *      Monterey Bay Aquarium Research Institute
 *      Moss Landing, CA 95039
 *    and Dale N. Chayes (dale@ldeo.columbia.edu)
 *      Lamont-Doherty Earth Observatory
 *      Palisades, NY 10964
 *
 *    See README file for copying and redistribution conditions.
 *--------------------------------------------------------------------*/
/*
 * mbr_mbnetcdf.c contains the functions for reading and writing
 * multibeam data in the MBF_MBNETCDF format.  
 * These functions include:
 *   mbr_alm_mbnetcdf	- allocate read/write memory
 *   mbr_dem_mbnetcdf	- deallocate read/write memory
 *   mbr_rt_mbnetcdf	- read and translate data
 *   mbr_wt_mbnetcdf	- translate and write data
 *
 * Author:	D. W. Caress
 * Date:	January 25, 2002
 * 
 * $Log: not supported by cvs2svn $
 * Revision 1.2  2002/05/02 03:55:34  caress
 * Release 5.0.beta17
 *
 * Revision 1.1  2002/02/22 09:03:43  caress
 * Initial revision
 *
 *
 */

/* standard include files */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <netcdf.h>

/* mbio include files */
#include "../../include/mb_status.h"
#include "../../include/mb_format.h"
#include "../../include/mb_io.h"
#include "../../include/mb_define.h"
#include "../../include/mbsys_netcdf.h"

/* essential function prototypes */
int mbr_register_mbnetcdf(int verbose, void *mbio_ptr, 
		int *error);
int mbr_info_mbnetcdf(int verbose, 
			int *system, 
			int *beams_bath_max, 
			int *beams_amp_max, 
			int *pixels_ss_max, 
			char *format_name, 
			char *system_name, 
			char *format_description, 
			int *numfile, 
			int *filetype, 
			int *variable_beams, 
			int *traveltime, 
			int *beam_flagging, 
			int *nav_source, 
			int *heading_source, 
			int *vru_source, 
			double *beamwidth_xtrack, 
			double *beamwidth_ltrack, 
			int *error);
int mbr_alm_mbnetcdf(int verbose, void *mbio_ptr, int *error);
int mbr_dem_mbnetcdf(int verbose, void *mbio_ptr, int *error);
int mbr_rt_mbnetcdf(int verbose, void *mbio_ptr, void *store_ptr, int *error);
int mbr_wt_mbnetcdf(int verbose, void *mbio_ptr, void *store_ptr, int *error);

static char res_id[]="$Id: mbr_mbnetcdf.c,v 5.0 2002-05-02 04:00:03 caress Exp $";

/*--------------------------------------------------------------------*/
int mbr_register_mbnetcdf(int verbose, void *mbio_ptr, int *error)
{
	char	*function_name = "mbr_register_mbnetcdf";
	int	status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:    %d\n",verbose);
		}

	/* get mb_io_ptr */
	mb_io_ptr = (struct mb_io_struct *) mbio_ptr;

	/* set format info parameters */
	status = mbr_info_mbnetcdf(verbose, 
			&mb_io_ptr->system, 
			&mb_io_ptr->beams_bath_max, 
			&mb_io_ptr->beams_amp_max, 
			&mb_io_ptr->pixels_ss_max, 
			mb_io_ptr->format_name, 
			mb_io_ptr->system_name, 
			mb_io_ptr->format_description, 
			&mb_io_ptr->numfile, 
			&mb_io_ptr->filetype, 
			&mb_io_ptr->variable_beams, 
			&mb_io_ptr->traveltime, 
			&mb_io_ptr->beam_flagging, 
			&mb_io_ptr->nav_source, 
			&mb_io_ptr->heading_source, 
			&mb_io_ptr->vru_source, 
			&mb_io_ptr->beamwidth_xtrack, 
			&mb_io_ptr->beamwidth_ltrack, 
			error);

	/* set format and system specific function pointers */
	mb_io_ptr->mb_io_format_alloc = &mbr_alm_mbnetcdf;
	mb_io_ptr->mb_io_format_free = &mbr_dem_mbnetcdf; 
	mb_io_ptr->mb_io_store_alloc = &mbsys_netcdf_alloc; 
	mb_io_ptr->mb_io_store_free = &mbsys_netcdf_deall; 
	mb_io_ptr->mb_io_read_ping = &mbr_rt_mbnetcdf; 
	mb_io_ptr->mb_io_write_ping = &mbr_wt_mbnetcdf; 
	mb_io_ptr->mb_io_extract = &mbsys_netcdf_extract; 
	mb_io_ptr->mb_io_insert = &mbsys_netcdf_insert; 
	mb_io_ptr->mb_io_extract_nav = &mbsys_netcdf_extract_nav; 
	mb_io_ptr->mb_io_insert_nav = &mbsys_netcdf_insert_nav; 
	mb_io_ptr->mb_io_extract_altitude = &mbsys_netcdf_extract_altitude; 
	mb_io_ptr->mb_io_insert_altitude = &mbsys_netcdf_insert_altitude; 
	mb_io_ptr->mb_io_extract_svp = NULL; 
	mb_io_ptr->mb_io_insert_svp = NULL; 
	mb_io_ptr->mb_io_ttimes = &mbsys_netcdf_ttimes; 
	mb_io_ptr->mb_io_copyrecord = &mbsys_netcdf_copy; 
	mb_io_ptr->mb_io_extract_rawss = NULL; 
	mb_io_ptr->mb_io_insert_rawss = NULL; 

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");	
		fprintf(stderr,"dbg2       system:             %d\n",mb_io_ptr->system);
		fprintf(stderr,"dbg2       beams_bath_max:     %d\n",mb_io_ptr->beams_bath_max);
		fprintf(stderr,"dbg2       beams_amp_max:      %d\n",mb_io_ptr->beams_amp_max);
		fprintf(stderr,"dbg2       pixels_ss_max:      %d\n",mb_io_ptr->pixels_ss_max);
		fprintf(stderr,"dbg2       format_name:        %s\n",mb_io_ptr->format_name);
		fprintf(stderr,"dbg2       system_name:        %s\n",mb_io_ptr->system_name);
		fprintf(stderr,"dbg2       format_description: %s\n",mb_io_ptr->format_description);
		fprintf(stderr,"dbg2       numfile:            %d\n",mb_io_ptr->numfile);
		fprintf(stderr,"dbg2       filetype:           %d\n",mb_io_ptr->filetype);
		fprintf(stderr,"dbg2       variable_beams:     %d\n",mb_io_ptr->variable_beams);
		fprintf(stderr,"dbg2       traveltime:         %d\n",mb_io_ptr->traveltime);
		fprintf(stderr,"dbg2       beam_flagging:      %d\n",mb_io_ptr->beam_flagging);
		fprintf(stderr,"dbg2       nav_source:         %d\n",mb_io_ptr->nav_source);
		fprintf(stderr,"dbg2       heading_source:     %d\n",mb_io_ptr->heading_source);
		fprintf(stderr,"dbg2       vru_source:         %d\n",mb_io_ptr->vru_source);
		fprintf(stderr,"dbg2       heading_source:     %d\n",mb_io_ptr->heading_source);
		fprintf(stderr,"dbg2       beamwidth_xtrack:   %f\n",mb_io_ptr->beamwidth_xtrack);
		fprintf(stderr,"dbg2       beamwidth_ltrack:   %f\n",mb_io_ptr->beamwidth_ltrack);
		fprintf(stderr,"dbg2       format_alloc:       %d\n",mb_io_ptr->mb_io_format_alloc);
		fprintf(stderr,"dbg2       format_free:        %d\n",mb_io_ptr->mb_io_format_free);
		fprintf(stderr,"dbg2       store_alloc:        %d\n",mb_io_ptr->mb_io_store_alloc);
		fprintf(stderr,"dbg2       store_free:         %d\n",mb_io_ptr->mb_io_store_free);
		fprintf(stderr,"dbg2       read_ping:          %d\n",mb_io_ptr->mb_io_read_ping);
		fprintf(stderr,"dbg2       write_ping:         %d\n",mb_io_ptr->mb_io_write_ping);
		fprintf(stderr,"dbg2       extract:            %d\n",mb_io_ptr->mb_io_extract);
		fprintf(stderr,"dbg2       insert:             %d\n",mb_io_ptr->mb_io_insert);
		fprintf(stderr,"dbg2       extract_nav:        %d\n",mb_io_ptr->mb_io_extract_nav);
		fprintf(stderr,"dbg2       insert_nav:         %d\n",mb_io_ptr->mb_io_insert_nav);
		fprintf(stderr,"dbg2       extract_altitude:   %d\n",mb_io_ptr->mb_io_extract_altitude);
		fprintf(stderr,"dbg2       insert_altitude:    %d\n",mb_io_ptr->mb_io_insert_altitude);
		fprintf(stderr,"dbg2       extract_svp:        %d\n",mb_io_ptr->mb_io_extract_svp);
		fprintf(stderr,"dbg2       insert_svp:         %d\n",mb_io_ptr->mb_io_insert_svp);
		fprintf(stderr,"dbg2       ttimes:             %d\n",mb_io_ptr->mb_io_ttimes);
		fprintf(stderr,"dbg2       extract_rawss:      %d\n",mb_io_ptr->mb_io_extract_rawss);
		fprintf(stderr,"dbg2       insert_rawss:       %d\n",mb_io_ptr->mb_io_insert_rawss);
		fprintf(stderr,"dbg2       copyrecord:         %d\n",mb_io_ptr->mb_io_copyrecord);
		fprintf(stderr,"dbg2       error:              %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:         %d\n",status);
		}

	/* return status */
	return(status);
}

/*--------------------------------------------------------------------*/
int mbr_info_mbnetcdf(int verbose, 
			int *system, 
			int *beams_bath_max, 
			int *beams_amp_max, 
			int *pixels_ss_max, 
			char *format_name, 
			char *system_name, 
			char *format_description, 
			int *numfile, 
			int *filetype, 
			int *variable_beams, 
			int *traveltime, 
			int *beam_flagging, 
			int *nav_source, 
			int *heading_source, 
			int *vru_source, 
			double *beamwidth_xtrack, 
			double *beamwidth_ltrack, 
			int *error)
{
	char	*function_name = "mbr_info_mbnetcdf";
	int	status = MB_SUCCESS;

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:    %d\n",verbose);
		}

	/* set format info parameters */
	status = MB_SUCCESS;
	*error = MB_ERROR_NO_ERROR;
	*system = MB_SYS_NETCDF;
	*beams_bath_max = 500;
	*beams_amp_max = 0;
	*pixels_ss_max = 0;
	strncpy(format_name, "MBNETCDF", MB_NAME_LENGTH);
	strncpy(system_name, "NETCDF", MB_NAME_LENGTH);
	strncpy(format_description, "Format name:          MBF_MBNETCDF\nInformal Description: IFREMER generic multibeam\nAttributes:           Data from all sonar systems, bathymetry only, \n                      variable beams, netCDF, IFREMER.\n", MB_DESCRIPTION_LENGTH);
	*numfile = 1;
	*filetype = MB_FILETYPE_NETCDF;
	*variable_beams = MB_YES;
	*traveltime = MB_NO;
	*beam_flagging = MB_YES;
	*nav_source = MB_DATA_DATA;
	*heading_source = MB_DATA_DATA;
	*vru_source = MB_DATA_DATA;
	*beamwidth_xtrack = 0.0;
	*beamwidth_ltrack = 0.0;

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");	
		fprintf(stderr,"dbg2       system:             %d\n",*system);
		fprintf(stderr,"dbg2       beams_bath_max:     %d\n",*beams_bath_max);
		fprintf(stderr,"dbg2       beams_amp_max:      %d\n",*beams_amp_max);
		fprintf(stderr,"dbg2       pixels_ss_max:      %d\n",*pixels_ss_max);
		fprintf(stderr,"dbg2       format_name:        %s\n",format_name);
		fprintf(stderr,"dbg2       system_name:        %s\n",system_name);
		fprintf(stderr,"dbg2       format_description: %s\n",format_description);
		fprintf(stderr,"dbg2       numfile:            %d\n",*numfile);
		fprintf(stderr,"dbg2       filetype:           %d\n",*filetype);
		fprintf(stderr,"dbg2       variable_beams:     %d\n",*variable_beams);
		fprintf(stderr,"dbg2       traveltime:         %d\n",*traveltime);
		fprintf(stderr,"dbg2       beam_flagging:      %d\n",*beam_flagging);
		fprintf(stderr,"dbg2       nav_source:         %d\n",*nav_source);
		fprintf(stderr,"dbg2       heading_source:     %d\n",*heading_source);
		fprintf(stderr,"dbg2       vru_source:         %d\n",*vru_source);
		fprintf(stderr,"dbg2       heading_source:     %d\n",*heading_source);
		fprintf(stderr,"dbg2       beamwidth_xtrack:   %f\n",*beamwidth_xtrack);
		fprintf(stderr,"dbg2       beamwidth_ltrack:   %f\n",*beamwidth_ltrack);
		fprintf(stderr,"dbg2       error:              %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:         %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/
int mbr_alm_mbnetcdf(int verbose, void *mbio_ptr, int *error)
{
	char	*function_name = "mbr_alm_mbnetcdf";
	int	status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;
	struct mbsys_netcdf_struct *store;
	int	*dataread;
	int	*commentread;
	int	*svpread;
	int	*recread;

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:    %d\n",verbose);
		fprintf(stderr,"dbg2       mbio_ptr:   %d\n",mbio_ptr);
		}

	/* get pointer to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *) mbio_ptr;

	/* set initial status */
	status = MB_SUCCESS;

	/* allocate memory for data structure */
	status = mbsys_netcdf_alloc(verbose,mbio_ptr,
		&mb_io_ptr->store_data,error);

	/* get pointer to raw data structure */
	store = (struct mbsys_netcdf_struct *) mb_io_ptr->store_data;

	/* initialize values in structure */
	dataread = (int *) &mb_io_ptr->save1;
	commentread = (int *) &mb_io_ptr->save2;
	svpread = (int *) &mb_io_ptr->save3;
	recread = (int *) &mb_io_ptr->save4;
	*dataread = MB_NO;
	*commentread = 0;
	*svpread = 0;
	*recread = 0;

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       error:      %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:  %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/
int mbr_dem_mbnetcdf(int verbose, void *mbio_ptr, int *error)
{
	char	*function_name = "mbr_dem_mbnetcdf";
	int	status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:    %d\n",verbose);
		fprintf(stderr,"dbg2       mbio_ptr:   %d\n",mbio_ptr);
		}

	/* get pointer to mbio descriptor */
	mb_io_ptr = (struct mb_io_struct *) mbio_ptr;

	/* deallocate memory for data descriptor */
	status = mbsys_netcdf_deall(verbose,mbio_ptr,
		&mb_io_ptr->store_data,error);

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       error:      %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:  %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/
int mbr_rt_mbnetcdf(int verbose, void *mbio_ptr, void *store_ptr, int *error)
{
	char	*function_name = "mbr_rt_mbnetcdf";
	int	status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;
	struct mbsys_netcdf_struct *store;
	int	*dataread;
	int	*commentread;
	int	*svpread;
	int	*recread;
	int	dim_id;
	size_t	index[2], count[2];
	int nc_status;
	int	i;

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:    %d\n",verbose);
		fprintf(stderr,"dbg2       mbio_ptr:   %d\n",mbio_ptr);
		fprintf(stderr,"dbg2       store_ptr:  %d\n",store_ptr);
		}

	/* get pointer to mbio descriptor and data structure */
	mb_io_ptr = (struct mb_io_struct *) mbio_ptr;
	store = (struct mbsys_netcdf_struct *) store_ptr;
	dataread = (int *) &mb_io_ptr->save1;
	commentread = (int *) &mb_io_ptr->save2;
	svpread = (int *) &mb_io_ptr->save3;
	recread = (int *) &mb_io_ptr->save4;
	
	/* set file position */
	mb_io_ptr->file_pos = mb_io_ptr->file_bytes;

	/* if first read then set everything up */
	if (*dataread == MB_NO)
	    {
	    *dataread = MB_YES;

	    /* get dimensions */
	    nc_status = nc_inq_dimid((int)mb_io_ptr->mbfp, "mbHistoryRecNbr", &dim_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimid mbHistoryRecNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimlen((int)mb_io_ptr->mbfp, dim_id, &store->mbHistoryRecNbr);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimlen mbHistoryRecNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimid((int)mb_io_ptr->mbfp, "mbNameLength", &dim_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimid mbNameLength error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimlen((int)mb_io_ptr->mbfp, dim_id, &store->mbNameLength);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimlen mbNameLength error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimid((int)mb_io_ptr->mbfp, "mbCommentLength", &dim_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimid mbCommentLength error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimlen((int)mb_io_ptr->mbfp, dim_id, &store->mbCommentLength);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimlen mbCommentLength error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimid((int)mb_io_ptr->mbfp, "mbAntennaNbr", &dim_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimid mbAntennaNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimlen((int)mb_io_ptr->mbfp, dim_id, &store->mbAntennaNbr);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimlen mbAntennaNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimid((int)mb_io_ptr->mbfp, "mbBeamNbr", &dim_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimid mbBeamNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimlen((int)mb_io_ptr->mbfp, dim_id, &store->mbBeamNbr);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimlen mbBeamNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimid((int)mb_io_ptr->mbfp, "mbCycleNbr", &dim_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimid mbCycleNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimlen((int)mb_io_ptr->mbfp, dim_id, &store->mbCycleNbr);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimlen mbCycleNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimid((int)mb_io_ptr->mbfp, "mbVelocityProfilNbr", &dim_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimid mbVelocityProfilNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_inq_dimlen((int)mb_io_ptr->mbfp, dim_id, &store->mbVelocityProfilNbr);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_dimlen mbVelocityProfilNbr error: %s\n", nc_strerror(nc_status));
	    if (nc_status != NC_NOERR)
		{
		status = MB_FAILURE;
		*error = MB_ERROR_EOF;
		}		
		
	    /* print input debug statements */
	    if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  NetCDF array dimensions read in function <%s>\n",
			function_name);
		fprintf(stderr,"dbg2  Array and variable dimensions:\n");
		fprintf(stderr,"dbg2       status:                  %d\n", status);
		fprintf(stderr,"dbg2       error:                   %d\n", *error);
		fprintf(stderr,"dbg2       nc_status:               %d\n", nc_status);
		fprintf(stderr,"dbg2       mbHistoryRecNbr:         %d\n", store->mbHistoryRecNbr);
		fprintf(stderr,"dbg2       mbNameLength:            %d\n", store->mbNameLength);
		fprintf(stderr,"dbg2       mbCommentLength:         %d\n", store->mbCommentLength);
		fprintf(stderr,"dbg2       mbAntennaNbr:            %d\n", store->mbAntennaNbr);
		fprintf(stderr,"dbg2       mbBeamNbr:               %d\n", store->mbBeamNbr);
		fprintf(stderr,"dbg2       mbCycleNbr:              %d\n", store->mbCycleNbr);
		fprintf(stderr,"dbg2       mbVelocityProfilNbr:     %d\n", store->mbVelocityProfilNbr);
		}
		
	    /* get global attributes */
	    if (status == MB_SUCCESS)
		{
		nc_status = nc_get_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbVersion", &store->mbVersion);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVersion error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbName", store->mbName);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbName error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbClasse", store->mbClasse);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbClasse error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbLevel", &store->mbLevel);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbLevel error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNbrHistoryRec", &store->mbNbrHistoryRec);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbNbrHistoryRec error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbTimeReference", store->mbTimeReference);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTimeReference error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbStartDate", &store->mbStartDate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbStartDate error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbStartTime", &store->mbStartTime);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbStartTime error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEndDate", &store->mbEndDate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbEndDate error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEndTime", &store->mbEndTime);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbEndTime error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNorthLatitude", &store->mbNorthLatitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbNorthLatitude error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSouthLatitude", &store->mbSouthLatitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSouthLatitude error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEastLongitude", &store->mbEastLongitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbEastLongitude error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbWestLongitude", &store->mbWestLongitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbWestLongitude error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbMeridian180", store->mbMeridian180);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbMeridian180 error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbGeoDictionnary", store->mbGeoDictionnary);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbGeoDictionnary error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbGeoRepresentation", store->mbGeoRepresentation);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbGeoRepresentation error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbGeodesicSystem", store->mbGeodesicSystem);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbGeodesicSystem error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidName", store->mbEllipsoidName);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbEllipsoidName error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidA", &store->mbEllipsoidA);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbEllipsoidA error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidInvF", &store->mbEllipsoidInvF);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbEllipsoidInvF error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidE2", &store->mbEllipsoidE2);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbEllipsoidE2 error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbProjType", &store->mbProjType);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbProjType error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbProjParameterValue", &store->mbProjParameterValue[0]);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbProjParameterValue error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbProjParameterCode", store->mbProjParameterCode);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbProjParameterCode error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSounder", &store->mbSounder);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounder error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbShip", store->mbShip);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbShip error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSurvey", store->mbSurvey);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSurvey error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbReference", store->mbReference);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReference error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbAntennaOffset", &store->mbAntennaOffset[0]);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntennaOffset error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbAntennaDelay", &store->mbAntennaDelay);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntennaDelay error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSounderOffset", &store->mbSounderOffset[0]);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderOffset error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSounderDelay", &store->mbSounderDelay);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderDelay error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbVRUOffset", &store->mbVRUOffset[0]);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVRUOffset error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbVRUDelay", &store->mbVRUDelay);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVRUDelay error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbHeadingBias", &store->mbHeadingBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeadingBias error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbRollBias", &store->mbRollBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRollBias error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbPitchBias", &store->mbPitchBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitchBias error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbHeaveBias", &store->mbHeaveBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeaveBias error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbDraught", &store->mbDraught);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDraught error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNavType", &store->mbNavType);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbNavType error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNavRef", store->mbNavRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbNavRef error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbTideType", &store->mbTideType);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTideType error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbTideRef", store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTideRef error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbMinDepth", &store->mbMinDepth);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbMinDepth error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbMaxDepth", &store->mbMaxDepth);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbMaxDepth error: %s\n", nc_strerror(nc_status));
		nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbCycleCounter", &store->mbCycleCounter);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycleCounter error: %s\n", nc_strerror(nc_status));
		if (nc_status != NC_NOERR)
		    {
		    status = MB_FAILURE;
		    *error = MB_ERROR_EOF;
		    }		
		
		/* print input debug statements */
		if (verbose >= 2)
		    {
		    fprintf(stderr,"\ndbg2  NetCDF global attributes read in function <%s>\n",
			    function_name);
		    fprintf(stderr,"dbg2  Global attributes:\n");
		    fprintf(stderr,"dbg2       status:                  %d\n", status);
		    fprintf(stderr,"dbg2       error:                   %d\n", *error);
		    fprintf(stderr,"dbg2       nc_status:             %d\n", nc_status);
		    fprintf(stderr,"dbg2       mbVersion:             %d\n", store->mbVersion);
		    fprintf(stderr,"dbg2       mbName:                %s\n", store->mbName);
		    fprintf(stderr,"dbg2       mbClasse:              %s\n", store->mbClasse);
		    fprintf(stderr,"dbg2       mbLevel:               %d\n", store->mbLevel);
		    fprintf(stderr,"dbg2       mbNbrHistoryRec:       %d\n", store->mbNbrHistoryRec);
		    fprintf(stderr,"dbg2       mbTimeReference:       %s\n", store->mbTimeReference);
		    fprintf(stderr,"dbg2       mbStartDate:           %d\n", store->mbStartDate);
		    fprintf(stderr,"dbg2       mbStartTime:           %d\n", store->mbStartTime);
		    fprintf(stderr,"dbg2       mbEndDate:             %d\n", store->mbEndDate);
		    fprintf(stderr,"dbg2       mbEndTime:             %d\n", store->mbEndTime);
		    fprintf(stderr,"dbg2       mbNorthLatitude:       %f\n", store->mbNorthLatitude);
		    fprintf(stderr,"dbg2       mbSouthLatitude:       %f\n", store->mbSouthLatitude);
		    fprintf(stderr,"dbg2       mbEastLongitude:       %f\n", store->mbEastLongitude);
		    fprintf(stderr,"dbg2       mbWestLongitude:       %f\n", store->mbWestLongitude);
		    fprintf(stderr,"dbg2       mbMeridian180:         %s\n", store->mbMeridian180);
		    fprintf(stderr,"dbg2       mbGeoDictionnary:      %s\n", store->mbGeoDictionnary);
		    fprintf(stderr,"dbg2       mbGeoRepresentation:   %s\n", store->mbGeoRepresentation);
		    fprintf(stderr,"dbg2       mbGeodesicSystem:      %s\n", store->mbGeodesicSystem);
		    fprintf(stderr,"dbg2       mbEllipsoidName:       %s\n", store->mbEllipsoidName);
		    fprintf(stderr,"dbg2       mbEllipsoidA:          %f\n", store->mbEllipsoidA);
		    fprintf(stderr,"dbg2       mbEllipsoidInvF:       %f\n", store->mbEllipsoidInvF);
		    fprintf(stderr,"dbg2       mbEllipsoidE2:         %f\n", store->mbEllipsoidE2);
		    fprintf(stderr,"dbg2       mbProjType:            %d\n", store->mbProjType);
		    for (i=0;i<10;i++)
			fprintf(stderr,"dbg2       mbProjParameterValue[%d]:%f\n", i, store->mbProjParameterValue[i]);
		    fprintf(stderr,"dbg2       mbProjParameterCode:   %s\n", store->mbProjParameterCode);
		    fprintf(stderr,"dbg2       mbSounder:             %d\n", store->mbSounder);
		    fprintf(stderr,"dbg2       mbShip:                %s\n", store->mbShip);
		    fprintf(stderr,"dbg2       mbSurvey:              %s\n", store->mbSurvey);
		    fprintf(stderr,"dbg2       mbReference:           %s\n", store->mbReference);
		    for (i=0;i<3;i++)
			fprintf(stderr,"dbg2       mbAntennaOffset[%d]:    %f\n", i, store->mbAntennaOffset[i]);
		    fprintf(stderr,"dbg2       mbAntennaDelay:        %f\n", store->mbAntennaDelay);
		    for (i=0;i<3;i++)
			fprintf(stderr,"dbg2       mbSounderOffset[%d]:    %f\n", i, store->mbSounderOffset[i]);
		    fprintf(stderr,"dbg2       mbSounderDelay:        %f\n", store->mbSounderDelay);
		    for (i=0;i<3;i++)
			fprintf(stderr,"dbg2       mbVRUOffset[%d]:        %f\n", i, store->mbVRUOffset[i]);
		    fprintf(stderr,"dbg2       mbVRUDelay:            %f\n", store->mbVRUDelay);
		    fprintf(stderr,"dbg2       mbHeadingBias:         %f\n", store->mbHeadingBias);
		    fprintf(stderr,"dbg2       mbRollBias:            %f\n", store->mbRollBias);
		    fprintf(stderr,"dbg2       mbPitchBias:           %f\n", store->mbPitchBias);
		    fprintf(stderr,"dbg2       mbHeaveBias:           %f\n", store->mbHeaveBias);
		    fprintf(stderr,"dbg2       mbDraught:             %f\n", store->mbDraught);
		    fprintf(stderr,"dbg2       mbNavType:             %d\n", store->mbNavType);
		    fprintf(stderr,"dbg2       mbNavRef:              %s\n", store->mbNavRef);
		    fprintf(stderr,"dbg2       mbTideType:            %d\n", store->mbTideType);
		    fprintf(stderr,"dbg2       mbTideRef:             %s\n", store->mbTideRef);
		    fprintf(stderr,"dbg2       mbMinDepth:            %f\n", store->mbMinDepth);
		    fprintf(stderr,"dbg2       mbMaxDepth:            %f\n", store->mbMaxDepth);
		    fprintf(stderr,"dbg2       mbCycleCounter:        %d\n", store->mbCycleCounter);
		    }
		}

	    /* get variable IDs */
	    if (status == MB_SUCCESS)
		{
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbHistDate", &store->mbHistDate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbHistDate_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbHistTime", &store->mbHistTime_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbHistTime_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbHistCode", &store->mbHistCode_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbHistCode_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbHistAutor", &store->mbHistAutor_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbHistAutor_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbHistModule", &store->mbHistModule_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbHistModule_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbHistComment", &store->mbHistComment_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbHistComment_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbCycle", &store->mbCycle_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbCycle_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbDate", &store->mbDate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbDate_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbTime", &store->mbTime_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbTime_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbOrdinate", &store->mbOrdinate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbOrdinate_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbAbscissa", &store->mbAbscissa_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbAbscissa_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbFrequency", &store->mbFrequency_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbFrequency_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbSounderMode", &store->mbSounderMode_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbSounderMode_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbReferenceDepth", &store->mbReferenceDepth_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbReferenceDepth_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbDynamicDraught", &store->mbDynamicDraught_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbDynamicDraught_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbTide", &store->mbTide_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbTide_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbSoundVelocity", &store->mbSoundVelocity_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbSoundVelocity_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbHeading", &store->mbHeading_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbHeading_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbRoll", &store->mbRoll_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbRoll_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbPitch", &store->mbPitch_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbPitch_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbTransmissionHeave", &store->mbTransmissionHeave_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbTransmissionHeave_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbDistanceScale", &store->mbDistanceScale_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbDistanceScale_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbDepthScale", &store->mbDepthScale_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbDepthScale_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbVerticalDepth", &store->mbVerticalDepth_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbVerticalDepth_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbCQuality", &store->mbCQuality_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbCQuality_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbCFlag", &store->mbCFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbCQuality_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbInterlacing", &store->mbInterlacing_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbInterlacing_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbSamplingRate", &store->mbSamplingRate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbSamplingRate_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbAlongDistance", &store->mbAlongDistance_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbAlongDistance_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbAcrossDistance", &store->mbAcrossDistance_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbAcrossDistance_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbDepth", &store->mbDepth_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbDepth_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbSQuality", &store->mbSQuality_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbSQuality_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbSFlag", &store->mbSFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbSFlag_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbAntenna", &store->mbAntenna_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbAntenna_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbBeamBias", &store->mbBeamBias_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbBeamBias_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbBFlag", &store->mbBFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbBFlag_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbBeam", &store->mbBeam_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbBeam_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbAFlag", &store->mbAFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbAFlag_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbVelProfilRef", &store->mbVelProfilRef_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbVelProfilRef_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbVelProfilIdx", &store->mbVelProfilIdx_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbVelProfilIdx_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbVelProfilDate", &store->mbVelProfilDate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbVelProfilDate_id error: %s\n", nc_strerror(nc_status));
		nc_status = nc_inq_varid((int)mb_io_ptr->mbfp, "mbVelProfilTime", &store->mbVelProfilTime_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_inq_varid mbVelProfilTime_id error: %s\n", nc_strerror(nc_status));
		if (nc_status != NC_NOERR)
		    {
		    status = MB_FAILURE;
		    *error = MB_ERROR_EOF;
		    }		
		
		/* print input debug statements */
		if (verbose >= 2)
		    {
		    fprintf(stderr,"\ndbg2  NetCDF variable ids read in function <%s>\n",
			    function_name);
		    fprintf(stderr,"dbg2  Variable ids:\n");
		    fprintf(stderr,"dbg2       status:                  %d\n", status);
		    fprintf(stderr,"dbg2       error:                   %d\n", *error);
		    fprintf(stderr,"dbg2       nc_status:               %d\n", nc_status);
		    fprintf(stderr,"dbg2       mbHistDate_id:           %d\n", store->mbHistDate_id);
		    fprintf(stderr,"dbg2       mbHistTime_id:           %d\n", store->mbHistTime_id);
		    fprintf(stderr,"dbg2       mbHistCode_id:           %d\n", store->mbHistCode_id);
		    fprintf(stderr,"dbg2       mbHistAutor_id:          %d\n", store->mbHistAutor_id);
		    fprintf(stderr,"dbg2       mbHistModule_id:         %d\n", store->mbHistModule_id);
		    fprintf(stderr,"dbg2       mbHistComment_id:        %d\n", store->mbHistComment_id);
		    fprintf(stderr,"dbg2       mbCycle_id:              %d\n", store->mbCycle_id);
		    fprintf(stderr,"dbg2       mbDate_id:               %d\n", store->mbDate_id);
		    fprintf(stderr,"dbg2       mbTime_id:               %d\n", store->mbTime_id);
		    fprintf(stderr,"dbg2       mbOrdinate_id:           %d\n", store->mbOrdinate_id);
		    fprintf(stderr,"dbg2       mbAbscissa_id:           %d\n", store->mbAbscissa_id);
		    fprintf(stderr,"dbg2       mbFrequency_id:          %d\n", store->mbFrequency_id);
		    fprintf(stderr,"dbg2       mbSounderMode_id:        %d\n", store->mbSounderMode_id);
		    fprintf(stderr,"dbg2       mbReferenceDepth_id:     %d\n", store->mbReferenceDepth_id);
		    fprintf(stderr,"dbg2       mbDynamicDraught_id:     %d\n", store->mbDynamicDraught_id);
		    fprintf(stderr,"dbg2       mbTide_id:               %d\n", store->mbTide_id);
		    fprintf(stderr,"dbg2       mbSoundVelocity_id:      %d\n", store->mbSoundVelocity_id);
		    fprintf(stderr,"dbg2       mbHeading_id:            %d\n", store->mbHeading_id);
		    fprintf(stderr,"dbg2       mbRoll_id:               %d\n", store->mbRoll_id);
		    fprintf(stderr,"dbg2       mbPitch_id:              %d\n", store->mbPitch_id);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_id:  %d\n", store->mbTransmissionHeave_id);
		    fprintf(stderr,"dbg2       mbDistanceScale_id:      %d\n", store->mbDistanceScale_id);
		    fprintf(stderr,"dbg2       mbDepthScale_id:         %d\n", store->mbDepthScale_id);
		    fprintf(stderr,"dbg2       mbVerticalDepth_id:      %d\n", store->mbVerticalDepth_id);
		    fprintf(stderr,"dbg2       mbCQuality_id:           %d\n", store->mbCQuality_id);
		    fprintf(stderr,"dbg2       mbCFlag_id:              %d\n", store->mbCFlag_id);
		    fprintf(stderr,"dbg2       mbInterlacing_id:        %d\n", store->mbInterlacing_id);
		    fprintf(stderr,"dbg2       mbSamplingRate_id:       %d\n", store->mbSamplingRate_id);
		    fprintf(stderr,"dbg2       mbAlongDistance_id:      %d\n", store->mbAlongDistance_id);
		    fprintf(stderr,"dbg2       mbAcrossDistance_id:     %d\n", store->mbAcrossDistance_id);
		    fprintf(stderr,"dbg2       mbDepth_id:              %d\n", store->mbDepth_id);
		    fprintf(stderr,"dbg2       mbSQuality_id:           %d\n", store->mbSQuality_id);
		    fprintf(stderr,"dbg2       mbSFlag_id:              %d\n", store->mbSFlag_id);
		    fprintf(stderr,"dbg2       mbAntenna_id:            %d\n", store->mbAntenna_id);
		    fprintf(stderr,"dbg2       mbBeamBias_id:           %d\n", store->mbBeamBias_id);
		    fprintf(stderr,"dbg2       mbBFlag_id:              %d\n", store->mbBFlag_id);
		    fprintf(stderr,"dbg2       mbBeam_id:               %d\n", store->mbBeam_id);
		    fprintf(stderr,"dbg2       mbAFlag_id:              %d\n", store->mbAFlag_id);
		    fprintf(stderr,"dbg2       mbVelProfilRef_id:       %d\n", store->mbVelProfilRef_id);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_id:       %d\n", store->mbVelProfilIdx_id);
		    fprintf(stderr,"dbg2       mbVelProfilDate_id:      %d\n", store->mbVelProfilDate_id);
		    fprintf(stderr,"dbg2       mbVelProfilTime_id:      %d\n", store->mbVelProfilTime_id);
		    }
		}
		
	    /* allocate memory for variables */
	    if (status == MB_SUCCESS)
		{
		status = mb_malloc(verbose, 
			    store->mbHistoryRecNbr * sizeof(int),
			    &store->mbHistDate,error);
		status = mb_malloc(verbose, 
			    store->mbHistoryRecNbr * sizeof(int),
			    &store->mbHistTime,error);
		status = mb_malloc(verbose, 
			    store->mbHistoryRecNbr * sizeof(char),
			    &store->mbHistCode,error);
		status = mb_malloc(verbose, 
			    store->mbHistoryRecNbr * store->mbNameLength * sizeof(char),
			    &store->mbHistAutor,error);
		status = mb_malloc(verbose, 
			    store->mbHistoryRecNbr * store->mbNameLength * sizeof(char),
			    &store->mbHistModule,error);
		status = mb_malloc(verbose, 
			    store->mbHistoryRecNbr * store->mbCommentLength * sizeof(char),
			    &store->mbHistComment,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbCycle,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(int),
			    &store->mbDate,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(int),
			    &store->mbTime,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(int),
			    &store->mbOrdinate,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(int),
			    &store->mbAbscissa,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbFrequency,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbSounderMode,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbReferenceDepth,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbDynamicDraught,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbTide,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbSoundVelocity,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbHeading,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbRoll,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbPitch,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbTransmissionHeave,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbDistanceScale,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbDepthScale,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbVerticalDepth,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbCQuality,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbCFlag,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbInterlacing,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbSamplingRate,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(short),
			    &store->mbAlongDistance,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(short),
			    &store->mbAcrossDistance,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(short),
			    &store->mbDepth,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(char),
			    &store->mbSQuality,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(char),
			    &store->mbSFlag,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(char),
			    &store->mbAntenna,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(short),
			    &store->mbBeamBias,error);
		status = mb_malloc(verbose, 
			    store->mbBeamNbr * sizeof(char),
			    &store->mbBFlag,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(short),
			    &store->mbBeam,error);
		status = mb_malloc(verbose, 
			    store->mbAntennaNbr * sizeof(char),
			    &store->mbAFlag,error);
		status = mb_malloc(verbose, 
			    store->mbVelocityProfilNbr * store->mbCommentLength * sizeof(char),
			    &store->mbVelProfilRef,error);
		status = mb_malloc(verbose, 
			    store->mbVelocityProfilNbr * sizeof(short),
			    &store->mbVelProfilIdx,error);
		status = mb_malloc(verbose, 
			    store->mbVelocityProfilNbr * sizeof(int),
			    &store->mbVelProfilDate,error);
		status = mb_malloc(verbose, 
			    store->mbVelocityProfilNbr * sizeof(int),
			    &store->mbVelProfilTime,error);

		/* deal with a memory allocation failure */
		if (status == MB_FAILURE)
		    {
		    status = mb_free(verbose, &store->mbHistDate, error);
		    status = mb_free(verbose, &store->mbHistTime, error);
		    status = mb_free(verbose, &store->mbHistCode, error);
		    status = mb_free(verbose, &store->mbHistAutor, error);
		    status = mb_free(verbose, &store->mbHistModule, error);
		    status = mb_free(verbose, &store->mbHistComment, error);
		    status = mb_free(verbose, &store->mbCycle, error);
		    status = mb_free(verbose, &store->mbDate, error);
		    status = mb_free(verbose, &store->mbTime, error);
		    status = mb_free(verbose, &store->mbOrdinate, error);
		    status = mb_free(verbose, &store->mbAbscissa, error);
		    status = mb_free(verbose, &store->mbFrequency, error);
		    status = mb_free(verbose, &store->mbSounderMode, error);
		    status = mb_free(verbose, &store->mbReferenceDepth, error);
		    status = mb_free(verbose, &store->mbDynamicDraught, error);
		    status = mb_free(verbose, &store->mbTide, error);
		    status = mb_free(verbose, &store->mbSoundVelocity, error);
		    status = mb_free(verbose, &store->mbHeading, error);
		    status = mb_free(verbose, &store->mbRoll, error);
		    status = mb_free(verbose, &store->mbPitch, error);
		    status = mb_free(verbose, &store->mbTransmissionHeave, error);
		    status = mb_free(verbose, &store->mbDistanceScale, error);
		    status = mb_free(verbose, &store->mbDepthScale, error);
		    status = mb_free(verbose, &store->mbVerticalDepth, error);
		    status = mb_free(verbose, &store->mbCQuality, error);
		    status = mb_free(verbose, &store->mbCFlag, error);
		    status = mb_free(verbose, &store->mbInterlacing, error);
		    status = mb_free(verbose, &store->mbSamplingRate, error);
		    status = mb_free(verbose, &store->mbAlongDistance, error);
		    status = mb_free(verbose, &store->mbAcrossDistance, error);
		    status = mb_free(verbose, &store->mbDepth, error);
		    status = mb_free(verbose, &store->mbSQuality, error);
		    status = mb_free(verbose, &store->mbSFlag, error);
		    status = mb_free(verbose, &store->mbAntenna, error);
		    status = mb_free(verbose, &store->mbBeamBias, error);
		    status = mb_free(verbose, &store->mbBFlag, error);
		    status = mb_free(verbose, &store->mbBeam, error);
		    status = mb_free(verbose, &store->mbAFlag, error);
		    status = mb_free(verbose, &store->mbVelProfilRef, error);
		    status = mb_free(verbose, &store->mbVelProfilIdx, error);
		    status = mb_free(verbose, &store->mbVelProfilDate, error);
		    status = mb_free(verbose, &store->mbVelProfilTime, error);
		    status = MB_FAILURE;
		    *error = MB_ERROR_MEMORY_FAIL;
		    if (verbose >= 2)
			    {
			    fprintf(stderr,"\ndbg2  MBIO function <%s> terminated with error\n",
				    function_name);
			    fprintf(stderr,"dbg2  Return values:\n");
			    fprintf(stderr,"dbg2       error:      %d\n",*error);
			    fprintf(stderr,"dbg2  Return status:\n");
			    fprintf(stderr,"dbg2       status:  %d\n",status);
			    }
		    return(status);
		    }
		}

	    /* read variable attributes */
	    if (status == MB_SUCCESS)
		{
		if (store->mbHistDate_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "type", store->mbHistDate_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "long_name", store->mbHistDate_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "name_code", store->mbHistDate_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "units", store->mbHistDate_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "unit_code", store->mbHistDate_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "add_offset", &store->mbHistDate_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "scale_factor", &store->mbHistDate_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "minimum", &store->mbHistDate_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "maximum", &store->mbHistDate_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "valid_minimum", &store->mbHistDate_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "valid_maximum", &store->mbHistDate_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "missing_value", &store->mbHistDate_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "format_C", store->mbHistDate_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "orientation", store->mbHistDate_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistDate_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistTime_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "type", store->mbHistTime_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "long_name", store->mbHistTime_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "name_code", store->mbHistTime_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "units", store->mbHistTime_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "unit_code", store->mbHistTime_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "add_offset", &store->mbHistTime_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "scale_factor", &store->mbHistTime_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "minimum", &store->mbHistTime_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "maximum", &store->mbHistTime_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "valid_minimum", &store->mbHistTime_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "valid_maximum", &store->mbHistTime_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "missing_value", &store->mbHistTime_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "format_C", store->mbHistTime_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "orientation", store->mbHistTime_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistTime_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistCode_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "type", store->mbHistCode_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "long_name", store->mbHistCode_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "name_code", store->mbHistCode_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "units", store->mbHistCode_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "unit_code", store->mbHistCode_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "add_offset", &store->mbHistCode_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "scale_factor", &store->mbHistCode_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "minimum", &store->mbHistCode_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "maximum", &store->mbHistCode_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "valid_minimum", &store->mbHistCode_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "valid_maximum", &store->mbHistCode_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "missing_value", &store->mbHistCode_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "format_C", store->mbHistCode_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "orientation", store->mbHistCode_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistCode_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistAutor_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, "type", store->mbHistAutor_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistAutor_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, "long_name", store->mbHistAutor_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistAutor_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, "name_code", store->mbHistAutor_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistAutor_name_code error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistModule_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, "type", store->mbHistModule_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistModule_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, "long_name", store->mbHistModule_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistModule_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, "name_code", store->mbHistModule_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistModule_name_code error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistComment_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, "type", store->mbHistComment_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistComment_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, "long_name", store->mbHistComment_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistComment_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, "name_code", store->mbHistComment_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHistComment_name_code error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbCycle_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "type", store->mbCycle_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "long_name", store->mbCycle_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "name_code", store->mbCycle_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "units", store->mbCycle_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "unit_code", store->mbCycle_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "add_offset", &store->mbCycle_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "scale_factor", &store->mbCycle_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "minimum", &store->mbCycle_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "maximum", &store->mbCycle_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "valid_minimum", &store->mbCycle_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "valid_maximum", &store->mbCycle_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "missing_value", &store->mbCycle_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "format_C", store->mbCycle_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "orientation", store->mbCycle_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCycle_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbDate_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "type", store->mbDate_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "long_name", store->mbDate_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "name_code", store->mbDate_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "units", store->mbDate_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "unit_code", store->mbDate_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "add_offset", &store->mbDate_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "scale_factor", &store->mbDate_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "minimum", &store->mbDate_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "maximum", &store->mbDate_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "valid_minimum", &store->mbDate_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "valid_maximum", &store->mbDate_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "missing_value", &store->mbDate_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "format_C", store->mbDate_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "orientation", store->mbDate_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDate_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbTime_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "type", store->mbTime_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "long_name", store->mbTime_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "name_code", store->mbTime_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "units", store->mbTime_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "unit_code", store->mbTime_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "add_offset", &store->mbTime_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "scale_factor", &store->mbTime_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "minimum", &store->mbTime_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "maximum", &store->mbTime_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "valid_minimum", &store->mbTime_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "valid_maximum", &store->mbTime_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "missing_value", &store->mbTime_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "format_C", store->mbTime_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "orientation", store->mbTime_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTime_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbOrdinate_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "type", store->mbOrdinate_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "long_name", store->mbOrdinate_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "name_code", store->mbOrdinate_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "units", store->mbOrdinate_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "unit_code", store->mbOrdinate_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "add_offset", &store->mbOrdinate_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "scale_factor", &store->mbOrdinate_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "minimum", &store->mbOrdinate_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "maximum", &store->mbOrdinate_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "valid_minimum", &store->mbOrdinate_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "valid_maximum", &store->mbOrdinate_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "missing_value", &store->mbOrdinate_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "format_C", store->mbOrdinate_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "orientation", store->mbOrdinate_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbOrdinate_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbAbscissa_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "type", store->mbAbscissa_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "long_name", store->mbAbscissa_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "name_code", store->mbAbscissa_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "units", store->mbAbscissa_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "unit_code", store->mbAbscissa_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "add_offset", &store->mbAbscissa_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "scale_factor", &store->mbAbscissa_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "minimum", &store->mbAbscissa_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "maximum", &store->mbAbscissa_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "valid_minimum", &store->mbAbscissa_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "valid_maximum", &store->mbAbscissa_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "missing_value", &store->mbAbscissa_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "format_C", store->mbAbscissa_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "orientation", store->mbAbscissa_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAbscissa_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbFrequency_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "type", store->mbFrequency_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "long_name", store->mbFrequency_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "name_code", store->mbFrequency_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "units", store->mbFrequency_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "unit_code", store->mbFrequency_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "add_offset", &store->mbFrequency_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "scale_factor", &store->mbFrequency_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "minimum", &store->mbFrequency_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "maximum", &store->mbFrequency_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "valid_minimum", &store->mbFrequency_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "valid_maximum", &store->mbFrequency_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "missing_value", &store->mbFrequency_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "format_C", store->mbFrequency_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "orientation", store->mbFrequency_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbFrequency_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbSounderMode_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "type", store->mbSounderMode_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "long_name", store->mbSounderMode_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "name_code", store->mbSounderMode_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "units", store->mbSounderMode_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "unit_code", store->mbSounderMode_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "add_offset", &store->mbSounderMode_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "scale_factor", &store->mbSounderMode_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "minimum", &store->mbSounderMode_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "maximum", &store->mbSounderMode_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "valid_minimum", &store->mbSounderMode_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "valid_maximum", &store->mbSounderMode_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "missing_value", &store->mbSounderMode_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "format_C", store->mbSounderMode_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "orientation", store->mbSounderMode_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSounderMode_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbReferenceDepth_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "type", store->mbReferenceDepth_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "long_name", store->mbReferenceDepth_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "name_code", store->mbReferenceDepth_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "units", store->mbReferenceDepth_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "unit_code", store->mbReferenceDepth_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "add_offset", &store->mbReferenceDepth_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "scale_factor", &store->mbReferenceDepth_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "minimum", &store->mbReferenceDepth_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "maximum", &store->mbReferenceDepth_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "valid_minimum", &store->mbReferenceDepth_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "valid_maximum", &store->mbReferenceDepth_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "missing_value", &store->mbReferenceDepth_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "format_C", store->mbReferenceDepth_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "orientation", store->mbReferenceDepth_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbReferenceDepth_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbDynamicDraught_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "type", store->mbDynamicDraught_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "long_name", store->mbDynamicDraught_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "name_code", store->mbDynamicDraught_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "units", store->mbDynamicDraught_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "unit_code", store->mbDynamicDraught_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "add_offset", &store->mbDynamicDraught_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "scale_factor", &store->mbDynamicDraught_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "minimum", &store->mbDynamicDraught_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "maximum", &store->mbDynamicDraught_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "valid_minimum", &store->mbDynamicDraught_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "valid_maximum", &store->mbDynamicDraught_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "missing_value", &store->mbDynamicDraught_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "format_C", store->mbDynamicDraught_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "orientation", store->mbDynamicDraught_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDynamicDraught_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbTide_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "type", store->mbTide_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "long_name", store->mbTide_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "name_code", store->mbTide_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "units", store->mbTide_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "unit_code", store->mbTide_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbTide_id, "add_offset", &store->mbTide_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbTide_id, "scale_factor", &store->mbTide_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "minimum", &store->mbTide_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "maximum", &store->mbTide_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "valid_minimum", &store->mbTide_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "valid_maximum", &store->mbTide_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "missing_value", &store->mbTide_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "format_C", store->mbTide_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "orientation", store->mbTide_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTide_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbSoundVelocity_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "type", store->mbSoundVelocity_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "long_name", store->mbSoundVelocity_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "name_code", store->mbSoundVelocity_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "units", store->mbSoundVelocity_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "unit_code", store->mbSoundVelocity_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "add_offset", &store->mbSoundVelocity_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "scale_factor", &store->mbSoundVelocity_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "minimum", &store->mbSoundVelocity_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "maximum", &store->mbSoundVelocity_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "valid_minimum", &store->mbSoundVelocity_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "valid_maximum", &store->mbSoundVelocity_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "missing_value", &store->mbSoundVelocity_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "format_C", store->mbSoundVelocity_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "orientation", store->mbSoundVelocity_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSoundVelocity_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHeading_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "type", store->mbHeading_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "long_name", store->mbHeading_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "name_code", store->mbHeading_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "units", store->mbHeading_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "unit_code", store->mbHeading_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbHeading_id, "add_offset", &store->mbHeading_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbHeading_id, "scale_factor", &store->mbHeading_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "minimum", &store->mbHeading_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "maximum", &store->mbHeading_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "valid_minimum", &store->mbHeading_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "valid_maximum", &store->mbHeading_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "missing_value", &store->mbHeading_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "format_C", store->mbHeading_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "orientation", store->mbHeading_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbHeading_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbRoll_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "type", store->mbRoll_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "long_name", store->mbRoll_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "name_code", store->mbRoll_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "units", store->mbRoll_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "unit_code", store->mbRoll_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbRoll_id, "add_offset", &store->mbRoll_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbRoll_id, "scale_factor", &store->mbRoll_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "minimum", &store->mbRoll_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "maximum", &store->mbRoll_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "valid_minimum", &store->mbRoll_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "valid_maximum", &store->mbRoll_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "missing_value", &store->mbRoll_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "format_C", store->mbRoll_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "orientation", store->mbRoll_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbRoll_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbPitch_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "type", store->mbPitch_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "long_name", store->mbPitch_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "name_code", store->mbPitch_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "units", store->mbPitch_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "unit_code", store->mbPitch_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbPitch_id, "add_offset", &store->mbPitch_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbPitch_id, "scale_factor", &store->mbPitch_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "minimum", &store->mbPitch_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "maximum", &store->mbPitch_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "valid_minimum", &store->mbPitch_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "valid_maximum", &store->mbPitch_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "missing_value", &store->mbPitch_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "format_C", store->mbPitch_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "orientation", store->mbPitch_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbPitch_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbTransmissionHeave_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "type", store->mbTransmissionHeave_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "long_name", store->mbTransmissionHeave_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "name_code", store->mbTransmissionHeave_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "units", store->mbTransmissionHeave_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "unit_code", store->mbTransmissionHeave_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "add_offset", &store->mbTransmissionHeave_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "scale_factor", &store->mbTransmissionHeave_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "minimum", &store->mbTransmissionHeave_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "maximum", &store->mbTransmissionHeave_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "valid_minimum", &store->mbTransmissionHeave_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "valid_maximum", &store->mbTransmissionHeave_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "missing_value", &store->mbTransmissionHeave_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "format_C", store->mbTransmissionHeave_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "orientation", store->mbTransmissionHeave_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbTransmissionHeave_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbDistanceScale_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "type", store->mbDistanceScale_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "long_name", store->mbDistanceScale_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "name_code", store->mbDistanceScale_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "units", store->mbDistanceScale_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "unit_code", store->mbDistanceScale_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "add_offset", &store->mbDistanceScale_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "scale_factor", &store->mbDistanceScale_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "minimum", &store->mbDistanceScale_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "maximum", &store->mbDistanceScale_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "valid_minimum", &store->mbDistanceScale_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "valid_maximum", &store->mbDistanceScale_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "missing_value", &store->mbDistanceScale_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "format_C", store->mbDistanceScale_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "orientation", store->mbDistanceScale_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDistanceScale_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbDepthScale_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "type", store->mbDepthScale_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "long_name", store->mbDepthScale_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "name_code", store->mbDepthScale_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "units", store->mbDepthScale_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "unit_code", store->mbDepthScale_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "add_offset", &store->mbDepthScale_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "scale_factor", &store->mbDepthScale_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "minimum", &store->mbDepthScale_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "maximum", &store->mbDepthScale_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "valid_minimum", &store->mbDepthScale_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "valid_maximum", &store->mbDepthScale_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "missing_value", &store->mbDepthScale_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "format_C", store->mbDepthScale_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "orientation", store->mbDepthScale_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepthScale_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVerticalDepth_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "type", store->mbVerticalDepth_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "long_name", store->mbVerticalDepth_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "name_code", store->mbVerticalDepth_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "units", store->mbVerticalDepth_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "unit_code", store->mbVerticalDepth_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "add_offset", &store->mbVerticalDepth_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "scale_factor", &store->mbVerticalDepth_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "minimum", &store->mbVerticalDepth_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "maximum", &store->mbVerticalDepth_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "valid_minimum", &store->mbVerticalDepth_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "valid_maximum", &store->mbVerticalDepth_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "missing_value", &store->mbVerticalDepth_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "format_C", store->mbVerticalDepth_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "orientation", store->mbVerticalDepth_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVerticalDepth_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbCQuality_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "type", store->mbCQuality_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "long_name", store->mbCQuality_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "name_code", store->mbCQuality_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "units", store->mbCQuality_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "unit_code", store->mbCQuality_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "add_offset", &store->mbCQuality_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "scale_factor", &store->mbCQuality_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "minimum", &store->mbCQuality_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "maximum", &store->mbCQuality_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "valid_minimum", &store->mbCQuality_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "valid_maximum", &store->mbCQuality_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "missing_value", &store->mbCQuality_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "format_C", store->mbCQuality_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "orientation", store->mbCQuality_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCQuality_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbCFlag_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "type", store->mbCFlag_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "long_name", store->mbCFlag_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "name_code", store->mbCFlag_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "units", store->mbCFlag_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "unit_code", store->mbCFlag_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "add_offset", &store->mbCFlag_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "scale_factor", &store->mbCFlag_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "minimum", &store->mbCFlag_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "maximum", &store->mbCFlag_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "valid_minimum", &store->mbCFlag_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "valid_maximum", &store->mbCFlag_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "missing_value", &store->mbCFlag_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "format_C", store->mbCFlag_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "orientation", store->mbCFlag_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbCFlag_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbInterlacing_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "type", store->mbInterlacing_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "long_name", store->mbInterlacing_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "name_code", store->mbInterlacing_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "units", store->mbInterlacing_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "unit_code", store->mbInterlacing_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "add_offset", &store->mbInterlacing_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "scale_factor", &store->mbInterlacing_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "minimum", &store->mbInterlacing_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "maximum", &store->mbInterlacing_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "valid_minimum", &store->mbInterlacing_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "valid_maximum", &store->mbInterlacing_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "missing_value", &store->mbInterlacing_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "format_C", store->mbInterlacing_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "orientation", store->mbInterlacing_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbInterlacing_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbSamplingRate_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "type", store->mbSamplingRate_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "long_name", store->mbSamplingRate_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "name_code", store->mbSamplingRate_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "units", store->mbSamplingRate_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "unit_code", store->mbSamplingRate_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "add_offset", &store->mbSamplingRate_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "scale_factor", &store->mbSamplingRate_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "minimum", &store->mbSamplingRate_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "maximum", &store->mbSamplingRate_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "valid_minimum", &store->mbSamplingRate_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "valid_maximum", &store->mbSamplingRate_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "missing_value", &store->mbSamplingRate_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "format_C", store->mbSamplingRate_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "orientation", store->mbSamplingRate_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSamplingRate_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbAlongDistance_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "type", store->mbAlongDistance_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "long_name", store->mbAlongDistance_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "name_code", store->mbAlongDistance_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "units", store->mbAlongDistance_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "unit_code", store->mbAlongDistance_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "add_offset", &store->mbAlongDistance_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "scale_factor", &store->mbAlongDistance_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "minimum", &store->mbAlongDistance_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "maximum", &store->mbAlongDistance_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "valid_minimum", &store->mbAlongDistance_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "valid_maximum", &store->mbAlongDistance_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "missing_value", &store->mbAlongDistance_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "format_C", store->mbAlongDistance_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "orientation", store->mbAlongDistance_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAlongDistance_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbAcrossDistance_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "type", store->mbAcrossDistance_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "long_name", store->mbAcrossDistance_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "name_code", store->mbAcrossDistance_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "units", store->mbAcrossDistance_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "unit_code", store->mbAcrossDistance_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "add_offset", &store->mbAcrossDistance_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "scale_factor", &store->mbAcrossDistance_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "minimum", &store->mbAcrossDistance_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "maximum", &store->mbAcrossDistance_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "valid_minimum", &store->mbAcrossDistance_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "valid_maximum", &store->mbAcrossDistance_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "missing_value", &store->mbAcrossDistance_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "format_C", store->mbAcrossDistance_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "orientation", store->mbAcrossDistance_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAcrossDistance_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbDepth_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "type", store->mbDepth_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "long_name", store->mbDepth_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "name_code", store->mbDepth_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "units", store->mbDepth_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "unit_code", store->mbDepth_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "add_offset", &store->mbDepth_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "scale_factor", &store->mbDepth_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "minimum", &store->mbDepth_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "maximum", &store->mbDepth_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "valid_minimum", &store->mbDepth_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "valid_maximum", &store->mbDepth_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "missing_value", &store->mbDepth_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "format_C", store->mbDepth_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "orientation", store->mbDepth_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbDepth_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbSQuality_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "type", store->mbSQuality_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "long_name", store->mbSQuality_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "name_code", store->mbSQuality_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "units", store->mbSQuality_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "unit_code", store->mbSQuality_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "add_offset", &store->mbSQuality_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "scale_factor", &store->mbSQuality_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "minimum", &store->mbSQuality_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "maximum", &store->mbSQuality_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "valid_minimum", &store->mbSQuality_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "valid_maximum", &store->mbSQuality_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "missing_value", &store->mbSQuality_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "format_C", store->mbSQuality_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "orientation", store->mbSQuality_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSQuality_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbSFlag_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "type", store->mbSFlag_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "long_name", store->mbSFlag_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "name_code", store->mbSFlag_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "units", store->mbSFlag_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "unit_code", store->mbSFlag_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "add_offset", &store->mbSFlag_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "scale_factor", &store->mbSFlag_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "minimum", &store->mbSFlag_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "maximum", &store->mbSFlag_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "valid_minimum", &store->mbSFlag_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "valid_maximum", &store->mbSFlag_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "missing_value", &store->mbSFlag_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "format_C", store->mbSFlag_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "orientation", store->mbSFlag_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbSFlag_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbAntenna_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "type", store->mbAntenna_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "long_name", store->mbAntenna_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "name_code", store->mbAntenna_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "units", store->mbAntenna_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "unit_code", store->mbAntenna_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "add_offset", &store->mbAntenna_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "scale_factor", &store->mbAntenna_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "minimum", &store->mbAntenna_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "maximum", &store->mbAntenna_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "valid_minimum", &store->mbAntenna_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "valid_maximum", &store->mbAntenna_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "missing_value", &store->mbAntenna_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "format_C", store->mbAntenna_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "orientation", store->mbAntenna_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAntenna_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbBeamBias_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "type", store->mbBeamBias_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "long_name", store->mbBeamBias_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "name_code", store->mbBeamBias_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "units", store->mbBeamBias_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "unit_code", store->mbBeamBias_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "add_offset", &store->mbBeamBias_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_double((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "scale_factor", &store->mbBeamBias_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "minimum", &store->mbBeamBias_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "maximum", &store->mbBeamBias_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "valid_minimum", &store->mbBeamBias_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "valid_maximum", &store->mbBeamBias_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "missing_value", &store->mbBeamBias_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "format_C", store->mbBeamBias_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "orientation", store->mbBeamBias_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeamBias_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbBFlag_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "type", store->mbBFlag_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "long_name", store->mbBFlag_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "name_code", store->mbBFlag_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "units", store->mbBFlag_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "unit_code", store->mbBFlag_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "add_offset", &store->mbBFlag_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "scale_factor", &store->mbBFlag_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "minimum", &store->mbBFlag_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "maximum", &store->mbBFlag_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "valid_minimum", &store->mbBFlag_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "valid_maximum", &store->mbBFlag_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "missing_value", &store->mbBFlag_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "format_C", store->mbBFlag_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "orientation", store->mbBFlag_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBFlag_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbBeam_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "type", store->mbBeam_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "long_name", store->mbBeam_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "name_code", store->mbBeam_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "units", store->mbBeam_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "unit_code", store->mbBeam_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "add_offset", &store->mbBeam_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "scale_factor", &store->mbBeam_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "minimum", &store->mbBeam_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "maximum", &store->mbBeam_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "valid_minimum", &store->mbBeam_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "valid_maximum", &store->mbBeam_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "missing_value", &store->mbBeam_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "format_C", store->mbBeam_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "orientation", store->mbBeam_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbBeam_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbAFlag_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "type", store->mbAFlag_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "long_name", store->mbAFlag_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "name_code", store->mbAFlag_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "units", store->mbAFlag_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "unit_code", store->mbAFlag_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "add_offset", &store->mbAFlag_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "scale_factor", &store->mbAFlag_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "minimum", &store->mbAFlag_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "maximum", &store->mbAFlag_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "valid_minimum", &store->mbAFlag_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "valid_maximum", &store->mbAFlag_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "missing_value", &store->mbAFlag_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "format_C", store->mbAFlag_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "orientation", store->mbAFlag_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbAFlag_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilRef_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, "type", store->mbVelProfilRef_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilRef_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, "long_name", store->mbVelProfilRef_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilRef_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, "name_code", store->mbVelProfilRef_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilRef_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "type", store->mbVelProfilIdx_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "long_name", store->mbVelProfilIdx_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "name_code", store->mbVelProfilIdx_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "units", store->mbVelProfilIdx_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "unit_code", store->mbVelProfilIdx_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_unit_code error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilIdx_id >= 0)
		    {
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "add_offset", &store->mbVelProfilIdx_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "scale_factor", &store->mbVelProfilIdx_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "minimum", &store->mbVelProfilIdx_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "maximum", &store->mbVelProfilIdx_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "valid_minimum", &store->mbVelProfilIdx_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "valid_maximum", &store->mbVelProfilIdx_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "missing_value", &store->mbVelProfilIdx_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "format_C", store->mbVelProfilIdx_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "orientation", store->mbVelProfilIdx_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilIdx_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilDate_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "type", store->mbVelProfilDate_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "long_name", store->mbVelProfilDate_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "name_code", store->mbVelProfilDate_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "units", store->mbVelProfilDate_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "unit_code", store->mbVelProfilDate_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "add_offset", &store->mbVelProfilDate_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "scale_factor", &store->mbVelProfilDate_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "minimum", &store->mbVelProfilDate_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "maximum", &store->mbVelProfilDate_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "valid_minimum", &store->mbVelProfilDate_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "valid_maximum", &store->mbVelProfilDate_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "missing_value", &store->mbVelProfilDate_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "format_C", store->mbVelProfilDate_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "orientation", store->mbVelProfilDate_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilDate_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilTime_id >= 0)
		    {
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "type", store->mbVelProfilTime_type);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_type error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "long_name", store->mbVelProfilTime_long_name);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_long_name error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "name_code", store->mbVelProfilTime_name_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_name_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "units", store->mbVelProfilTime_units);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_units error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "unit_code", store->mbVelProfilTime_unit_code);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_unit_code error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "add_offset", &store->mbVelProfilTime_add_offset);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_add_offset error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "scale_factor", &store->mbVelProfilTime_scale_factor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_scale_factor error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "minimum", &store->mbVelProfilTime_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "maximum", &store->mbVelProfilTime_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "valid_minimum", &store->mbVelProfilTime_valid_minimum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_valid_minimum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "valid_maximum", &store->mbVelProfilTime_valid_maximum);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_valid_maximum error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "missing_value", &store->mbVelProfilTime_missing_value);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_missing_value error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "format_C", store->mbVelProfilTime_format_C);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_format_C error: %s\n", nc_strerror(nc_status));
		    nc_status = nc_get_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "orientation", store->mbVelProfilTime_orientation);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_att mbVelProfilTime_orientation error: %s\n", nc_strerror(nc_status));
		    }
		if (nc_status != NC_NOERR)
		    {
		    status = MB_FAILURE;
		    *error = MB_ERROR_EOF;
		    }		
		
		/* print input debug statements */
		if (verbose >= 2)
		    {
		    fprintf(stderr,"\ndbg2  NetCDF variable attributes read in function <%s>\n",
			    function_name);
		    fprintf(stderr,"dbg2  Variable attributes:\n");
		    fprintf(stderr,"dbg2       status:                  %d\n", status);
		    fprintf(stderr,"dbg2       error:                   %d\n", *error);
		    fprintf(stderr,"dbg2       nc_status:               %d\n", nc_status);
		    fprintf(stderr,"dbg2       mbHistCode_long_name:		%s\n", store->mbHistCode_long_name);
		    fprintf(stderr,"dbg2       mbHistCode_name_code:		%s\n", store->mbHistCode_name_code);
		    fprintf(stderr,"dbg2       mbHistCode_units:	    %s\n", store->mbHistCode_units);
		    fprintf(stderr,"dbg2       mbHistCode_unit_code:		    %s\n", store->mbHistCode_unit_code);
		    fprintf(stderr,"dbg2       mbHistCode_add_offset:		    %d\n", store->mbHistCode_add_offset);
		    fprintf(stderr,"dbg2       mbHistCode_scale_factor:		%d\n", store->mbHistCode_scale_factor);
		    fprintf(stderr,"dbg2       mbHistCode_minimum:	    %d\n", store->mbHistCode_minimum);
		    fprintf(stderr,"dbg2       mbHistCode_maximum:	    %d\n", store->mbHistCode_maximum);
		    fprintf(stderr,"dbg2       mbHistCode_valid_minimum:	    %d\n", store->mbHistCode_valid_minimum);
		    fprintf(stderr,"dbg2       mbHistCode_valid_maximum:	%d\n", store->mbHistCode_valid_maximum);
		    fprintf(stderr,"dbg2       mbHistCode_missing_value:	%d\n", store->mbHistCode_missing_value);
		    fprintf(stderr,"dbg2       mbHistCode_format_C:	    %s\n", store->mbHistCode_format_C);
		    fprintf(stderr,"dbg2       mbHistCode_orientation:		%s\n", store->mbHistCode_orientation);
		    fprintf(stderr,"dbg2       mbHistAutor_type:	    %s\n", store->mbHistAutor_type);
		    fprintf(stderr,"dbg2       mbHistAutor_long_name:		%s\n", store->mbHistAutor_long_name);
		    fprintf(stderr,"dbg2       mbHistAutor_name_code:		%s\n", store->mbHistAutor_name_code);
		    fprintf(stderr,"dbg2       mbHistModule_type:	    %s\n", store->mbHistModule_type);
		    fprintf(stderr,"dbg2       mbHistModule_long_name:		%s\n", store->mbHistModule_long_name);
		    fprintf(stderr,"dbg2       mbHistModule_name_code:		%s\n", store->mbHistModule_name_code);
		    fprintf(stderr,"dbg2       mbHistComment_type:	    %s\n", store->mbHistComment_type);
		    fprintf(stderr,"dbg2       mbHistComment_long_name:		%s\n", store->mbHistComment_long_name);
		    fprintf(stderr,"dbg2       mbHistComment_name_code:		%s\n", store->mbHistComment_name_code);
		    fprintf(stderr,"dbg2       mbCycle_type:		%s\n", store->mbCycle_type);
		    fprintf(stderr,"dbg2       mbCycle_long_name:		%s\n", store->mbCycle_long_name);
		    fprintf(stderr,"dbg2       mbCycle_name_code:		%s\n", store->mbCycle_name_code);
		    fprintf(stderr,"dbg2       mbCycle_units:		%s\n", store->mbCycle_units);
		    fprintf(stderr,"dbg2       mbCycle_unit_code:		%s\n", store->mbCycle_unit_code);
		    fprintf(stderr,"dbg2       mbCycle_add_offset:		%d\n", store->mbCycle_add_offset);
		    fprintf(stderr,"dbg2       mbCycle_scale_factor:		%d\n", store->mbCycle_scale_factor);
		    fprintf(stderr,"dbg2       mbCycle_minimum:		%d\n", store->mbCycle_minimum);
		    fprintf(stderr,"dbg2       mbCycle_maximum:		%d\n", store->mbCycle_maximum);
		    fprintf(stderr,"dbg2       mbCycle_valid_minimum:		%d\n", store->mbCycle_valid_minimum);
		    fprintf(stderr,"dbg2       mbCycle_valid_maximum:		%d\n", store->mbCycle_valid_maximum);
		    fprintf(stderr,"dbg2       mbCycle_missing_value:		%d\n", store->mbCycle_missing_value);
		    fprintf(stderr,"dbg2       mbCycle_format_C:		%s\n", store->mbCycle_format_C);
		    fprintf(stderr,"dbg2       mbCycle_orientation:		%s\n", store->mbCycle_orientation);
		    fprintf(stderr,"dbg2       mbDate_type:		%s\n", store->mbDate_type);
		    fprintf(stderr,"dbg2       mbDate_long_name:		%s\n", store->mbDate_long_name);
		    fprintf(stderr,"dbg2       mbDate_name_code:		%s\n", store->mbDate_name_code);
		    fprintf(stderr,"dbg2       mbDate_units:		%s\n", store->mbDate_units);
		    fprintf(stderr,"dbg2       mbDate_unit_code:		%s\n", store->mbDate_unit_code);
		    fprintf(stderr,"dbg2       mbDate_add_offset:		%d\n", store->mbDate_add_offset);
		    fprintf(stderr,"dbg2       mbDate_scale_factor:		%d\n", store->mbDate_scale_factor);
		    fprintf(stderr,"dbg2       mbDate_minimum:		%d\n", store->mbDate_minimum);
		    fprintf(stderr,"dbg2       mbDate_maximum:		%d\n", store->mbDate_maximum);
		    fprintf(stderr,"dbg2       mbDate_valid_minimum:		%d\n", store->mbDate_valid_minimum);
		    fprintf(stderr,"dbg2       mbDate_valid_maximum:		%d\n", store->mbDate_valid_maximum);
		    fprintf(stderr,"dbg2       mbDate_missing_value:		%d\n", store->mbDate_missing_value);
		    fprintf(stderr,"dbg2       mbDate_format_C:		%s\n", store->mbDate_format_C);
		    fprintf(stderr,"dbg2       mbDate_orientation:		%s\n", store->mbDate_orientation);
		    fprintf(stderr,"dbg2       mbTime_type:		%s\n", store->mbTime_type);
		    fprintf(stderr,"dbg2       mbTime_long_name:		%s\n", store->mbTime_long_name);
		    fprintf(stderr,"dbg2       mbTime_name_code:		%s\n", store->mbTime_name_code);
		    fprintf(stderr,"dbg2       mbTime_units:		%s\n", store->mbTime_units);
		    fprintf(stderr,"dbg2       mbTime_unit_code:		%s\n", store->mbTime_unit_code);
		    fprintf(stderr,"dbg2       mbTime_add_offset:		%d\n", store->mbTime_add_offset);
		    fprintf(stderr,"dbg2       mbTime_scale_factor:		%d\n", store->mbTime_scale_factor);
		    fprintf(stderr,"dbg2       mbTime_minimum:		%d\n", store->mbTime_minimum);
		    fprintf(stderr,"dbg2       mbTime_maximum:		%d\n", store->mbTime_maximum);
		    fprintf(stderr,"dbg2       mbTime_valid_minimum:		%d\n", store->mbTime_valid_minimum);
		    fprintf(stderr,"dbg2       mbTime_valid_maximum:		%d\n", store->mbTime_valid_maximum);
		    fprintf(stderr,"dbg2       mbTime_missing_value:		%d\n", store->mbTime_missing_value);
		    fprintf(stderr,"dbg2       mbTime_format_C:		%s\n", store->mbTime_format_C);
		    fprintf(stderr,"dbg2       mbTime_orientation:		%s\n", store->mbTime_orientation);
		    fprintf(stderr,"dbg2       mbOrdinate_type:		%s\n", store->mbOrdinate_type);
		    fprintf(stderr,"dbg2       mbOrdinate_long_name:		%s\n", store->mbOrdinate_long_name);
		    fprintf(stderr,"dbg2       mbOrdinate_name_code:		%s\n", store->mbOrdinate_name_code);
		    fprintf(stderr,"dbg2       mbOrdinate_units:		%s\n", store->mbOrdinate_units);
		    fprintf(stderr,"dbg2       mbOrdinate_unit_code:		%s\n", store->mbOrdinate_unit_code);
		    fprintf(stderr,"dbg2       mbOrdinate_add_offset:		%f\n", store->mbOrdinate_add_offset);
		    fprintf(stderr,"dbg2       mbOrdinate_scale_factor:		%f\n", store->mbOrdinate_scale_factor);
		    fprintf(stderr,"dbg2       mbOrdinate_minimum:		%d\n", store->mbOrdinate_minimum);
		    fprintf(stderr,"dbg2       mbOrdinate_maximum:		%d\n", store->mbOrdinate_maximum);
		    fprintf(stderr,"dbg2       mbOrdinate_valid_minimum:		%d\n", store->mbOrdinate_valid_minimum);
		    fprintf(stderr,"dbg2       mbOrdinate_valid_maximum:		%d\n", store->mbOrdinate_valid_maximum);
		    fprintf(stderr,"dbg2       mbOrdinate_missing_value:		%d\n", store->mbOrdinate_missing_value);
		    fprintf(stderr,"dbg2       mbOrdinate_format_C:		%s\n", store->mbOrdinate_format_C);
		    fprintf(stderr,"dbg2       mbOrdinate_orientation:		%s\n", store->mbOrdinate_orientation);
		    fprintf(stderr,"dbg2       mbAbscissa_type:		%s\n", store->mbAbscissa_type);
		    fprintf(stderr,"dbg2       mbAbscissa_long_name:		%s\n", store->mbAbscissa_long_name);
		    fprintf(stderr,"dbg2       mbAbscissa_name_code:		%s\n", store->mbAbscissa_name_code);
		    fprintf(stderr,"dbg2       mbAbscissa_units:		%s\n", store->mbAbscissa_units);
		    fprintf(stderr,"dbg2       mbAbscissa_unit_code:		%s\n", store->mbAbscissa_unit_code);
		    fprintf(stderr,"dbg2       mbAbscissa_add_offset:		%f\n", store->mbAbscissa_add_offset);
		    fprintf(stderr,"dbg2       mbAbscissa_scale_factor:		%f\n", store->mbAbscissa_scale_factor);
		    fprintf(stderr,"dbg2       mbAbscissa_minimum:		%d\n", store->mbAbscissa_minimum);
		    fprintf(stderr,"dbg2       mbAbscissa_maximum:		%d\n", store->mbAbscissa_maximum);
		    fprintf(stderr,"dbg2       mbAbscissa_valid_minimum:		%d\n", store->mbAbscissa_valid_minimum);
		    fprintf(stderr,"dbg2       mbAbscissa_valid_maximum:		%d\n", store->mbAbscissa_valid_maximum);
		    fprintf(stderr,"dbg2       mbAbscissa_missing_value:		%d\n", store->mbAbscissa_missing_value);
		    fprintf(stderr,"dbg2       mbAbscissa_format_C:		%s\n", store->mbAbscissa_format_C);
		    fprintf(stderr,"dbg2       mbAbscissa_orientation:		%s\n", store->mbAbscissa_orientation);
		    fprintf(stderr,"dbg2       mbFrequency_type:		%s\n", store->mbFrequency_type);
		    fprintf(stderr,"dbg2       mbFrequency_long_name:		%s\n", store->mbFrequency_long_name);
		    fprintf(stderr,"dbg2       mbFrequency_name_code:		%s\n", store->mbFrequency_name_code);
		    fprintf(stderr,"dbg2       mbFrequency_units:		%s\n", store->mbFrequency_units);
		    fprintf(stderr,"dbg2       mbFrequency_unit_code:		%s\n", store->mbFrequency_unit_code);
		    fprintf(stderr,"dbg2       mbFrequency_add_offset:		%d\n", store->mbFrequency_add_offset);
		    fprintf(stderr,"dbg2       mbFrequency_scale_factor:		%d\n", store->mbFrequency_scale_factor);
		    fprintf(stderr,"dbg2       mbFrequency_minimum:		%d\n", store->mbFrequency_minimum);
		    fprintf(stderr,"dbg2       mbFrequency_maximum:		%d\n", store->mbFrequency_maximum);
		    fprintf(stderr,"dbg2       mbFrequency_valid_minimum:		%d\n", store->mbFrequency_valid_minimum);
		    fprintf(stderr,"dbg2       mbFrequency_valid_maximum:		%d\n", store->mbFrequency_valid_maximum);
		    fprintf(stderr,"dbg2       mbFrequency_missing_value:		%d\n", store->mbFrequency_missing_value);
		    fprintf(stderr,"dbg2       mbFrequency_format_C:		%s\n", store->mbFrequency_format_C);
		    fprintf(stderr,"dbg2       mbFrequency_orientation:		%s\n", store->mbFrequency_orientation);
		    fprintf(stderr,"dbg2       mbSounderMode_type:		%s\n", store->mbSounderMode_type);
		    fprintf(stderr,"dbg2       mbSounderMode_long_name:		%s\n", store->mbSounderMode_long_name);
		    fprintf(stderr,"dbg2       mbSounderMode_name_code:		%s\n", store->mbSounderMode_name_code);
		    fprintf(stderr,"dbg2       mbSounderMode_units:		%s\n", store->mbSounderMode_units);
		    fprintf(stderr,"dbg2       mbSounderMode_unit_code:		%s\n", store->mbSounderMode_unit_code);
		    fprintf(stderr,"dbg2       mbSounderMode_add_offset:		%d\n", store->mbSounderMode_add_offset);
		    fprintf(stderr,"dbg2       mbSounderMode_scale_factor:		%d\n", store->mbSounderMode_scale_factor);
		    fprintf(stderr,"dbg2       mbSounderMode_minimum:		%d\n", store->mbSounderMode_minimum);
		    fprintf(stderr,"dbg2       mbSounderMode_maximum:		%d\n", store->mbSounderMode_maximum);
		    fprintf(stderr,"dbg2       mbSounderMode_valid_minimum:		%d\n", store->mbSounderMode_valid_minimum);
		    fprintf(stderr,"dbg2       mbSounderMode_valid_maximum:		%d\n", store->mbSounderMode_valid_maximum);
		    fprintf(stderr,"dbg2       mbSounderMode_missing_value:		%d\n", store->mbSounderMode_missing_value);
		    fprintf(stderr,"dbg2       mbSounderMode_format_C:		%s\n", store->mbSounderMode_format_C);
		    fprintf(stderr,"dbg2       mbSounderMode_orientation:		%s\n", store->mbSounderMode_orientation);
		    fprintf(stderr,"dbg2       mbReferenceDepth_type:		%s\n", store->mbReferenceDepth_type);
		    fprintf(stderr,"dbg2       mbReferenceDepth_long_name:		%s\n", store->mbReferenceDepth_long_name);
		    fprintf(stderr,"dbg2       mbReferenceDepth_name_code:		%s\n", store->mbReferenceDepth_name_code);
		    fprintf(stderr,"dbg2       mbReferenceDepth_units:		%s\n", store->mbReferenceDepth_units);
		    fprintf(stderr,"dbg2       mbReferenceDepth_unit_code:		%s\n", store->mbReferenceDepth_unit_code);
		    fprintf(stderr,"dbg2       mbReferenceDepth_add_offset:		%f\n", store->mbReferenceDepth_add_offset);
		    fprintf(stderr,"dbg2       mbReferenceDepth_scale_factor:		%f\n", store->mbReferenceDepth_scale_factor);
		    fprintf(stderr,"dbg2       mbReferenceDepth_minimum:		%d\n", store->mbReferenceDepth_minimum);
		    fprintf(stderr,"dbg2       mbReferenceDepth_maximum:		%d\n", store->mbReferenceDepth_maximum);
		    fprintf(stderr,"dbg2       mbReferenceDepth_valid_minimum:		%d\n", store->mbReferenceDepth_valid_minimum);
		    fprintf(stderr,"dbg2       mbReferenceDepth_valid_maximum:		%d\n", store->mbReferenceDepth_valid_maximum);
		    fprintf(stderr,"dbg2       mbReferenceDepth_missing_value:		%d\n", store->mbReferenceDepth_missing_value);
		    fprintf(stderr,"dbg2       mbReferenceDepth_format_C:		%s\n", store->mbReferenceDepth_format_C);
		    fprintf(stderr,"dbg2       mbReferenceDepth_orientation:		%s\n", store->mbReferenceDepth_orientation);
		    fprintf(stderr,"dbg2       mbDynamicDraught_type:		%s\n", store->mbDynamicDraught_type);
		    fprintf(stderr,"dbg2       mbDynamicDraught_long_name:		%s\n", store->mbDynamicDraught_long_name);
		    fprintf(stderr,"dbg2       mbDynamicDraught_name_code:		%s\n", store->mbDynamicDraught_name_code);
		    fprintf(stderr,"dbg2       mbDynamicDraught_units:		%s\n", store->mbDynamicDraught_units);
		    fprintf(stderr,"dbg2       mbDynamicDraught_unit_code:		%s\n", store->mbDynamicDraught_unit_code);
		    fprintf(stderr,"dbg2       mbDynamicDraught_add_offset:		%f\n", store->mbDynamicDraught_add_offset);
		    fprintf(stderr,"dbg2       mbDynamicDraught_scale_factor:		%f\n", store->mbDynamicDraught_scale_factor);
		    fprintf(stderr,"dbg2       mbDynamicDraught_minimum:		%d\n", store->mbDynamicDraught_minimum);
		    fprintf(stderr,"dbg2       mbDynamicDraught_maximum:		%d\n", store->mbDynamicDraught_maximum);
		    fprintf(stderr,"dbg2       mbDynamicDraught_valid_minimum:		%d\n", store->mbDynamicDraught_valid_minimum);
		    fprintf(stderr,"dbg2       mbDynamicDraught_valid_maximum:		%d\n", store->mbDynamicDraught_valid_maximum);
		    fprintf(stderr,"dbg2       mbDynamicDraught_missing_value:		%d\n", store->mbDynamicDraught_missing_value);
		    fprintf(stderr,"dbg2       mbDynamicDraught_format_C:		%s\n", store->mbDynamicDraught_format_C);
		    fprintf(stderr,"dbg2       mbDynamicDraught_orientation:		%s\n", store->mbDynamicDraught_orientation);
		    fprintf(stderr,"dbg2       mbTide_type:		%s\n", store->mbTide_type);
		    fprintf(stderr,"dbg2       mbTide_long_name:		%s\n", store->mbTide_long_name);
		    fprintf(stderr,"dbg2       mbTide_name_code:		%s\n", store->mbTide_name_code);
		    fprintf(stderr,"dbg2       mbTide_units:		%s\n", store->mbTide_units);
		    fprintf(stderr,"dbg2       mbTide_unit_code:		%s\n", store->mbTide_unit_code);
		    fprintf(stderr,"dbg2       mbTide_add_offset:		%f\n", store->mbTide_add_offset);
		    fprintf(stderr,"dbg2       mbTide_scale_factor:		%f\n", store->mbTide_scale_factor);
		    fprintf(stderr,"dbg2       mbTide_minimum:		%d\n", store->mbTide_minimum);
		    fprintf(stderr,"dbg2       mbTide_maximum:		%d\n", store->mbTide_maximum);
		    fprintf(stderr,"dbg2       mbTide_valid_minimum:		%d\n", store->mbTide_valid_minimum);
		    fprintf(stderr,"dbg2       mbTide_valid_maximum:		%d\n", store->mbTide_valid_maximum);
		    fprintf(stderr,"dbg2       mbTide_missing_value:		%d\n", store->mbTide_missing_value);
		    fprintf(stderr,"dbg2       mbTide_format_C:		%s\n", store->mbTide_format_C);
		    fprintf(stderr,"dbg2       mbTide_orientation:		%s\n", store->mbTide_orientation);
		    fprintf(stderr,"dbg2       mbSoundVelocity_type:		%s\n", store->mbSoundVelocity_type);
		    fprintf(stderr,"dbg2       mbSoundVelocity_long_name:		%s\n", store->mbSoundVelocity_long_name);
		    fprintf(stderr,"dbg2       mbSoundVelocity_name_code:		%s\n", store->mbSoundVelocity_name_code);
		    fprintf(stderr,"dbg2       mbSoundVelocity_units:		%s\n", store->mbSoundVelocity_units);
		    fprintf(stderr,"dbg2       mbSoundVelocity_unit_code:		%s\n", store->mbSoundVelocity_unit_code);
		    fprintf(stderr,"dbg2       mbSoundVelocity_add_offset:		%f\n", store->mbSoundVelocity_add_offset);
		    fprintf(stderr,"dbg2       mbSoundVelocity_scale_factor:		%f\n", store->mbSoundVelocity_scale_factor);
		    fprintf(stderr,"dbg2       mbSoundVelocity_minimum:		%d\n", store->mbSoundVelocity_minimum);
		    fprintf(stderr,"dbg2       mbSoundVelocity_maximum:		%d\n", store->mbSoundVelocity_maximum);
		    fprintf(stderr,"dbg2       mbSoundVelocity_valid_minimum:		%d\n", store->mbSoundVelocity_valid_minimum);
		    fprintf(stderr,"dbg2       mbSoundVelocity_valid_maximum:		%d\n", store->mbSoundVelocity_valid_maximum);
		    fprintf(stderr,"dbg2       mbSoundVelocity_missing_value:		%d\n", store->mbSoundVelocity_missing_value);
		    fprintf(stderr,"dbg2       mbSoundVelocity_format_C:		%s\n", store->mbSoundVelocity_format_C);
		    fprintf(stderr,"dbg2       mbSoundVelocity_orientation:		%s\n", store->mbSoundVelocity_orientation);
		    fprintf(stderr,"dbg2       mbHeading_type:		%s\n", store->mbHeading_type);
		    fprintf(stderr,"dbg2       mbHeading_long_name:		%s\n", store->mbHeading_long_name);
		    fprintf(stderr,"dbg2       mbHeading_name_code:		%s\n", store->mbHeading_name_code);
		    fprintf(stderr,"dbg2       mbHeading_units:		%s\n", store->mbHeading_units);
		    fprintf(stderr,"dbg2       mbHeading_unit_code:		%s\n", store->mbHeading_unit_code);
		    fprintf(stderr,"dbg2       mbHeading_add_offset:		%f\n", store->mbHeading_add_offset);
		    fprintf(stderr,"dbg2       mbHeading_scale_factor:		%f\n", store->mbHeading_scale_factor);
		    fprintf(stderr,"dbg2       mbHeading_minimum:		%d\n", store->mbHeading_minimum);
		    fprintf(stderr,"dbg2       mbHeading_maximum:		%d\n", store->mbHeading_maximum);
		    fprintf(stderr,"dbg2       mbHeading_valid_minimum:		%d\n", store->mbHeading_valid_minimum);
		    fprintf(stderr,"dbg2       mbHeading_valid_maximum:		%d\n", store->mbHeading_valid_maximum);
		    fprintf(stderr,"dbg2       mbHeading_missing_value:		%d\n", store->mbHeading_missing_value);
		    fprintf(stderr,"dbg2       mbHeading_format_C:		%s\n", store->mbHeading_format_C);
		    fprintf(stderr,"dbg2       mbHeading_orientation:		%s\n", store->mbHeading_orientation);
		    fprintf(stderr,"dbg2       mbRoll_type:		%s\n", store->mbRoll_type);
		    fprintf(stderr,"dbg2       mbRoll_long_name:		%s\n", store->mbRoll_long_name);
		    fprintf(stderr,"dbg2       mbRoll_name_code:		%s\n", store->mbRoll_name_code);
		    fprintf(stderr,"dbg2       mbRoll_units:		%s\n", store->mbRoll_units);
		    fprintf(stderr,"dbg2       mbRoll_unit_code:		%s\n", store->mbRoll_unit_code);
		    fprintf(stderr,"dbg2       mbRoll_add_offset:		%f\n", store->mbRoll_add_offset);
		    fprintf(stderr,"dbg2       mbRoll_scale_factor:		%f\n", store->mbRoll_scale_factor);
		    fprintf(stderr,"dbg2       mbRoll_minimum:		%d\n", store->mbRoll_minimum);
		    fprintf(stderr,"dbg2       mbRoll_maximum:		%d\n", store->mbRoll_maximum);
		    fprintf(stderr,"dbg2       mbRoll_valid_minimum:		%d\n", store->mbRoll_valid_minimum);
		    fprintf(stderr,"dbg2       mbRoll_valid_maximum:		%d\n", store->mbRoll_valid_maximum);
		    fprintf(stderr,"dbg2       mbRoll_missing_value:		%d\n", store->mbRoll_missing_value);
		    fprintf(stderr,"dbg2       mbRoll_format_C:		%s\n", store->mbRoll_format_C);
		    fprintf(stderr,"dbg2       mbRoll_orientation:		%s\n", store->mbRoll_orientation);
		    fprintf(stderr,"dbg2       mbPitch_type:		%s\n", store->mbPitch_type);
		    fprintf(stderr,"dbg2       mbPitch_long_name:		%s\n", store->mbPitch_long_name);
		    fprintf(stderr,"dbg2       mbPitch_name_code:		%s\n", store->mbPitch_name_code);
		    fprintf(stderr,"dbg2       mbPitch_units:		%s\n", store->mbPitch_units);
		    fprintf(stderr,"dbg2       mbPitch_unit_code:		%s\n", store->mbPitch_unit_code);
		    fprintf(stderr,"dbg2       mbPitch_add_offset:		%f\n", store->mbPitch_add_offset);
		    fprintf(stderr,"dbg2       mbPitch_scale_factor:		%f\n", store->mbPitch_scale_factor);
		    fprintf(stderr,"dbg2       mbPitch_minimum:		%d\n", store->mbPitch_minimum);
		    fprintf(stderr,"dbg2       mbPitch_maximum:		%d\n", store->mbPitch_maximum);
		    fprintf(stderr,"dbg2       mbPitch_valid_minimum:		%d\n", store->mbPitch_valid_minimum);
		    fprintf(stderr,"dbg2       mbPitch_valid_maximum:		%d\n", store->mbPitch_valid_maximum);
		    fprintf(stderr,"dbg2       mbPitch_missing_value:		%d\n", store->mbPitch_missing_value);
		    fprintf(stderr,"dbg2       mbPitch_format_C:		%s\n", store->mbPitch_format_C);
		    fprintf(stderr,"dbg2       mbPitch_orientation:		%s\n", store->mbPitch_orientation);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_type:		%s\n", store->mbTransmissionHeave_type);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_long_name:		%s\n", store->mbTransmissionHeave_long_name);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_name_code:		%s\n", store->mbTransmissionHeave_name_code);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_units:		%s\n", store->mbTransmissionHeave_units);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_unit_code:		%s\n", store->mbTransmissionHeave_unit_code);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_add_offset:		%f\n", store->mbTransmissionHeave_add_offset);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_scale_factor:		%f\n", store->mbTransmissionHeave_scale_factor);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_minimum:		%d\n", store->mbTransmissionHeave_minimum);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_maximum:		%d\n", store->mbTransmissionHeave_maximum);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_valid_minimum:		%d\n", store->mbTransmissionHeave_valid_minimum);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_valid_maximum:		%d\n", store->mbTransmissionHeave_valid_maximum);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_missing_value:		%d\n", store->mbTransmissionHeave_missing_value);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_format_C:		%s\n", store->mbTransmissionHeave_format_C);
		    fprintf(stderr,"dbg2       mbTransmissionHeave_orientation:		%s\n", store->mbTransmissionHeave_orientation);
		    fprintf(stderr,"dbg2       mbDistanceScale_type:		%s\n", store->mbDistanceScale_type);
		    fprintf(stderr,"dbg2       mbDistanceScale_long_name:		%s\n", store->mbDistanceScale_long_name);
		    fprintf(stderr,"dbg2       mbDistanceScale_name_code:		%s\n", store->mbDistanceScale_name_code);
		    fprintf(stderr,"dbg2       mbDistanceScale_units:		%s\n", store->mbDistanceScale_units);
		    fprintf(stderr,"dbg2       mbDistanceScale_unit_code:		%s\n", store->mbDistanceScale_unit_code);
		    fprintf(stderr,"dbg2       mbDistanceScale_add_offset:		%f\n", store->mbDistanceScale_add_offset);
		    fprintf(stderr,"dbg2       mbDistanceScale_scale_factor:		%f\n", store->mbDistanceScale_scale_factor);
		    fprintf(stderr,"dbg2       mbDistanceScale_minimum:		%d\n", store->mbDistanceScale_minimum);
		    fprintf(stderr,"dbg2       mbDistanceScale_maximum:		%d\n", store->mbDistanceScale_maximum);
		    fprintf(stderr,"dbg2       mbDistanceScale_valid_minimum:		%d\n", store->mbDistanceScale_valid_minimum);
		    fprintf(stderr,"dbg2       mbDistanceScale_valid_maximum:		%d\n", store->mbDistanceScale_valid_maximum);
		    fprintf(stderr,"dbg2       mbDistanceScale_missing_value:		%d\n", store->mbDistanceScale_missing_value);
		    fprintf(stderr,"dbg2       mbDistanceScale_format_C:		%s\n", store->mbDistanceScale_format_C);
		    fprintf(stderr,"dbg2       mbDistanceScale_orientation:		%s\n", store->mbDistanceScale_orientation);
		    fprintf(stderr,"dbg2       mbDepthScale_type:		%s\n", store->mbDepthScale_type);
		    fprintf(stderr,"dbg2       mbDepthScale_long_name:		%s\n", store->mbDepthScale_long_name);
		    fprintf(stderr,"dbg2       mbDepthScale_name_code:		%s\n", store->mbDepthScale_name_code);
		    fprintf(stderr,"dbg2       mbDepthScale_units:		%s\n", store->mbDepthScale_units);
		    fprintf(stderr,"dbg2       mbDepthScale_unit_code:		%s\n", store->mbDepthScale_unit_code);
		    fprintf(stderr,"dbg2       mbDepthScale_add_offset:		%f\n", store->mbDepthScale_add_offset);
		    fprintf(stderr,"dbg2       mbDepthScale_scale_factor:		%f\n", store->mbDepthScale_scale_factor);
		    fprintf(stderr,"dbg2       mbDepthScale_minimum:		%d\n", store->mbDepthScale_minimum);
		    fprintf(stderr,"dbg2       mbDepthScale_maximum:		%d\n", store->mbDepthScale_maximum);
		    fprintf(stderr,"dbg2       mbDepthScale_valid_minimum:		%d\n", store->mbDepthScale_valid_minimum);
		    fprintf(stderr,"dbg2       mbDepthScale_valid_maximum:		%d\n", store->mbDepthScale_valid_maximum);
		    fprintf(stderr,"dbg2       mbDepthScale_missing_value:		%d\n", store->mbDepthScale_missing_value);
		    fprintf(stderr,"dbg2       mbDepthScale_format_C:		%s\n", store->mbDepthScale_format_C);
		    fprintf(stderr,"dbg2       mbDepthScale_orientation:		%s\n", store->mbDepthScale_orientation);
		    fprintf(stderr,"dbg2       mbVerticalDepth_type:		%s\n", store->mbVerticalDepth_type);
		    fprintf(stderr,"dbg2       mbVerticalDepth_long_name:		%s\n", store->mbVerticalDepth_long_name);
		    fprintf(stderr,"dbg2       mbVerticalDepth_name_code:		%s\n", store->mbVerticalDepth_name_code);
		    fprintf(stderr,"dbg2       mbVerticalDepth_units:		%s\n", store->mbVerticalDepth_units);
		    fprintf(stderr,"dbg2       mbVerticalDepth_unit_code:		%s\n", store->mbVerticalDepth_unit_code);
		    fprintf(stderr,"dbg2       mbVerticalDepth_add_offset:		%d\n", store->mbVerticalDepth_add_offset);
		    fprintf(stderr,"dbg2       mbVerticalDepth_scale_factor:		%d\n", store->mbVerticalDepth_scale_factor);
		    fprintf(stderr,"dbg2       mbVerticalDepth_minimum:		%d\n", store->mbVerticalDepth_minimum);
		    fprintf(stderr,"dbg2       mbVerticalDepth_maximum:		%d\n", store->mbVerticalDepth_maximum);
		    fprintf(stderr,"dbg2       mbVerticalDepth_valid_minimum:		%d\n", store->mbVerticalDepth_valid_minimum);
		    fprintf(stderr,"dbg2       mbVerticalDepth_valid_maximum:		%d\n", store->mbVerticalDepth_valid_maximum);
		    fprintf(stderr,"dbg2       mbVerticalDepth_missing_value:		%d\n", store->mbVerticalDepth_missing_value);
		    fprintf(stderr,"dbg2       mbVerticalDepth_format_C:		%s\n", store->mbVerticalDepth_format_C);
		    fprintf(stderr,"dbg2       mbVerticalDepth_orientation:		%s\n", store->mbVerticalDepth_orientation);
		    fprintf(stderr,"dbg2       mbCQuality_type:		%s\n", store->mbCQuality_type);
		    fprintf(stderr,"dbg2       mbCQuality_long_name:		%s\n", store->mbCQuality_long_name);
		    fprintf(stderr,"dbg2       mbCQuality_name_code:		%s\n", store->mbCQuality_name_code);
		    fprintf(stderr,"dbg2       mbCQuality_units:		%s\n", store->mbCQuality_units);
		    fprintf(stderr,"dbg2       mbCQuality_unit_code:		%s\n", store->mbCQuality_unit_code);
		    fprintf(stderr,"dbg2       mbCQuality_add_offset:		%d\n", store->mbCQuality_add_offset);
		    fprintf(stderr,"dbg2       mbCQuality_scale_factor:		%d\n", store->mbCQuality_scale_factor);
		    fprintf(stderr,"dbg2       mbCQuality_minimum:		%d\n", store->mbCQuality_minimum);
		    fprintf(stderr,"dbg2       mbCQuality_maximum:		%d\n", store->mbCQuality_maximum);
		    fprintf(stderr,"dbg2       mbCQuality_valid_minimum:		%d\n", store->mbCQuality_valid_minimum);
		    fprintf(stderr,"dbg2       mbCQuality_valid_maximum:		%d\n", store->mbCQuality_valid_maximum);
		    fprintf(stderr,"dbg2       mbCQuality_missing_value:		%d\n", store->mbCQuality_missing_value);
		    fprintf(stderr,"dbg2       mbCQuality_format_C:		%s\n", store->mbCQuality_format_C);
		    fprintf(stderr,"dbg2       mbCQuality_orientation:		%s\n", store->mbCQuality_orientation);
		    fprintf(stderr,"dbg2       mbCFlag_type:		%s\n", store->mbCFlag_type);
		    fprintf(stderr,"dbg2       mbCFlag_long_name:		%s\n", store->mbCFlag_long_name);
		    fprintf(stderr,"dbg2       mbCFlag_name_code:		%s\n", store->mbCFlag_name_code);
		    fprintf(stderr,"dbg2       mbCFlag_units:		%s\n", store->mbCFlag_units);
		    fprintf(stderr,"dbg2       mbCFlag_unit_code:		%s\n", store->mbCFlag_unit_code);
		    fprintf(stderr,"dbg2       mbCFlag_add_offset:		%d\n", store->mbCFlag_add_offset);
		    fprintf(stderr,"dbg2       mbCFlag_scale_factor:		%d\n", store->mbCFlag_scale_factor);
		    fprintf(stderr,"dbg2       mbCFlag_minimum:		%d\n", store->mbCFlag_minimum);
		    fprintf(stderr,"dbg2       mbCFlag_maximum:		%d\n", store->mbCFlag_maximum);
		    fprintf(stderr,"dbg2       mbCFlag_valid_minimum:		%d\n", store->mbCFlag_valid_minimum);
		    fprintf(stderr,"dbg2       mbCFlag_valid_maximum:		%d\n", store->mbCFlag_valid_maximum);
		    fprintf(stderr,"dbg2       mbCFlag_missing_value:		%d\n", store->mbCFlag_missing_value);
		    fprintf(stderr,"dbg2       mbCFlag_format_C:		%s\n", store->mbCFlag_format_C);
		    fprintf(stderr,"dbg2       mbCFlag_orientation:		%s\n", store->mbCFlag_orientation);
		    fprintf(stderr,"dbg2       mbInterlacing_type:		%s\n", store->mbInterlacing_type);
		    fprintf(stderr,"dbg2       mbInterlacing_long_name:		%s\n", store->mbInterlacing_long_name);
		    fprintf(stderr,"dbg2       mbInterlacing_name_code:		%s\n", store->mbInterlacing_name_code);
		    fprintf(stderr,"dbg2       mbInterlacing_units:		%s\n", store->mbInterlacing_units);
		    fprintf(stderr,"dbg2       mbInterlacing_unit_code:		%s\n", store->mbInterlacing_unit_code);
		    fprintf(stderr,"dbg2       mbInterlacing_add_offset:		%d\n", store->mbInterlacing_add_offset);
		    fprintf(stderr,"dbg2       mbInterlacing_scale_factor:		%d\n", store->mbInterlacing_scale_factor);
		    fprintf(stderr,"dbg2       mbInterlacing_minimum:		%d\n", store->mbInterlacing_minimum);
		    fprintf(stderr,"dbg2       mbInterlacing_maximum:		%d\n", store->mbInterlacing_maximum);
		    fprintf(stderr,"dbg2       mbInterlacing_valid_minimum:		%d\n", store->mbInterlacing_valid_minimum);
		    fprintf(stderr,"dbg2       mbInterlacing_valid_maximum:		%d\n", store->mbInterlacing_valid_maximum);
		    fprintf(stderr,"dbg2       mbInterlacing_missing_value:		%d\n", store->mbInterlacing_missing_value);
		    fprintf(stderr,"dbg2       mbInterlacing_format_C:		%s\n", store->mbInterlacing_format_C);
		    fprintf(stderr,"dbg2       mbInterlacing_orientation:		%s\n", store->mbInterlacing_orientation);
		    fprintf(stderr,"dbg2       mbSamplingRate_type:		%s\n", store->mbSamplingRate_type);
		    fprintf(stderr,"dbg2       mbSamplingRate_long_name:		%s\n", store->mbSamplingRate_long_name);
		    fprintf(stderr,"dbg2       mbSamplingRate_name_code:		%s\n", store->mbSamplingRate_name_code);
		    fprintf(stderr,"dbg2       mbSamplingRate_units:		%s\n", store->mbSamplingRate_units);
		    fprintf(stderr,"dbg2       mbSamplingRate_unit_code:		%s\n", store->mbSamplingRate_unit_code);
		    fprintf(stderr,"dbg2       mbSamplingRate_add_offset:		%d\n", store->mbSamplingRate_add_offset);
		    fprintf(stderr,"dbg2       mbSamplingRate_scale_factor:		%d\n", store->mbSamplingRate_scale_factor);
		    fprintf(stderr,"dbg2       mbSamplingRate_minimum:		%d\n", store->mbSamplingRate_minimum);
		    fprintf(stderr,"dbg2       mbSamplingRate_maximum:		%d\n", store->mbSamplingRate_maximum);
		    fprintf(stderr,"dbg2       mbSamplingRate_valid_minimum:		%d\n", store->mbSamplingRate_valid_minimum);
		    fprintf(stderr,"dbg2       mbSamplingRate_valid_maximum:		%d\n", store->mbSamplingRate_valid_maximum);
		    fprintf(stderr,"dbg2       mbSamplingRate_missing_value:		%d\n", store->mbSamplingRate_missing_value);
		    fprintf(stderr,"dbg2       mbSamplingRate_format_C:		%s\n", store->mbSamplingRate_format_C);
		    fprintf(stderr,"dbg2       mbSamplingRate_orientation:		%s\n", store->mbSamplingRate_orientation);
		    fprintf(stderr,"dbg2       mbAlongDistance_type:		%s\n", store->mbAlongDistance_type);
		    fprintf(stderr,"dbg2       mbAlongDistance_long_name:		%s\n", store->mbAlongDistance_long_name);
		    fprintf(stderr,"dbg2       mbAlongDistance_name_code:		%s\n", store->mbAlongDistance_name_code);
		    fprintf(stderr,"dbg2       mbAlongDistance_units:		%s\n", store->mbAlongDistance_units);
		    fprintf(stderr,"dbg2       mbAlongDistance_unit_code:		%s\n", store->mbAlongDistance_unit_code);
		    fprintf(stderr,"dbg2       mbAlongDistance_add_offset:		%d\n", store->mbAlongDistance_add_offset);
		    fprintf(stderr,"dbg2       mbAlongDistance_scale_factor:		%d\n", store->mbAlongDistance_scale_factor);
		    fprintf(stderr,"dbg2       mbAlongDistance_minimum:		%d\n", store->mbAlongDistance_minimum);
		    fprintf(stderr,"dbg2       mbAlongDistance_maximum:		%d\n", store->mbAlongDistance_maximum);
		    fprintf(stderr,"dbg2       mbAlongDistance_valid_minimum:		%d\n", store->mbAlongDistance_valid_minimum);
		    fprintf(stderr,"dbg2       mbAlongDistance_valid_maximum:		%d\n", store->mbAlongDistance_valid_maximum);
		    fprintf(stderr,"dbg2       mbAlongDistance_missing_value:		%d\n", store->mbAlongDistance_missing_value);
		    fprintf(stderr,"dbg2       mbAlongDistance_format_C:		%s\n", store->mbAlongDistance_format_C);
		    fprintf(stderr,"dbg2       mbAlongDistance_orientation:		%s\n", store->mbAlongDistance_orientation);
		    fprintf(stderr,"dbg2       mbAcrossDistance_type:		%s\n", store->mbAcrossDistance_type);
		    fprintf(stderr,"dbg2       mbAcrossDistance_long_name:		%s\n", store->mbAcrossDistance_long_name);
		    fprintf(stderr,"dbg2       mbAcrossDistance_name_code:		%s\n", store->mbAcrossDistance_name_code);
		    fprintf(stderr,"dbg2       mbAcrossDistance_units:		%s\n", store->mbAcrossDistance_units);
		    fprintf(stderr,"dbg2       mbAcrossDistance_unit_code:		%s\n", store->mbAcrossDistance_unit_code);
		    fprintf(stderr,"dbg2       mbAcrossDistance_add_offset:		%d\n", store->mbAcrossDistance_add_offset);
		    fprintf(stderr,"dbg2       mbAcrossDistance_scale_factor:		%d\n", store->mbAcrossDistance_scale_factor);
		    fprintf(stderr,"dbg2       mbAcrossDistance_minimum:		%d\n", store->mbAcrossDistance_minimum);
		    fprintf(stderr,"dbg2       mbAcrossDistance_maximum:		%d\n", store->mbAcrossDistance_maximum);
		    fprintf(stderr,"dbg2       mbAcrossDistance_valid_minimum:		%d\n", store->mbAcrossDistance_valid_minimum);
		    fprintf(stderr,"dbg2       mbAcrossDistance_valid_maximum:		%d\n", store->mbAcrossDistance_valid_maximum);
		    fprintf(stderr,"dbg2       mbAcrossDistance_missing_value:		%d\n", store->mbAcrossDistance_missing_value);
		    fprintf(stderr,"dbg2       mbAcrossDistance_format_C:		%s\n", store->mbAcrossDistance_format_C);
		    fprintf(stderr,"dbg2       mbAcrossDistance_orientation:		%s\n", store->mbAcrossDistance_orientation);
		    fprintf(stderr,"dbg2       mbDepth_type:		%s\n", store->mbDepth_type);
		    fprintf(stderr,"dbg2       mbDepth_long_name:		%s\n", store->mbDepth_long_name);
		    fprintf(stderr,"dbg2       mbDepth_name_code:		%s\n", store->mbDepth_name_code);
		    fprintf(stderr,"dbg2       mbDepth_units:		%s\n", store->mbDepth_units);
		    fprintf(stderr,"dbg2       mbDepth_unit_code:		%s\n", store->mbDepth_unit_code);
		    fprintf(stderr,"dbg2       mbDepth_add_offset:		%d\n", store->mbDepth_add_offset);
		    fprintf(stderr,"dbg2       mbDepth_scale_factor:		%d\n", store->mbDepth_scale_factor);
		    fprintf(stderr,"dbg2       mbDepth_minimum:		%d\n", store->mbDepth_minimum);
		    fprintf(stderr,"dbg2       mbDepth_maximum:		%d\n", store->mbDepth_maximum);
		    fprintf(stderr,"dbg2       mbDepth_valid_minimum:		%d\n", store->mbDepth_valid_minimum);
		    fprintf(stderr,"dbg2       mbDepth_valid_maximum:		%d\n", store->mbDepth_valid_maximum);
		    fprintf(stderr,"dbg2       mbDepth_missing_value:		%d\n", store->mbDepth_missing_value);
		    fprintf(stderr,"dbg2       mbDepth_format_C:		%s\n", store->mbDepth_format_C);
		    fprintf(stderr,"dbg2       mbDepth_orientation:		%s\n", store->mbDepth_orientation);
		    fprintf(stderr,"dbg2       mbSQuality_type:		%s\n", store->mbSQuality_type);
		    fprintf(stderr,"dbg2       mbSQuality_long_name:		%s\n", store->mbSQuality_long_name);
		    fprintf(stderr,"dbg2       mbSQuality_name_code:		%s\n", store->mbSQuality_name_code);
		    fprintf(stderr,"dbg2       mbSQuality_units:		%s\n", store->mbSQuality_units);
		    fprintf(stderr,"dbg2       mbSQuality_unit_code:		%s\n", store->mbSQuality_unit_code);
		    fprintf(stderr,"dbg2       mbSQuality_add_offset:		%d\n", store->mbSQuality_add_offset);
		    fprintf(stderr,"dbg2       mbSQuality_scale_factor:		%d\n", store->mbSQuality_scale_factor);
		    fprintf(stderr,"dbg2       mbSQuality_minimum:		%d\n", store->mbSQuality_minimum);
		    fprintf(stderr,"dbg2       mbSQuality_maximum:		%d\n", store->mbSQuality_maximum);
		    fprintf(stderr,"dbg2       mbSQuality_valid_minimum:		%d\n", store->mbSQuality_valid_minimum);
		    fprintf(stderr,"dbg2       mbSQuality_valid_maximum:		%d\n", store->mbSQuality_valid_maximum);
		    fprintf(stderr,"dbg2       mbSQuality_missing_value:		%d\n", store->mbSQuality_missing_value);
		    fprintf(stderr,"dbg2       mbSQuality_format_C:		%s\n", store->mbSQuality_format_C);
		    fprintf(stderr,"dbg2       mbSQuality_orientation:		%s\n", store->mbSQuality_orientation);
		    fprintf(stderr,"dbg2       mbSFlag_type:		%s\n", store->mbSFlag_type);
		    fprintf(stderr,"dbg2       mbSFlag_long_name:		%s\n", store->mbSFlag_long_name);
		    fprintf(stderr,"dbg2       mbSFlag_name_code:		%s\n", store->mbSFlag_name_code);
		    fprintf(stderr,"dbg2       mbSFlag_units:		%s\n", store->mbSFlag_units);
		    fprintf(stderr,"dbg2       mbSFlag_unit_code:		%s\n", store->mbSFlag_unit_code);
		    fprintf(stderr,"dbg2       mbSFlag_add_offset:		%d\n", store->mbSFlag_add_offset);
		    fprintf(stderr,"dbg2       mbSFlag_scale_factor:		%d\n", store->mbSFlag_scale_factor);
		    fprintf(stderr,"dbg2       mbSFlag_minimum:		%d\n", store->mbSFlag_minimum);
		    fprintf(stderr,"dbg2       mbSFlag_maximum:		%d\n", store->mbSFlag_maximum);
		    fprintf(stderr,"dbg2       mbSFlag_valid_minimum:		%d\n", store->mbSFlag_valid_minimum);
		    fprintf(stderr,"dbg2       mbSFlag_valid_maximum:		%d\n", store->mbSFlag_valid_maximum);
		    fprintf(stderr,"dbg2       mbSFlag_missing_value:		%d\n", store->mbSFlag_missing_value);
		    fprintf(stderr,"dbg2       mbSFlag_format_C:		%s\n", store->mbSFlag_format_C);
		    fprintf(stderr,"dbg2       mbSFlag_orientation:		%s\n", store->mbSFlag_orientation);
		    fprintf(stderr,"dbg2       mbAntenna_type:		%s\n", store->mbAntenna_type);
		    fprintf(stderr,"dbg2       mbAntenna_long_name:		%s\n", store->mbAntenna_long_name);
		    fprintf(stderr,"dbg2       mbAntenna_name_code:		%s\n", store->mbAntenna_name_code);
		    fprintf(stderr,"dbg2       mbAntenna_units:		%s\n", store->mbAntenna_units);
		    fprintf(stderr,"dbg2       mbAntenna_unit_code:		%s\n", store->mbAntenna_unit_code);
		    fprintf(stderr,"dbg2       mbAntenna_add_offset:		%d\n", store->mbAntenna_add_offset);
		    fprintf(stderr,"dbg2       mbAntenna_scale_factor:		%d\n", store->mbAntenna_scale_factor);
		    fprintf(stderr,"dbg2       mbAntenna_minimum:		%d\n", store->mbAntenna_minimum);
		    fprintf(stderr,"dbg2       mbAntenna_maximum:		%d\n", store->mbAntenna_maximum);
		    fprintf(stderr,"dbg2       mbAntenna_valid_minimum:		%d\n", store->mbAntenna_valid_minimum);
		    fprintf(stderr,"dbg2       mbAntenna_valid_maximum:		%d\n", store->mbAntenna_valid_maximum);
		    fprintf(stderr,"dbg2       mbAntenna_missing_value:		%d\n", store->mbAntenna_missing_value);
		    fprintf(stderr,"dbg2       mbAntenna_format_C:		%s\n", store->mbAntenna_format_C);
		    fprintf(stderr,"dbg2       mbAntenna_orientation:		%s\n", store->mbAntenna_orientation);
		    fprintf(stderr,"dbg2       mbBeamBias_type:		%s\n", store->mbBeamBias_type);
		    fprintf(stderr,"dbg2       mbBeamBias_long_name:		%s\n", store->mbBeamBias_long_name);
		    fprintf(stderr,"dbg2       mbBeamBias_name_code:		%s\n", store->mbBeamBias_name_code);
		    fprintf(stderr,"dbg2       mbBeamBias_units:		%s\n", store->mbBeamBias_units);
		    fprintf(stderr,"dbg2       mbBeamBias_unit_code:		%s\n", store->mbBeamBias_unit_code);
		    fprintf(stderr,"dbg2       mbBeamBias_add_offset:		%f\n", store->mbBeamBias_add_offset);
		    fprintf(stderr,"dbg2       mbBeamBias_scale_factor:		%f\n", store->mbBeamBias_scale_factor);
		    fprintf(stderr,"dbg2       mbBeamBias_minimum:		%d\n", store->mbBeamBias_minimum);
		    fprintf(stderr,"dbg2       mbBeamBias_maximum:		%d\n", store->mbBeamBias_maximum);
		    fprintf(stderr,"dbg2       mbBeamBias_valid_minimum:		%d\n", store->mbBeamBias_valid_minimum);
		    fprintf(stderr,"dbg2       mbBeamBias_valid_maximum:		%d\n", store->mbBeamBias_valid_maximum);
		    fprintf(stderr,"dbg2       mbBeamBias_missing_value:		%d\n", store->mbBeamBias_missing_value);
		    fprintf(stderr,"dbg2       mbBeamBias_format_C:		%s\n", store->mbBeamBias_format_C);
		    fprintf(stderr,"dbg2       mbBeamBias_orientation:		%s\n", store->mbBeamBias_orientation);
		    fprintf(stderr,"dbg2       mbBFlag_type:		%s\n", store->mbBFlag_type);
		    fprintf(stderr,"dbg2       mbBFlag_long_name:		%s\n", store->mbBFlag_long_name);
		    fprintf(stderr,"dbg2       mbBFlag_name_code:		%s\n", store->mbBFlag_name_code);
		    fprintf(stderr,"dbg2       mbBFlag_units:		%s\n", store->mbBFlag_units);
		    fprintf(stderr,"dbg2       mbBFlag_unit_code:		%s\n", store->mbBFlag_unit_code);
		    fprintf(stderr,"dbg2       mbBFlag_add_offset:		%d\n", store->mbBFlag_add_offset);
		    fprintf(stderr,"dbg2       mbBFlag_scale_factor:		%d\n", store->mbBFlag_scale_factor);
		    fprintf(stderr,"dbg2       mbBFlag_minimum:		%d\n", store->mbBFlag_minimum);
		    fprintf(stderr,"dbg2       mbBFlag_maximum:		%d\n", store->mbBFlag_maximum);
		    fprintf(stderr,"dbg2       mbBFlag_valid_minimum:		%d\n", store->mbBFlag_valid_minimum);
		    fprintf(stderr,"dbg2       mbBFlag_valid_maximum:		%d\n", store->mbBFlag_valid_maximum);
		    fprintf(stderr,"dbg2       mbBFlag_missing_value:		%d\n", store->mbBFlag_missing_value);
		    fprintf(stderr,"dbg2       mbBFlag_format_C:		%s\n", store->mbBFlag_format_C);
		    fprintf(stderr,"dbg2       mbBFlag_orientation:		%s\n", store->mbBFlag_orientation);
		    fprintf(stderr,"dbg2       mbBeam_type:		%s\n", store->mbBeam_type);
		    fprintf(stderr,"dbg2       mbBeam_long_name:		%s\n", store->mbBeam_long_name);
		    fprintf(stderr,"dbg2       mbBeam_name_code:		%s\n", store->mbBeam_name_code);
		    fprintf(stderr,"dbg2       mbBeam_units:		%s\n", store->mbBeam_units);
		    fprintf(stderr,"dbg2       mbBeam_unit_code:		%s\n", store->mbBeam_unit_code);
		    fprintf(stderr,"dbg2       mbBeam_add_offset:		%d\n", store->mbBeam_add_offset);
		    fprintf(stderr,"dbg2       mbBeam_scale_factor:		%d\n", store->mbBeam_scale_factor);
		    fprintf(stderr,"dbg2       mbBeam_minimum:		%d\n", store->mbBeam_minimum);
		    fprintf(stderr,"dbg2       mbBeam_maximum:		%d\n", store->mbBeam_maximum);
		    fprintf(stderr,"dbg2       mbBeam_valid_minimum:		%d\n", store->mbBeam_valid_minimum);
		    fprintf(stderr,"dbg2       mbBeam_valid_maximum:		%d\n", store->mbBeam_valid_maximum);
		    fprintf(stderr,"dbg2       mbBeam_missing_value:		%d\n", store->mbBeam_missing_value);
		    fprintf(stderr,"dbg2       mbBeam_format_C:		%s\n", store->mbBeam_format_C);
		    fprintf(stderr,"dbg2       mbBeam_orientation:		%s\n", store->mbBeam_orientation);
		    fprintf(stderr,"dbg2       mbAFlag_type:		%s\n", store->mbAFlag_type);
		    fprintf(stderr,"dbg2       mbAFlag_long_name:		%s\n", store->mbAFlag_long_name);
		    fprintf(stderr,"dbg2       mbAFlag_name_code:		%s\n", store->mbAFlag_name_code);
		    fprintf(stderr,"dbg2       mbAFlag_units:		%s\n", store->mbAFlag_units);
		    fprintf(stderr,"dbg2       mbAFlag_unit_code:		%s\n", store->mbAFlag_unit_code);
		    fprintf(stderr,"dbg2       mbAFlag_add_offset:		%d\n", store->mbAFlag_add_offset);
		    fprintf(stderr,"dbg2       mbAFlag_scale_factor:		%d\n", store->mbAFlag_scale_factor);
		    fprintf(stderr,"dbg2       mbAFlag_minimum:		%d\n", store->mbAFlag_minimum);
		    fprintf(stderr,"dbg2       mbAFlag_maximum:		%d\n", store->mbAFlag_maximum);
		    fprintf(stderr,"dbg2       mbAFlag_valid_minimum:		%d\n", store->mbAFlag_valid_minimum);
		    fprintf(stderr,"dbg2       mbAFlag_valid_maximum:		%d\n", store->mbAFlag_valid_maximum);
		    fprintf(stderr,"dbg2       mbAFlag_missing_value:		%d\n", store->mbAFlag_missing_value);
		    fprintf(stderr,"dbg2       mbAFlag_format_C:		%s\n", store->mbAFlag_format_C);
		    fprintf(stderr,"dbg2       mbAFlag_orientation:		%s\n", store->mbAFlag_orientation);
		    fprintf(stderr,"dbg2       mbVelProfilRef_type:		%s\n", store->mbVelProfilRef_type);
		    fprintf(stderr,"dbg2       mbVelProfilRef_long_name:		%s\n", store->mbVelProfilRef_long_name);
		    fprintf(stderr,"dbg2       mbVelProfilRef_name_code:		%s\n", store->mbVelProfilRef_name_code);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_type:		%s\n", store->mbVelProfilIdx_type);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_long_name:		%s\n", store->mbVelProfilIdx_long_name);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_name_code:		%s\n", store->mbVelProfilIdx_name_code);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_units:		%s\n", store->mbVelProfilIdx_units);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_unit_code:		%s\n", store->mbVelProfilIdx_unit_code);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_add_offset:		%d\n", store->mbVelProfilIdx_add_offset);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_scale_factor:		%d\n", store->mbVelProfilIdx_scale_factor);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_minimum:		%d\n", store->mbVelProfilIdx_minimum);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_maximum:		%d\n", store->mbVelProfilIdx_maximum);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_valid_minimum:		%d\n", store->mbVelProfilIdx_valid_minimum);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_valid_maximum:		%d\n", store->mbVelProfilIdx_valid_maximum);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_missing_value:		%d\n", store->mbVelProfilIdx_missing_value);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_format_C:		%s\n", store->mbVelProfilIdx_format_C);
		    fprintf(stderr,"dbg2       mbVelProfilIdx_orientation:		%s\n", store->mbVelProfilIdx_orientation);
		    fprintf(stderr,"dbg2       mbVelProfilDate_type:		%s\n", store->mbVelProfilDate_type);
		    fprintf(stderr,"dbg2       mbVelProfilDate_long_name:		%s\n", store->mbVelProfilDate_long_name);
		    fprintf(stderr,"dbg2       mbVelProfilDate_name_code:		%s\n", store->mbVelProfilDate_name_code);
		    fprintf(stderr,"dbg2       mbVelProfilDate_units:		%s\n", store->mbVelProfilDate_units);
		    fprintf(stderr,"dbg2       mbVelProfilDate_unit_code:		%s\n", store->mbVelProfilDate_unit_code);
		    fprintf(stderr,"dbg2       mbVelProfilDate_add_offset:		%d\n", store->mbVelProfilDate_add_offset);
		    fprintf(stderr,"dbg2       mbVelProfilDate_scale_factor:		%d\n", store->mbVelProfilDate_scale_factor);
		    fprintf(stderr,"dbg2       mbVelProfilDate_minimum:		%d\n", store->mbVelProfilDate_minimum);
		    fprintf(stderr,"dbg2       mbVelProfilDate_maximum:		%d\n", store->mbVelProfilDate_maximum);
		    fprintf(stderr,"dbg2       mbVelProfilDate_valid_minimum:		%d\n", store->mbVelProfilDate_valid_minimum);
		    fprintf(stderr,"dbg2       mbVelProfilDate_valid_maximum:		%d\n", store->mbVelProfilDate_valid_maximum);
		    fprintf(stderr,"dbg2       mbVelProfilDate_missing_value:		%d\n", store->mbVelProfilDate_missing_value);
		    fprintf(stderr,"dbg2       mbVelProfilDate_format_C:		%s\n", store->mbVelProfilDate_format_C);
		    fprintf(stderr,"dbg2       mbVelProfilDate_orientation:		%s\n", store->mbVelProfilDate_orientation);
		    fprintf(stderr,"dbg2       mbVelProfilTime_type:		%s\n", store->mbVelProfilTime_type);
		    fprintf(stderr,"dbg2       mbVelProfilTime_long_name:		%s\n", store->mbVelProfilTime_long_name);
		    fprintf(stderr,"dbg2       mbVelProfilTime_name_code:		%s\n", store->mbVelProfilTime_name_code);
		    fprintf(stderr,"dbg2       mbVelProfilTime_units:		%s\n", store->mbVelProfilTime_units);
		    fprintf(stderr,"dbg2       mbVelProfilTime_unit_code:		%s\n", store->mbVelProfilTime_unit_code);
		    fprintf(stderr,"dbg2       mbVelProfilTime_add_offset:		%d\n", store->mbVelProfilTime_add_offset);
		    fprintf(stderr,"dbg2       mbVelProfilTime_scale_factor:		%d\n", store->mbVelProfilTime_scale_factor);
		    fprintf(stderr,"dbg2       mbVelProfilTime_minimum:		%d\n", store->mbVelProfilTime_minimum);
		    fprintf(stderr,"dbg2       mbVelProfilTime_maximum:		%d\n", store->mbVelProfilTime_maximum);
		    fprintf(stderr,"dbg2       mbVelProfilTime_valid_minimum:		%d\n", store->mbVelProfilTime_valid_minimum);
		    fprintf(stderr,"dbg2       mbVelProfilTime_valid_maximum:		%d\n", store->mbVelProfilTime_valid_maximum);
		    fprintf(stderr,"dbg2       mbVelProfilTime_missing_value:		%d\n", store->mbVelProfilTime_missing_value);
		    fprintf(stderr,"dbg2       mbVelProfilTime_format_C:		%s\n", store->mbVelProfilTime_format_C);
		    fprintf(stderr,"dbg2       mbVelProfilTime_orientation:		%s\n", store->mbVelProfilTime_orientation);
		    }
		}

	    /* read global variables */
	    if (status == MB_SUCCESS)
		{
		if (store->mbHistDate_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbNbrHistoryRec;
		    nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, index, count, store->mbHistDate);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbHistDate error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistTime_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbNbrHistoryRec;
		    nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, index, count, store->mbHistTime);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbHistTime error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistCode_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbNbrHistoryRec;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, index, count, store->mbHistCode);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbHistCode error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistAutor_id >= 0)
		    {
		    index[0] = 0;
		    index[1] = 0;
		    count[0] = store->mbNbrHistoryRec;
		    count[1] = store->mbNameLength;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, index, count, store->mbHistAutor);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbHistAutor error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistModule_id >= 0)
		    {
		    index[0] = 0;
		    index[1] = 0;
		    count[0] = store->mbNbrHistoryRec;
		    count[1] = store->mbNameLength;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, index, count, store->mbHistModule);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbHistModule error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbHistComment_id >= 0)
		    {
		    index[0] = 0;
		    index[1] = 0;
		    count[0] = store->mbNbrHistoryRec;
		    count[1] = store->mbCommentLength;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, index, count, store->mbHistComment);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbHistComment error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbAntenna_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbBeamNbr;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, index, count, store->mbAntenna);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbAntenna error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbBeamBias_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbBeamNbr;
		    nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbBeamBias_id, index, count, store->mbBeamBias);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbBeamBias error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbBFlag_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbBeamNbr;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, index, count, store->mbBFlag);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbBFlag error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbBeam_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbAntennaNbr;
		    nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbBeam_id, index, count, store->mbBeam);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbBeam error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbAFlag_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbAntennaNbr;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, index, count, store->mbAFlag);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbAFlag error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilRef_id >= 0)
		    {
		    index[0] = 0;
		    index[1] = 0;
		    count[0] = store->mbVelocityProfilNbr;
		    count[1] = store->mbCommentLength;
		    nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, index, count, store->mbVelProfilRef);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbVelProfilRef error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilIdx_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbVelocityProfilNbr;
		    nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, index, count, store->mbVelProfilIdx);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbVelProfilIdx error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilDate_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbVelocityProfilNbr;
		    nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, index, count, store->mbVelProfilDate);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbVelProfilDate error: %s\n", nc_strerror(nc_status));
		    }
		if (store->mbVelProfilTime_id >= 0)
		    {
		    index[0] = 0;
		    count[0] = store->mbVelocityProfilNbr;
		    nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, index, count, store->mbVelProfilTime);
		    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbVelProfilTime error: %s\n", nc_strerror(nc_status));
		    }
		if (nc_status != NC_NOERR)
		    {
		    status = MB_FAILURE;
		    *error = MB_ERROR_EOF;
		    }		
		
		/* print input debug statements */
		if (verbose >= 2)
		    {
		    fprintf(stderr,"\ndbg2  NetCDF Global Variables read in function <%s>\n",
			    function_name);
		    fprintf(stderr,"dbg2  Global Variables:\n");
		    fprintf(stderr,"dbg2       status:                  %d\n", status);
		    fprintf(stderr,"dbg2       error:                   %d\n", *error);
		    fprintf(stderr,"dbg2       nc_status:               %d\n", nc_status);
		    fprintf(stderr,"dbg2       mbNbrHistoryRec:         %d\n", store->mbNbrHistoryRec);
		    for (i=0;i<store->mbNbrHistoryRec;i++)
			{
			fprintf(stderr,"dbg2       mbHistDate[%2d]:          %d\n", i, store->mbHistDate[i]);
			fprintf(stderr,"dbg2       mbHistTime[%2d]:          %d\n", i, store->mbHistTime[i]);
			fprintf(stderr,"dbg2       mbHistCode[%2d]:          %d\n", i, store->mbHistCode[i]);
			fprintf(stderr,"dbg2       mbHistAutor[%2d]:         %s\n", i, &(store->mbHistAutor[i*store->mbNameLength]));
			fprintf(stderr,"dbg2       mbHistModule[%2d]:        %s\n", i, &(store->mbHistModule[i*store->mbNameLength]));
			fprintf(stderr,"dbg2       mbHistComment[%2d]:       %s\n", i, &(store->mbHistComment[i*store->mbCommentLength]));
			}
		    fprintf(stderr,"dbg2       mbAntennaNbr:              %d\n", store->mbAntennaNbr);
		    fprintf(stderr,"dbg2       Antenna mbBeam mbAFlag\n");
		    for (i=0;i<store->mbAntennaNbr;i++)
			{
			fprintf(stderr,"dbg2       %d %d %d\n", 
				i, store->mbBeam[i], store->mbAFlag[i]);
			}
		    fprintf(stderr,"dbg2       mbBeamNbr:              %d\n", store->mbBeamNbr);
		    fprintf(stderr,"dbg2       beam mbAntenna mbBeamBias mbBFlag\n");
		    for (i=0;i<store->mbBeamNbr;i++)
			{
			fprintf(stderr,"dbg2       %3d %d %d %d\n", 
				i, store->mbAntenna[i], store->mbBeamBias[i], 
				store->mbBFlag[i]);
			}
		    fprintf(stderr,"dbg2       mbVelocityProfilNbr:    %d\n", store->mbVelocityProfilNbr);
		    for (i=0;i<store->mbVelocityProfilNbr;i++)
			{
			fprintf(stderr,"dbg2       mbVelProfilRef[%2d]:      %s\n", i, &(store->mbVelProfilRef[i*store->mbCommentLength]));
			fprintf(stderr,"dbg2       mbVelProfilIdx[%2d]:      %d\n", i, store->mbVelProfilIdx[i]);
			fprintf(stderr,"dbg2       mbVelProfilDate[%2d]:     %d\n", i, store->mbVelProfilDate[i]);
			fprintf(stderr,"dbg2       mbVelProfilTime[%2d]:     %d\n", i, store->mbVelProfilTime[i]);
			}
		    }
		}
	    }

	/* read next data from file */
	/* first run through all comment records */
	if (status == MB_SUCCESS && store->mbNbrHistoryRec > *commentread)
	    {
	    store->kind = MB_DATA_COMMENT;
	    
	    /* get next comment */
	    strncpy(store->comment, 
		    &(store->mbHistComment[(*commentread) *store->mbCommentLength]), 
		    MBSYS_NETCDF_COMMENTLEN);
	    
	    /* set counters */
	    (*commentread)++;
	    (*dataread)++;
	    
	    /* print input debug statements */
	    if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  Comment read in function <%s>\n",
			function_name);
		fprintf(stderr,"dbg2  Comment:\n");
		fprintf(stderr,"dbg2       status:                  %d\n", status);
		fprintf(stderr,"dbg2       error:                   %d\n", *error);
		fprintf(stderr,"dbg2       comment:                 %s\n", store->comment);
		}
	    }
	
	/* next run through all survey records */
	else if (status == MB_SUCCESS && store->mbCycleNbr > *recread)
	    {
	    /* set kind */
	    store->kind = MB_DATA_DATA;

	    /* read the variables from next record */
	    if (store->mbCycle_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbCycle_id, index, count, store->mbCycle);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbCycle error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbDate_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbDate_id, index, count, store->mbDate);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbDate error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbTime_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbTime_id, index, count, store->mbTime);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbTime error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbOrdinate_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, index, count, store->mbOrdinate);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbOrdinate error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbAbscissa_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, index, count, store->mbAbscissa);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbAbscissa error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbFrequency_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, index, count, store->mbFrequency);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbFrequency error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbSounderMode_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, index, count, store->mbSounderMode);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbSounderMode error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbReferenceDepth_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, index, count, store->mbReferenceDepth);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbReferenceDepth error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbDynamicDraught_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, index, count, store->mbDynamicDraught);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbDynamicDraught error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbTide_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbTide_id, index, count, store->mbTide);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbTide error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbSoundVelocity_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, index, count, store->mbSoundVelocity);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbSoundVelocity error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbHeading_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbHeading_id, index, count, store->mbHeading);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbHeading error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbRoll_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbRoll_id, index, count, store->mbRoll);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbRoll error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbPitch_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbPitch_id, index, count, store->mbPitch);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbPitch error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbTransmissionHeave_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, index, count, store->mbTransmissionHeave);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbTransmissionHeave error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbDistanceScale_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, index, count, store->mbDistanceScale);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbDistanceScale error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbDepthScale_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, index, count, store->mbDepthScale);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbDepthScale error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbVerticalDepth_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, index, count, store->mbVerticalDepth);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbVerticalDepth error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbCQuality_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, index, count, store->mbCQuality);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbCQuality error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbCFlag_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, index, count, store->mbCFlag);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbCFlag error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbInterlacing_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, index, count, store->mbInterlacing);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbInterlacing error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbSamplingRate_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbAntennaNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, index, count, store->mbSamplingRate);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbSamplingRate error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbAlongDistance_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbBeamNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, index, count, store->mbAlongDistance);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbAlongDistance error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbAcrossDistance_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbBeamNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, index, count, store->mbAcrossDistance);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbAcrossDistance error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbDepth_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbBeamNbr;
		nc_status = nc_get_vara_short((int)mb_io_ptr->mbfp, store->mbDepth_id, index, count, store->mbDepth);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbDepth error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbSQuality_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbBeamNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, index, count, store->mbSQuality);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbSQuality error: %s\n", nc_strerror(nc_status));
		}
	    if (store->mbSFlag_id >= 0)
		{
		index[0] = *recread;
		index[1] = 0;
		count[0] = 1;
		count[1] = store->mbBeamNbr;
		nc_status = nc_get_vara_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, index, count, store->mbSFlag);
		if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_get_vara mbSFlag error: %s\n", nc_strerror(nc_status));
		}
	    /* check status */
	    if (nc_status != NC_NOERR)
		{
		status = MB_FAILURE;
		*error = MB_ERROR_EOF;
		}		
	    
	    /* set counters */
	    (*recread)++;
	    (*dataread)++;
		
	    /* print input debug statements */
	    if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  NetCDF Survey Record read in function <%s>\n",
			function_name);
		fprintf(stderr,"dbg2  Global Variables:\n");
		fprintf(stderr,"dbg2       status:                  %d\n", status);
		fprintf(stderr,"dbg2       error:                   %d\n", *error);
		fprintf(stderr,"dbg2       nc_status:               %d\n", nc_status);
		fprintf(stderr,"dbg2       mbCycle:                 ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbCycle[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbDate:                  ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbDate[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbTime:                  ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbTime[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbOrdinate:              ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbOrdinate[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbAbscissa:              ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbAbscissa[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbFrequency:             ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbFrequency[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbSounderMode:           ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbSounderMode[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbReferenceDepth:        ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbReferenceDepth[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbDynamicDraught:        ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbDynamicDraught[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbTide:                  ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbTide[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbSoundVelocity:         ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbSoundVelocity[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbHeading:               ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbHeading[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbRoll:                  ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbRoll[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbPitch:                 ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbPitch[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbTransmissionHeave:     ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbTransmissionHeave[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbDistanceScale:         ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbDistanceScale[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbDepthScale:            ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbDepthScale[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbVerticalDepth:         ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbVerticalDepth[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbCQuality:              ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbCQuality[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbCFlag:                 ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbCFlag[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbInterlacing:           ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbInterlacing[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbSamplingRate:          ");
		for (i=0;i<store->mbAntennaNbr;i++)
			fprintf(stderr, "%d ", store->mbSamplingRate[i]);
		fprintf(stderr, "\n");
		fprintf(stderr,"dbg2       mbBeamNbr:               %d\n", store->mbBeamNbr);
		fprintf(stderr,"dbg2       beam ltrack xtrack depth quality flag\n");
		for (i=0;i<store->mbBeamNbr;i++)
		    {
		    fprintf(stderr,"dbg2       %3d %8d %8d %8d %d %d\n", 
				    i, store->mbAlongDistance[i], 
				    store->mbAcrossDistance[i], store->mbDepth[i], 
				    store->mbSQuality[i], store->mbSFlag[i]);
		    }
		}
	    }
	
	/* else end of file */
	else
	    {
	    /* set kind */
	    store->kind = MB_DATA_NONE;
	    
	    /* set flags */
	    *error = MB_ERROR_EOF;
	    status = MB_FAILURE;
	    }

	/* set error and kind in mb_io_ptr */
	mb_io_ptr->new_error = *error;
	mb_io_ptr->new_kind = store->kind;

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       error:      %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:  %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/
int mbr_wt_mbnetcdf(int verbose, void *mbio_ptr, void *store_ptr, int *error)
{
	char	*function_name = "mbr_wt_mbnetcdf";
	int	status = MB_SUCCESS;
	struct mb_io_struct *mb_io_ptr;
	struct mbsys_netcdf_struct *store;
	int	*datawrite;
	int	*commentwrite;
	int	*svpwrite;
	int	*recwrite;
	int 	nc_status;
	int 	done;
	int	mbHistoryRecNbr_id;
	int	mbNameLength_id;
	int	mbCommentLength_id;
	int	mbAntennaNbr_id;
	int	mbBeamNbr_id;
	int	mbCycleNbr_id;
	int	mbVelocityProfilNbr_id;
	int	dims[2];
	size_t	index[2], count[2];

	/* print input debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> called\n",
			function_name);
		fprintf(stderr,"dbg2  Input arguments:\n");
		fprintf(stderr,"dbg2       verbose:    %d\n",verbose);
		fprintf(stderr,"dbg2       mbio_ptr:   %d\n",mbio_ptr);
		fprintf(stderr,"dbg2       store_ptr:  %d\n",store_ptr);
		}

	/* get pointer to mbio descriptor and data storage */
	mb_io_ptr = (struct mb_io_struct *) mbio_ptr;
	store = (struct mbsys_netcdf_struct *) store_ptr;
	datawrite = (int *) &mb_io_ptr->save1;
	commentwrite = (int *) &mb_io_ptr->save2;
	svpwrite = (int *) &mb_io_ptr->save3;
	recwrite = (int *) &mb_io_ptr->save4;

	/* if comment and nothing written yet save it */
	if (store->kind == MB_DATA_COMMENT
	    && *recwrite == 0
	    && store->mbNbrHistoryRec < store->mbHistoryRecNbr)
	    {
	    
	    /* get next comment */
	    strncpy(&(store->mbHistComment[(*commentwrite) *store->mbCommentLength]), 
		    store->comment,
		    MBSYS_NETCDF_COMMENTLEN);
	    store->mbHistCode[*commentwrite] = 1;
	    store->mbNbrHistoryRec++;
	    
	    /* set counters */
	    (*commentwrite)++;
	    (*datawrite)++;
	    
	    /* print input debug statements */
	    if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  Comment saved in function <%s>\n",
			function_name);
		fprintf(stderr,"dbg2  Comment:\n");
		fprintf(stderr,"dbg2       status:                  %d\n", status);
		fprintf(stderr,"dbg2       error:                   %d\n", *error);
		fprintf(stderr,"dbg2       comment:                 %s\n", store->comment);
		}
	    }
	
	/* if data and nothing written yet start it off */
	if (store->kind == MB_DATA_DATA
	    && *recwrite == 0
	    && status == MB_SUCCESS)
	    {
	    /* define the dimensions */
   	    nc_status = nc_def_dim((int)mb_io_ptr->mbfp, "mbHistoryRecNbr", store->mbHistoryRecNbr, &mbHistoryRecNbr_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_dim mbHistoryRecNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_def_dim((int)mb_io_ptr->mbfp, "mbNameLength", store->mbNameLength, &mbNameLength_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_dim mbNameLength error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_def_dim((int)mb_io_ptr->mbfp, "mbCommentLength", store->mbCommentLength, &mbCommentLength_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_dim mbCommentLength error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_def_dim((int)mb_io_ptr->mbfp, "mbAntennaNbr", store->mbAntennaNbr, &mbAntennaNbr_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_dim mbAntennaNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_def_dim((int)mb_io_ptr->mbfp, "mbBeamNbr", store->mbBeamNbr, &mbBeamNbr_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_dim mbBeamNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_def_dim((int)mb_io_ptr->mbfp, "mbCycleNbr", NC_UNLIMITED, &mbCycleNbr_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_dim mbCycleNbr error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_def_dim((int)mb_io_ptr->mbfp, "mbVelocityProfilNbr", store->mbVelocityProfilNbr, &mbVelocityProfilNbr_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_dim mbVelocityProfilNbr error: %s\n", nc_strerror(nc_status));
	    
	    /* define the variables */
	    dims[0] = mbHistoryRecNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbHistDate", NC_INT, 1, dims, &store->mbHistDate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbHistDate_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbHistoryRecNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbHistTime", NC_INT, 1, dims, &store->mbHistTime_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbHistTime_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbHistoryRecNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbHistCode", NC_CHAR, 1, dims, &store->mbHistCode_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbHistCode_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbHistoryRecNbr_id;
	    dims[1] = mbNameLength_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbHistAutor", NC_CHAR, 2, dims, &store->mbHistAutor_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbHistAutor_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbHistoryRecNbr_id;
	    dims[1] = mbNameLength_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbHistModule", NC_CHAR, 2, dims, &store->mbHistModule_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbHistModule_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbHistoryRecNbr_id;
	    dims[1] = mbCommentLength_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbHistComment", NC_CHAR, 2, dims, &store->mbHistComment_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbHistComment_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbCycle", NC_SHORT, 2, dims, &store->mbCycle_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbCycle_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbDate", NC_INT, 2, dims, &store->mbDate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbDate_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbTime", NC_INT, 2, dims, &store->mbTime_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbTime_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbOrdinate", NC_INT, 2, dims, &store->mbOrdinate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbOrdinate_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbAbscissa", NC_INT, 2, dims, &store->mbAbscissa_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbAbscissa_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbFrequency", NC_CHAR, 2, dims, &store->mbFrequency_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbFrequency_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbSounderMode", NC_CHAR, 2, dims, &store->mbSounderMode_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbSounderMode_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbReferenceDepth", NC_SHORT, 2, dims, &store->mbReferenceDepth_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbReferenceDepth_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbDynamicDraught", NC_SHORT, 2, dims, &store->mbDynamicDraught_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbDynamicDraught_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbTide", NC_SHORT, 2, dims, &store->mbTide_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbTide_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbSoundVelocity", NC_SHORT, 2, dims, &store->mbSoundVelocity_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbSoundVelocity_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbHeading", NC_SHORT, 2, dims, &store->mbHeading_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbHeading_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbRoll", NC_SHORT, 2, dims, &store->mbRoll_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbRoll_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbPitch", NC_SHORT, 2, dims, &store->mbPitch_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbPitch_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbTransmissionHeave", NC_SHORT, 2, dims, &store->mbTransmissionHeave_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbTransmissionHeave_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbDistanceScale", NC_CHAR, 2, dims, &store->mbDistanceScale_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbDistanceScale_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbDepthScale", NC_CHAR, 2, dims, &store->mbDepthScale_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbDepthScale_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbVerticalDepth", NC_SHORT, 2, dims, &store->mbVerticalDepth_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbVerticalDepth_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbCQuality", NC_CHAR, 2, dims, &store->mbCQuality_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbCQuality_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbCFlag", NC_CHAR, 2, dims, &store->mbCFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbCFlag_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbInterlacing", NC_CHAR, 2, dims, &store->mbInterlacing_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbInterlacing_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbSamplingRate", NC_SHORT, 2, dims, &store->mbSamplingRate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbSamplingRate_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbAlongDistance", NC_SHORT, 2, dims, &store->mbAlongDistance_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbAlongDistance_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbAcrossDistance", NC_SHORT, 2, dims, &store->mbAcrossDistance_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbAcrossDistance_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbDepth", NC_SHORT, 2, dims, &store->mbDepth_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbDepth_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbSQuality", NC_CHAR, 2, dims, &store->mbSQuality_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbSQuality_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbCycleNbr_id;
	    dims[1] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbSFlag", NC_CHAR, 2, dims, &store->mbSFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbSFlag_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbAntenna", NC_CHAR, 1, dims, &store->mbAntenna_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbAntenna_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbBeamBias", NC_SHORT, 1, dims, &store->mbBeamBias_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbBeamBias_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbBeamNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbBFlag", NC_CHAR, 1, dims, &store->mbBFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbBFlag_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbBeam", NC_SHORT, 1, dims, &store->mbBeam_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbBeam_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbAntennaNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbAFlag", NC_CHAR, 1, dims, &store->mbAFlag_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbAFlag_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbVelocityProfilNbr_id;
	    dims[1] = mbCommentLength_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbVelProfilRef", NC_CHAR, 2, dims, &store->mbVelProfilRef_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbVelProfilRef_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbVelocityProfilNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbVelProfilIdx", NC_SHORT, 1, dims, &store->mbVelProfilIdx_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbVelProfilIdx_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbVelocityProfilNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbVelProfilDate", NC_INT, 1, dims, &store->mbVelProfilDate_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbVelProfilDate_id error: %s\n", nc_strerror(nc_status));
	    dims[0] = mbVelocityProfilNbr_id;
	    nc_status = nc_def_var((int)mb_io_ptr->mbfp, "mbVelProfilTime", NC_INT, 1, dims, &store->mbVelProfilTime_id);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_def_var mbVelProfilTime_id error: %s\n", nc_strerror(nc_status));
	    
	    /* save the global attributes */
	    nc_status = nc_put_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbVersion", NC_SHORT, 1, &store->mbVersion);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVersion error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbName", 12, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbName error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbClasse", 8, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbClasse error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbLevel", NC_SHORT, 1, &store->mbLevel);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbLevel error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNbrHistoryRec", NC_SHORT, 1, &store->mbNbrHistoryRec);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbNbrHistoryRec error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbTimeReference", 38, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTimeReference error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbStartDate", NC_INT, 1, &store->mbStartDate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbStartDate error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbStartTime", NC_INT, 1, &store->mbStartTime);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbStartTime error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEndDate", NC_INT, 1, &store->mbEndDate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbEndDate error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEndTime", NC_INT, 1, &store->mbEndTime);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbEndTime error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNorthLatitude", NC_DOUBLE, 1, &store->mbNorthLatitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbNorthLatitude error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSouthLatitude", NC_DOUBLE, 1, &store->mbSouthLatitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSouthLatitude error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEastLongitude", NC_DOUBLE, 1, &store->mbEastLongitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbEastLongitude error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbWestLongitude", NC_DOUBLE, 1, &store->mbWestLongitude);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbWestLongitude error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbMeridian180", 1, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbMeridian180 error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbGeoDictionnary", 20, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbGeoDictionnary error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbGeoRepresentation", 20, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbGeoRepresentation error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbGeodesicSystem", 20, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbGeodesicSystem error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidName", 256, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbEllipsoidName error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidA", NC_DOUBLE, 1, &store->mbEllipsoidA);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbEllipsoidA error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidInvF", NC_DOUBLE, 1, &store->mbEllipsoidInvF);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbEllipsoidInvF error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbEllipsoidE2", NC_DOUBLE, 1, &store->mbEllipsoidE2);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbEllipsoidE2 error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbProjType", NC_SHORT, 1, &store->mbProjType);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbProjType error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbProjParameterValue", NC_DOUBLE, 10, store->mbProjParameterValue);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbProjParameterValue error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbProjParameterCode", 256, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbProjParameterCode error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSounder", NC_SHORT, 1, &store->mbSounder);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounder error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbShip", 256, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbShip error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSurvey", 256, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSurvey error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbReference", 256, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReference error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbAntennaOffset", NC_DOUBLE, 3, store->mbAntennaOffset);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntennaOffset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbAntennaDelay", NC_DOUBLE, 1, &store->mbAntennaDelay);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntennaDelay error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSounderOffset", NC_DOUBLE, 3, store->mbSounderOffset);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderOffset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbSounderDelay", NC_DOUBLE, 1, &store->mbSounderDelay);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderDelay error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbVRUOffset", NC_DOUBLE, 3, store->mbVRUOffset);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVRUOffset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbVRUDelay", NC_DOUBLE, 1, &store->mbVRUDelay);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVRUDelay error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbHeadingBias", NC_DOUBLE, 1, &store->mbHeadingBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeadingBias error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbRollBias", NC_DOUBLE, 1, &store->mbRollBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRollBias error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbPitchBias", NC_DOUBLE, 1, &store->mbPitchBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitchBias error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbHeaveBias", NC_DOUBLE, 1, &store->mbHeaveBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeaveBias error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbDraught", NC_DOUBLE, 1, &store->mbDraught);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDraught error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNavType", NC_SHORT, 1, &store->mbNavType);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbNavType error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbNavRef", 256, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbNavRef error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_short((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbTideType", NC_SHORT, 1, &store->mbTideType);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTideType error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbTideRef", 256, store->mbTideRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTideRef error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbMinDepth", NC_DOUBLE, 1, &store->mbMinDepth);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbMinDepth error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbMaxDepth", NC_DOUBLE, 1, &store->mbMaxDepth);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbMaxDepth error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, NC_GLOBAL, "mbCycleCounter", NC_INT, 1, &store->mbCycleCounter);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVersion error: %s\n", nc_strerror(nc_status));

	    
	    /* save the variable attributes */
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "type", 7, store->mbHistDate_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "long_name", 12, store->mbHistDate_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "name_code", 15, store->mbHistDate_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "units", 11, store->mbHistDate_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "unit_code", 14, store->mbHistDate_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "add_offset", NC_INT, 1, &store->mbHistDate_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistDate_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "scale_factor", NC_INT, 1, &store->mbHistDate_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistDate_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "minimum", NC_INT, 1, &store->mbHistDate_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistDate_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "maximum", NC_INT, 1, &store->mbHistDate_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistDate_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "valid_minimum", NC_INT, 1, &store->mbHistDate_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistDate_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "valid_maximum", NC_INT, 1, &store->mbHistDate_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistDate_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, "missing_value", NC_INT, 1, &store->mbHistDate_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistDate_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "format_C", 2, store->mbHistDate_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistDate_id, "orientation", 6, store->mbHistDate_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "type", 7, store->mbHistTime_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "long_name", 17, store->mbHistTime_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "name_code", 15, store->mbHistTime_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "units", 2, store->mbHistTime_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "unit_code", 5, store->mbHistTime_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "add_offset", NC_INT, 1, &store->mbHistTime_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistTime_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "scale_factor", NC_INT, 1, &store->mbHistTime_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistTime_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "minimum", NC_INT, 1, &store->mbHistTime_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistTime_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "maximum", NC_INT, 1, &store->mbHistTime_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistTime_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "valid_minimum", NC_INT, 1, &store->mbHistTime_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistTime_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "valid_maximum", NC_INT, 1, &store->mbHistTime_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistTime_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, "missing_value", NC_INT, 1, &store->mbHistTime_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistTime_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "format_C", 2, store->mbHistTime_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistTime_id, "orientation", 6, store->mbHistTime_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "type", 7, store->mbHistCode_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "long_name", 12, store->mbHistCode_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "name_code", 15, store->mbHistCode_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "units", 1, store->mbHistCode_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "unit_code", 14, store->mbHistCode_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "add_offset", NC_INT, 1, &store->mbHistCode_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistCode_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "scale_factor", NC_INT, 1, &store->mbHistCode_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistCode_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "minimum", NC_INT, 1, &store->mbHistCode_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistCode_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "maximum", NC_INT, 1, &store->mbHistCode_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistCode_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "valid_minimum", NC_INT, 1, &store->mbHistCode_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistCode_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "valid_maximum", NC_INT, 1, &store->mbHistCode_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistCode_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHistCode_id, "missing_value", NC_INT, 1, &store->mbHistCode_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHistCode_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "format_C", 2, store->mbHistCode_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, "orientation", 6, store->mbHistCode_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, "type", 6, store->mbHistAutor_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, "long_name", 13, store->mbHistAutor_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, "name_code", 16, store->mbHistAutor_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, "type", 6, store->mbHistModule_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, "long_name", 14, store->mbHistModule_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, "name_code", 17, store->mbHistModule_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, "type", 6, store->mbHistComment_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, "long_name", 15, store->mbHistComment_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, "name_code", 18, store->mbHistComment_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "type", 7, store->mbCycle_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "long_name", 12, store->mbCycle_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "name_code", 15, store->mbCycle_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "units", 1, store->mbCycle_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "unit_code", 14, store->mbCycle_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "add_offset", NC_INT, 1, &store->mbCycle_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCycle_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "scale_factor", NC_INT, 1, &store->mbCycle_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCycle_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "minimum", NC_INT, 1, &store->mbCycle_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCycle_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "maximum", NC_INT, 1, &store->mbCycle_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCycle_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "valid_minimum", NC_INT, 1, &store->mbCycle_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCycle_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "valid_maximum", NC_INT, 1, &store->mbCycle_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCycle_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCycle_id, "missing_value", NC_INT, 1, &store->mbCycle_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCycle_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "format_C", 2, store->mbCycle_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCycle_id, "orientation", 6, store->mbCycle_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "type", 7, store->mbDate_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "long_name", 13, store->mbDate_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "name_code", 13, store->mbDate_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "units", 11, store->mbDate_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "unit_code", 14, store->mbDate_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "add_offset", NC_INT, 1, &store->mbDate_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDate_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "scale_factor", NC_INT, 1, &store->mbDate_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDate_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "minimum", NC_INT, 1, &store->mbDate_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDate_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "maximum", NC_INT, 1, &store->mbDate_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDate_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "valid_minimum", NC_INT, 1, &store->mbDate_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDate_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "valid_maximum", NC_INT, 1, &store->mbDate_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDate_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDate_id, "missing_value", NC_INT, 1, &store->mbDate_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDate_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "format_C", 2, store->mbDate_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDate_id, "orientation", 6, store->mbDate_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "type", 7, store->mbTime_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "long_name", 13, store->mbTime_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "name_code", 13, store->mbTime_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "units", 2, store->mbTime_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "unit_code", 5, store->mbTime_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "add_offset", NC_INT, 1, &store->mbTime_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTime_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "scale_factor", NC_INT, 1, &store->mbTime_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTime_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "minimum", NC_INT, 1, &store->mbTime_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTime_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "maximum", NC_INT, 1, &store->mbTime_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTime_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "valid_minimum", NC_INT, 1, &store->mbTime_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTime_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "valid_maximum", NC_INT, 1, &store->mbTime_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTime_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTime_id, "missing_value", NC_INT, 1, &store->mbTime_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTime_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "format_C", 2, store->mbTime_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTime_id, "orientation", 6, store->mbTime_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "type", 4, store->mbOrdinate_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "long_name", 1, store->mbOrdinate_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "name_code", 10, store->mbOrdinate_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "units", 1, store->mbOrdinate_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "unit_code", 4, store->mbOrdinate_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "add_offset", NC_DOUBLE, 1, &store->mbOrdinate_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbOrdinate_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "scale_factor", NC_DOUBLE, 1, &store->mbOrdinate_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbOrdinate_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "minimum", NC_INT, 1, &store->mbOrdinate_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbOrdinate_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "maximum", NC_INT, 1, &store->mbOrdinate_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbOrdinate_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "valid_minimum", NC_INT, 1, &store->mbOrdinate_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbOrdinate_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "valid_maximum", NC_INT, 1, &store->mbOrdinate_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbOrdinate_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "missing_value", NC_INT, 1, &store->mbOrdinate_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbOrdinate_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "format_C", 2, store->mbOrdinate_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbOrdinate_id, "orientation", 6, store->mbOrdinate_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "type", 4, store->mbAbscissa_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "long_name", 1, store->mbAbscissa_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "name_code", 10, store->mbAbscissa_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "units", 1, store->mbAbscissa_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "unit_code", 4, store->mbAbscissa_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "add_offset", NC_DOUBLE, 1, &store->mbAbscissa_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAbscissa_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "scale_factor", NC_DOUBLE, 1, &store->mbAbscissa_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAbscissa_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "minimum", NC_INT, 1, &store->mbAbscissa_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAbscissa_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "maximum", NC_INT, 1, &store->mbAbscissa_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAbscissa_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "valid_minimum", NC_INT, 1, &store->mbAbscissa_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAbscissa_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "valid_maximum", NC_INT, 1, &store->mbAbscissa_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAbscissa_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "missing_value", NC_INT, 1, &store->mbAbscissa_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAbscissa_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "format_C", 2, store->mbAbscissa_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAbscissa_id, "orientation", 6, store->mbAbscissa_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "type", 7, store->mbFrequency_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "long_name", 18, store->mbFrequency_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "name_code", 18, store->mbFrequency_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "units", 1, store->mbFrequency_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "unit_code", 14, store->mbFrequency_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "add_offset", NC_INT, 1, &store->mbFrequency_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbFrequency_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "scale_factor", NC_INT, 1, &store->mbFrequency_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbFrequency_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "minimum", NC_INT, 1, &store->mbFrequency_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbFrequency_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "maximum", NC_INT, 1, &store->mbFrequency_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbFrequency_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "valid_minimum", NC_INT, 1, &store->mbFrequency_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbFrequency_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "valid_maximum", NC_INT, 1, &store->mbFrequency_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbFrequency_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbFrequency_id, "missing_value", NC_INT, 1, &store->mbFrequency_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbFrequency_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "format_C", 2, store->mbFrequency_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, "orientation", 6, store->mbFrequency_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "type", 7, store->mbSounderMode_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "long_name", 12, store->mbSounderMode_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "name_code", 13, store->mbSounderMode_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "units", 1, store->mbSounderMode_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "unit_code", 14, store->mbSounderMode_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "add_offset", NC_INT, 1, &store->mbSounderMode_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderMode_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "scale_factor", NC_INT, 1, &store->mbSounderMode_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderMode_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "minimum", NC_INT, 1, &store->mbSounderMode_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderMode_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "maximum", NC_INT, 1, &store->mbSounderMode_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderMode_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "valid_minimum", NC_INT, 1, &store->mbSounderMode_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderMode_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "valid_maximum", NC_INT, 1, &store->mbSounderMode_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderMode_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "missing_value", NC_INT, 1, &store->mbSounderMode_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSounderMode_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "format_C", 2, store->mbSounderMode_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, "orientation", 6, store->mbSounderMode_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "type", 4, store->mbReferenceDepth_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "long_name", 28, store->mbReferenceDepth_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "name_code", 24, store->mbReferenceDepth_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "units", 1, store->mbReferenceDepth_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "unit_code", 4, store->mbReferenceDepth_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "add_offset", NC_DOUBLE, 1, &store->mbReferenceDepth_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReferenceDepth_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "scale_factor", NC_DOUBLE, 1, &store->mbReferenceDepth_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReferenceDepth_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "minimum", NC_INT, 1, &store->mbReferenceDepth_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReferenceDepth_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "maximum", NC_INT, 1, &store->mbReferenceDepth_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReferenceDepth_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "valid_minimum", NC_INT, 1, &store->mbReferenceDepth_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReferenceDepth_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "valid_maximum", NC_INT, 1, &store->mbReferenceDepth_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReferenceDepth_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "missing_value", NC_INT, 1, &store->mbReferenceDepth_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbReferenceDepth_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "format_C", 4, store->mbReferenceDepth_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, "orientation", 6, store->mbReferenceDepth_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "type", 4, store->mbDynamicDraught_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "long_name", 26, store->mbDynamicDraught_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "name_code", 16, store->mbDynamicDraught_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "units", 1, store->mbDynamicDraught_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "unit_code", 4, store->mbDynamicDraught_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "add_offset", NC_DOUBLE, 1, &store->mbDynamicDraught_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDynamicDraught_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "scale_factor", NC_DOUBLE, 1, &store->mbDynamicDraught_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDynamicDraught_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "minimum", NC_INT, 1, &store->mbDynamicDraught_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDynamicDraught_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "maximum", NC_INT, 1, &store->mbDynamicDraught_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDynamicDraught_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "valid_minimum", NC_INT, 1, &store->mbDynamicDraught_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDynamicDraught_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "valid_maximum", NC_INT, 1, &store->mbDynamicDraught_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDynamicDraught_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "missing_value", NC_INT, 1, &store->mbDynamicDraught_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDynamicDraught_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "format_C", 4, store->mbDynamicDraught_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, "orientation", 6, store->mbDynamicDraught_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "type", 4, store->mbTide_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "long_name", 15, store->mbTide_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "name_code", 13, store->mbTide_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "units", 1, store->mbTide_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "unit_code", 4, store->mbTide_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbTide_id, "add_offset", NC_DOUBLE, 1, &store->mbTide_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTide_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbTide_id, "scale_factor", NC_DOUBLE, 1, &store->mbTide_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTide_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "minimum", NC_INT, 1, &store->mbTide_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTide_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "maximum", NC_INT, 1, &store->mbTide_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTide_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "valid_minimum", NC_INT, 1, &store->mbTide_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTide_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "valid_maximum", NC_INT, 1, &store->mbTide_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTide_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTide_id, "missing_value", NC_INT, 1, &store->mbTide_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTide_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "format_C", 4, store->mbTide_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTide_id, "orientation", 6, store->mbTide_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "type", 4, store->mbSoundVelocity_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "long_name", 22, store->mbSoundVelocity_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "name_code", 17, store->mbSoundVelocity_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "units", 3, store->mbSoundVelocity_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "unit_code", 6, store->mbSoundVelocity_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "add_offset", NC_DOUBLE, 1, &store->mbSoundVelocity_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSoundVelocity_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "scale_factor", NC_DOUBLE, 1, &store->mbSoundVelocity_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSoundVelocity_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "minimum", NC_INT, 1, &store->mbSoundVelocity_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSoundVelocity_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "maximum", NC_INT, 1, &store->mbSoundVelocity_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSoundVelocity_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "valid_minimum", NC_INT, 1, &store->mbSoundVelocity_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSoundVelocity_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "valid_maximum", NC_INT, 1, &store->mbSoundVelocity_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSoundVelocity_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "missing_value", NC_INT, 1, &store->mbSoundVelocity_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSoundVelocity_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "format_C", 4, store->mbSoundVelocity_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, "orientation", 6, store->mbSoundVelocity_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "type", 4, store->mbHeading_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "long_name", 15, store->mbHeading_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "name_code", 16, store->mbHeading_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "units", 6, store->mbHeading_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "unit_code", 9, store->mbHeading_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbHeading_id, "add_offset", NC_DOUBLE, 1, &store->mbHeading_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeading_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbHeading_id, "scale_factor", NC_DOUBLE, 1, &store->mbHeading_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeading_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "minimum", NC_INT, 1, &store->mbHeading_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeading_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "maximum", NC_INT, 1, &store->mbHeading_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeading_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "valid_minimum", NC_INT, 1, &store->mbHeading_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeading_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "valid_maximum", NC_INT, 1, &store->mbHeading_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeading_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbHeading_id, "missing_value", NC_INT, 1, &store->mbHeading_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbHeading_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "format_C", 4, store->mbHeading_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbHeading_id, "orientation", 6, store->mbHeading_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "type", 4, store->mbRoll_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "long_name", 4, store->mbRoll_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "name_code", 13, store->mbRoll_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "units", 6, store->mbRoll_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "unit_code", 9, store->mbRoll_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbRoll_id, "add_offset", NC_DOUBLE, 1, &store->mbRoll_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRoll_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbRoll_id, "scale_factor", NC_DOUBLE, 1, &store->mbRoll_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRoll_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "minimum", NC_INT, 1, &store->mbRoll_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRoll_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "maximum", NC_INT, 1, &store->mbRoll_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRoll_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "valid_minimum", NC_INT, 1, &store->mbRoll_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRoll_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "valid_maximum", NC_INT, 1, &store->mbRoll_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRoll_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbRoll_id, "missing_value", NC_INT, 1, &store->mbRoll_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbRoll_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "format_C", 4, store->mbRoll_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbRoll_id, "orientation", 6, store->mbRoll_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "type", 4, store->mbPitch_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "long_name", 5, store->mbPitch_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "name_code", 14, store->mbPitch_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "units", 6, store->mbPitch_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "unit_code", 9, store->mbPitch_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbPitch_id, "add_offset", NC_DOUBLE, 1, &store->mbPitch_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitch_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbPitch_id, "scale_factor", NC_DOUBLE, 1, &store->mbPitch_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitch_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "minimum", NC_INT, 1, &store->mbPitch_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitch_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "maximum", NC_INT, 1, &store->mbPitch_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitch_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "valid_minimum", NC_INT, 1, &store->mbPitch_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitch_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "valid_maximum", NC_INT, 1, &store->mbPitch_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitch_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbPitch_id, "missing_value", NC_INT, 1, &store->mbPitch_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbPitch_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "format_C", 4, store->mbPitch_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbPitch_id, "orientation", 6, store->mbPitch_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "type", 4, store->mbTransmissionHeave_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "long_name", 14, store->mbTransmissionHeave_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "name_code", 14, store->mbTransmissionHeave_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "units", 1, store->mbTransmissionHeave_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "unit_code", 4, store->mbTransmissionHeave_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "add_offset", NC_DOUBLE, 1, &store->mbTransmissionHeave_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTransmissionHeave_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "scale_factor", NC_DOUBLE, 1, &store->mbTransmissionHeave_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTransmissionHeave_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "minimum", NC_INT, 1, &store->mbTransmissionHeave_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTransmissionHeave_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "maximum", NC_INT, 1, &store->mbTransmissionHeave_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTransmissionHeave_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "valid_minimum", NC_INT, 1, &store->mbTransmissionHeave_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTransmissionHeave_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "valid_maximum", NC_INT, 1, &store->mbTransmissionHeave_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTransmissionHeave_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "missing_value", NC_INT, 1, &store->mbTransmissionHeave_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbTransmissionHeave_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "format_C", 4, store->mbTransmissionHeave_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, "orientation", 6, store->mbTransmissionHeave_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "type", 4, store->mbDistanceScale_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "long_name", 16, store->mbDistanceScale_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "name_code", 17, store->mbDistanceScale_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "units", 1, store->mbDistanceScale_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "unit_code", 4, store->mbDistanceScale_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "add_offset", NC_DOUBLE, 1, &store->mbDistanceScale_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDistanceScale_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "scale_factor", NC_DOUBLE, 1, &store->mbDistanceScale_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDistanceScale_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "minimum", NC_INT, 1, &store->mbDistanceScale_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDistanceScale_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "maximum", NC_INT, 1, &store->mbDistanceScale_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDistanceScale_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "valid_minimum", NC_INT, 1, &store->mbDistanceScale_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDistanceScale_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "valid_maximum", NC_INT, 1, &store->mbDistanceScale_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDistanceScale_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "missing_value", NC_INT, 1, &store->mbDistanceScale_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDistanceScale_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "format_C", 4, store->mbDistanceScale_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, "orientation", 6, store->mbDistanceScale_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "type", 4, store->mbDepthScale_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "long_name", 11, store->mbDepthScale_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "name_code", 16, store->mbDepthScale_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "units", 1, store->mbDepthScale_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "unit_code", 4, store->mbDepthScale_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "add_offset", NC_DOUBLE, 1, &store->mbDepthScale_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepthScale_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "scale_factor", NC_DOUBLE, 1, &store->mbDepthScale_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepthScale_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "minimum", NC_INT, 1, &store->mbDepthScale_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepthScale_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "maximum", NC_INT, 1, &store->mbDepthScale_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepthScale_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "valid_minimum", NC_INT, 1, &store->mbDepthScale_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepthScale_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "valid_maximum", NC_INT, 1, &store->mbDepthScale_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepthScale_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "missing_value", NC_INT, 1, &store->mbDepthScale_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepthScale_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "format_C", 4, store->mbDepthScale_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, "orientation", 6, store->mbDepthScale_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "type", 7, store->mbVerticalDepth_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "long_name", 14, store->mbVerticalDepth_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "name_code", 14, store->mbVerticalDepth_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "units", 1, store->mbVerticalDepth_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "unit_code", 14, store->mbVerticalDepth_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "add_offset", NC_INT, 1, &store->mbVerticalDepth_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVerticalDepth_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "scale_factor", NC_INT, 1, &store->mbVerticalDepth_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVerticalDepth_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "minimum", NC_INT, 1, &store->mbVerticalDepth_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVerticalDepth_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "maximum", NC_INT, 1, &store->mbVerticalDepth_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVerticalDepth_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "valid_minimum", NC_INT, 1, &store->mbVerticalDepth_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVerticalDepth_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "valid_maximum", NC_INT, 1, &store->mbVerticalDepth_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVerticalDepth_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "missing_value", NC_INT, 1, &store->mbVerticalDepth_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVerticalDepth_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "format_C", 2, store->mbVerticalDepth_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, "orientation", 6, store->mbVerticalDepth_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "type", 7, store->mbCQuality_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "long_name", 14, store->mbCQuality_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "name_code", 16, store->mbCQuality_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "units", 1, store->mbCQuality_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "unit_code", 14, store->mbCQuality_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "add_offset", NC_INT, 1, &store->mbCQuality_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCQuality_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "scale_factor", NC_INT, 1, &store->mbCQuality_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCQuality_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "minimum", NC_INT, 1, &store->mbCQuality_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCQuality_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "maximum", NC_INT, 1, &store->mbCQuality_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCQuality_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "valid_minimum", NC_INT, 1, &store->mbCQuality_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCQuality_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "valid_maximum", NC_INT, 1, &store->mbCQuality_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCQuality_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCQuality_id, "missing_value", NC_INT, 1, &store->mbCQuality_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCQuality_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "format_C", 2, store->mbCQuality_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, "orientation", 6, store->mbCQuality_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "type", 7, store->mbCFlag_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "long_name", 13, store->mbCFlag_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "name_code", 13, store->mbCFlag_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "units", 1, store->mbCFlag_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "unit_code", 14, store->mbCFlag_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "add_offset", NC_INT, 1, &store->mbCFlag_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCFlag_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "scale_factor", NC_INT, 1, &store->mbCFlag_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCFlag_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "minimum", NC_INT, 1, &store->mbCFlag_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCFlag_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "maximum", NC_INT, 1, &store->mbCFlag_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCFlag_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "valid_minimum", NC_INT, 1, &store->mbCFlag_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "valid_maximum", NC_INT, 1, &store->mbCFlag_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbCFlag_id, "missing_value", NC_INT, 1, &store->mbCFlag_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbCFlag_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "format_C", 2, store->mbCFlag_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, "orientation", 6, store->mbCFlag_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "type", 7, store->mbInterlacing_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "long_name", 30, store->mbInterlacing_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "name_code", 20, store->mbInterlacing_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "units", 1, store->mbInterlacing_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "unit_code", 14, store->mbInterlacing_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "add_offset", NC_INT, 1, &store->mbInterlacing_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbInterlacing_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "scale_factor", NC_INT, 1, &store->mbInterlacing_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbInterlacing_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "minimum", NC_INT, 1, &store->mbInterlacing_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbInterlacing_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "maximum", NC_INT, 1, &store->mbInterlacing_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbInterlacing_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "valid_minimum", NC_INT, 1, &store->mbInterlacing_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbInterlacing_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "valid_maximum", NC_INT, 1, &store->mbInterlacing_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbInterlacing_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "missing_value", NC_INT, 1, &store->mbInterlacing_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbInterlacing_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "format_C", 2, store->mbInterlacing_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, "orientation", 6, store->mbInterlacing_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "type", 7, store->mbSamplingRate_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "long_name", 13, store->mbSamplingRate_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "name_code", 16, store->mbSamplingRate_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "units", 2, store->mbSamplingRate_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "unit_code", 5, store->mbSamplingRate_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "add_offset", NC_INT, 1, &store->mbSamplingRate_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSamplingRate_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "scale_factor", NC_INT, 1, &store->mbSamplingRate_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSamplingRate_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "minimum", NC_INT, 1, &store->mbSamplingRate_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSamplingRate_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "maximum", NC_INT, 1, &store->mbSamplingRate_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSamplingRate_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "valid_minimum", NC_INT, 1, &store->mbSamplingRate_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSamplingRate_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "valid_maximum", NC_INT, 1, &store->mbSamplingRate_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSamplingRate_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "missing_value", NC_INT, 1, &store->mbSamplingRate_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSamplingRate_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "format_C", 2, store->mbSamplingRate_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, "orientation", 6, store->mbSamplingRate_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "type", 7, store->mbAlongDistance_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "long_name", 14, store->mbAlongDistance_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "name_code", 13, store->mbAlongDistance_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "units", 1, store->mbAlongDistance_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "unit_code", 14, store->mbAlongDistance_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "add_offset", NC_INT, 1, &store->mbAlongDistance_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAlongDistance_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "scale_factor", NC_INT, 1, &store->mbAlongDistance_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAlongDistance_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "minimum", NC_INT, 1, &store->mbAlongDistance_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAlongDistance_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "maximum", NC_INT, 1, &store->mbAlongDistance_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAlongDistance_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "valid_minimum", NC_INT, 1, &store->mbAlongDistance_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAlongDistance_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "valid_maximum", NC_INT, 1, &store->mbAlongDistance_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAlongDistance_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "missing_value", NC_INT, 1, &store->mbAlongDistance_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAlongDistance_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "format_C", 2, store->mbAlongDistance_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, "orientation", 6, store->mbAlongDistance_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "type", 7, store->mbAcrossDistance_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "long_name", 15, store->mbAcrossDistance_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "name_code", 13, store->mbAcrossDistance_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "units", 1, store->mbAcrossDistance_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "unit_code", 14, store->mbAcrossDistance_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "add_offset", NC_INT, 1, &store->mbAcrossDistance_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAcrossDistance_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "scale_factor", NC_INT, 1, &store->mbAcrossDistance_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAcrossDistance_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "minimum", NC_INT, 1, &store->mbAcrossDistance_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAcrossDistance_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "maximum", NC_INT, 1, &store->mbAcrossDistance_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAcrossDistance_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "valid_minimum", NC_INT, 1, &store->mbAcrossDistance_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAcrossDistance_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "valid_maximum", NC_INT, 1, &store->mbAcrossDistance_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAcrossDistance_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "missing_value", NC_INT, 1, &store->mbAcrossDistance_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAcrossDistance_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "format_C", 2, store->mbAcrossDistance_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, "orientation", 6, store->mbAcrossDistance_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "type", 7, store->mbDepth_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "long_name", 5, store->mbDepth_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "name_code", 13, store->mbDepth_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "units", 1, store->mbDepth_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "unit_code", 14, store->mbDepth_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "add_offset", NC_INT, 1, &store->mbDepth_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepth_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "scale_factor", NC_INT, 1, &store->mbDepth_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepth_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "minimum", NC_INT, 1, &store->mbDepth_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepth_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "maximum", NC_INT, 1, &store->mbDepth_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepth_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "valid_minimum", NC_INT, 1, &store->mbDepth_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepth_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "valid_maximum", NC_INT, 1, &store->mbDepth_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepth_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbDepth_id, "missing_value", NC_INT, 1, &store->mbDepth_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbDepth_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "format_C", 2, store->mbDepth_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbDepth_id, "orientation", 7, store->mbDepth_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "type", 7, store->mbSQuality_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "long_name", 14, store->mbSQuality_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "name_code", 19, store->mbSQuality_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "units", 1, store->mbSQuality_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "unit_code", 14, store->mbSQuality_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "add_offset", NC_INT, 1, &store->mbSQuality_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSQuality_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "scale_factor", NC_INT, 1, &store->mbSQuality_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSQuality_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "minimum", NC_INT, 1, &store->mbSQuality_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSQuality_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "maximum", NC_INT, 1, &store->mbSQuality_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSQuality_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "valid_minimum", NC_INT, 1, &store->mbSQuality_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSQuality_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "valid_maximum", NC_INT, 1, &store->mbSQuality_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSQuality_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSQuality_id, "missing_value", NC_INT, 1, &store->mbSQuality_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSQuality_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "format_C", 2, store->mbSQuality_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, "orientation", 6, store->mbSQuality_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "type", 7, store->mbSFlag_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "long_name", 16, store->mbSFlag_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "name_code", 16, store->mbSFlag_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "units", 1, store->mbSFlag_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "unit_code", 14, store->mbSFlag_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "add_offset", NC_INT, 1, &store->mbSFlag_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSFlag_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "scale_factor", NC_INT, 1, &store->mbSFlag_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSFlag_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "minimum", NC_INT, 1, &store->mbSFlag_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSFlag_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "maximum", NC_INT, 1, &store->mbSFlag_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSFlag_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "valid_minimum", NC_INT, 1, &store->mbSFlag_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "valid_maximum", NC_INT, 1, &store->mbSFlag_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbSFlag_id, "missing_value", NC_INT, 1, &store->mbSFlag_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbSFlag_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "format_C", 2, store->mbSFlag_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, "orientation", 6, store->mbSFlag_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "type", 7, store->mbAntenna_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "long_name", 13, store->mbAntenna_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "name_code", 15, store->mbAntenna_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "units", 1, store->mbAntenna_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "unit_code", 14, store->mbAntenna_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "add_offset", NC_INT, 1, &store->mbAntenna_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntenna_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "scale_factor", NC_INT, 1, &store->mbAntenna_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntenna_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "minimum", NC_INT, 1, &store->mbAntenna_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntenna_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "maximum", NC_INT, 1, &store->mbAntenna_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntenna_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "valid_minimum", NC_INT, 1, &store->mbAntenna_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntenna_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "valid_maximum", NC_INT, 1, &store->mbAntenna_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntenna_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAntenna_id, "missing_value", NC_INT, 1, &store->mbAntenna_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAntenna_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "format_C", 2, store->mbAntenna_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, "orientation", 6, store->mbAntenna_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "type", 4, store->mbBeamBias_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "long_name", 9, store->mbBeamBias_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "name_code", 12, store->mbBeamBias_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "units", 1, store->mbBeamBias_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "unit_code", 4, store->mbBeamBias_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "add_offset", NC_DOUBLE, 1, &store->mbBeamBias_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeamBias_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_double((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "scale_factor", NC_DOUBLE, 1, &store->mbBeamBias_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeamBias_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "minimum", NC_INT, 1, &store->mbBeamBias_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeamBias_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "maximum", NC_INT, 1, &store->mbBeamBias_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeamBias_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "valid_minimum", NC_INT, 1, &store->mbBeamBias_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeamBias_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "valid_maximum", NC_INT, 1, &store->mbBeamBias_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeamBias_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "missing_value", NC_INT, 1, &store->mbBeamBias_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeamBias_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "format_C", 2, store->mbBeamBias_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeamBias_id, "orientation", 6, store->mbBeamBias_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "type", 7, store->mbBFlag_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "long_name", 12, store->mbBFlag_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "name_code", 12, store->mbBFlag_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "units", 1, store->mbBFlag_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "unit_code", 14, store->mbBFlag_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "add_offset", NC_INT, 1, &store->mbBFlag_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBFlag_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "scale_factor", NC_INT, 1, &store->mbBFlag_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBFlag_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "minimum", NC_INT, 1, &store->mbBFlag_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBFlag_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "maximum", NC_INT, 1, &store->mbBFlag_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBFlag_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "valid_minimum", NC_INT, 1, &store->mbBFlag_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "valid_maximum", NC_INT, 1, &store->mbBFlag_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBFlag_id, "missing_value", NC_INT, 1, &store->mbBFlag_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBFlag_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "format_C", 2, store->mbBFlag_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, "orientation", 6, store->mbBFlag_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "type", 7, store->mbBeam_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "long_name", 15, store->mbBeam_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "name_code", 15, store->mbBeam_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "units", 1, store->mbBeam_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "unit_code", 14, store->mbBeam_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "add_offset", NC_INT, 1, &store->mbBeam_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeam_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "scale_factor", NC_INT, 1, &store->mbBeam_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeam_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "minimum", NC_INT, 1, &store->mbBeam_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeam_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "maximum", NC_INT, 1, &store->mbBeam_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeam_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "valid_minimum", NC_INT, 1, &store->mbBeam_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeam_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "valid_maximum", NC_INT, 1, &store->mbBeam_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeam_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbBeam_id, "missing_value", NC_INT, 1, &store->mbBeam_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbBeam_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "format_C", 2, store->mbBeam_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbBeam_id, "orientation", 6, store->mbBeam_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "type", 7, store->mbAFlag_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "long_name", 15, store->mbAFlag_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "name_code", 15, store->mbAFlag_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "units", 1, store->mbAFlag_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "unit_code", 14, store->mbAFlag_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "add_offset", NC_INT, 1, &store->mbAFlag_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAFlag_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "scale_factor", NC_INT, 1, &store->mbAFlag_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAFlag_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "minimum", NC_INT, 1, &store->mbAFlag_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAFlag_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "maximum", NC_INT, 1, &store->mbAFlag_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAFlag_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "valid_minimum", NC_INT, 1, &store->mbAFlag_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAFlag_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "valid_maximum", NC_INT, 1, &store->mbAFlag_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAFlag_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbAFlag_id, "missing_value", NC_INT, 1, &store->mbAFlag_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbAFlag_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "format_C", 2, store->mbAFlag_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, "orientation", 6, store->mbAFlag_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, "type", 6, store->mbVelProfilRef_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, "long_name", 28, store->mbVelProfilRef_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, "name_code", 13, store->mbVelProfilRef_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "type", 7, store->mbVelProfilIdx_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "long_name", 34, store->mbVelProfilIdx_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "name_code", 13, store->mbVelProfilIdx_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "units", 1, store->mbVelProfilIdx_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "unit_code", 14, store->mbVelProfilIdx_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "add_offset", NC_INT, 1, &store->mbVelProfilIdx_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilIdx_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "scale_factor", NC_INT, 1, &store->mbVelProfilIdx_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilIdx_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "minimum", NC_INT, 1, &store->mbVelProfilIdx_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilIdx_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "maximum", NC_INT, 1, &store->mbVelProfilIdx_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilIdx_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "valid_minimum", NC_INT, 1, &store->mbVelProfilIdx_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilIdx_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "valid_maximum", NC_INT, 1, &store->mbVelProfilIdx_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilIdx_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "missing_value", NC_INT, 1, &store->mbVelProfilIdx_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilIdx_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "format_C", 2, store->mbVelProfilIdx_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, "orientation", 6, store->mbVelProfilIdx_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "type", 7, store->mbVelProfilDate_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "long_name", 13, store->mbVelProfilDate_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "name_code", 14, store->mbVelProfilDate_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "units", 11, store->mbVelProfilDate_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "unit_code", 14, store->mbVelProfilDate_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "add_offset", NC_INT, 1, &store->mbVelProfilDate_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilDate_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "scale_factor", NC_INT, 1, &store->mbVelProfilDate_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilDate_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "minimum", NC_INT, 1, &store->mbVelProfilDate_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilDate_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "maximum", NC_INT, 1, &store->mbVelProfilDate_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilDate_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "valid_minimum", NC_INT, 1, &store->mbVelProfilDate_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilDate_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "valid_maximum", NC_INT, 1, &store->mbVelProfilDate_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilDate_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "missing_value", NC_INT, 1, &store->mbVelProfilDate_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilDate_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "format_C", 2, store->mbVelProfilDate_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, "orientation", 6, store->mbVelProfilDate_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "type", 7, store->mbVelProfilTime_type); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "long_name", 13, store->mbVelProfilTime_long_name); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "name_code", 14, store->mbVelProfilTime_name_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "units", 2, store->mbVelProfilTime_units); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "unit_code", 5, store->mbVelProfilTime_unit_code); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "add_offset", NC_INT, 1, &store->mbVelProfilTime_add_offset);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilTime_add_offset error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "scale_factor", NC_INT, 1, &store->mbVelProfilTime_scale_factor);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilTime_scale_factor error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "minimum", NC_INT, 1, &store->mbVelProfilTime_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilTime_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "maximum", NC_INT, 1, &store->mbVelProfilTime_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilTime_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "valid_minimum", NC_INT, 1, &store->mbVelProfilTime_valid_minimum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilTime_valid_minimum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "valid_maximum", NC_INT, 1, &store->mbVelProfilTime_valid_maximum);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilTime_valid_maximum error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "missing_value", NC_INT, 1, &store->mbVelProfilTime_missing_value);
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att mbVelProfilTime_missing_value error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "format_C", 2, store->mbVelProfilTime_format_C); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));
	    nc_status = nc_put_att_text((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, "orientation", 6, store->mbVelProfilTime_orientation); 
	    if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_att JJJJ error: %s\n", nc_strerror(nc_status));

	    /* end define mode */
	    nc_status = nc_enddef((int)mb_io_ptr->mbfp);

	    /* save the comments */
	    
	    /* save the beam arrays */
	    
	    /* save the soundvel arrays */

	    /* write global variables */
	    if (status == MB_SUCCESS)
		{
		index[0] = 0;
		count[0] = store->mbNbrHistoryRec;
		nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbHistDate_id, index, count, store->mbHistDate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbHistDate error: %s\n", nc_strerror(nc_status));
		index[0] = 0;
		count[0] = store->mbNbrHistoryRec;
		nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbHistTime_id, index, count, store->mbHistTime);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbHistTime error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbNbrHistoryRec;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbHistCode_id, index, count, store->mbHistCode);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbHistCode error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		index[1] = 0;
		count[0] = store->mbNbrHistoryRec;
		count[1] = store->mbNameLength;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbHistAutor_id, index, count, store->mbHistAutor);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbHistAutor error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		index[1] = 0;
		count[0] = store->mbNbrHistoryRec;
		count[1] = store->mbNameLength;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbHistModule_id, index, count, store->mbHistModule);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbHistModule error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		index[1] = 0;
		count[0] = store->mbNbrHistoryRec;
		count[1] = store->mbCommentLength;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbHistComment_id, index, count, store->mbHistComment);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbHistComment error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbBeamNbr;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbAntenna_id, index, count, store->mbAntenna);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbAntenna error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbBeamNbr;
		nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbBeamBias_id, index, count, store->mbBeamBias);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbBeamBias error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbBeamNbr;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbBFlag_id, index, count, store->mbBFlag);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbBFlag error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbAntennaNbr;
		nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbBeam_id, index, count, store->mbBeam);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbBeam error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbAntennaNbr;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbAFlag_id, index, count, store->mbAFlag);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbAFlag error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		index[1] = 0;
		count[0] = store->mbVelocityProfilNbr;
		count[1] = store->mbCommentLength;
		nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbVelProfilRef_id, index, count, store->mbVelProfilRef);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbVelProfilRef error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbVelocityProfilNbr;
		nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbVelProfilIdx_id, index, count, store->mbVelProfilIdx);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbVelProfilIdx error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbVelocityProfilNbr;
		nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbVelProfilDate_id, index, count, store->mbVelProfilDate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbVelProfilDate error: %s\n", nc_strerror(nc_status));
		
		index[0] = 0;
		count[0] = store->mbVelocityProfilNbr;
		nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbVelProfilTime_id, index, count, store->mbVelProfilTime);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbVelProfilTime error: %s\n", nc_strerror(nc_status));
		if (nc_status != NC_NOERR)
		    {
		    status = MB_FAILURE;
		    *error = MB_ERROR_EOF;
		    }		
		}
	    }
	
	/* now write the data */
	if (store->kind == MB_DATA_DATA
	    && status == MB_SUCCESS)
	    {
	    /* write the variables from next record */
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbCycle_id, index, count, store->mbCycle);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbCycle error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbDate_id, index, count, store->mbDate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbDate error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbTime_id, index, count, store->mbTime);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbTime error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbOrdinate_id, index, count, store->mbOrdinate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbOrdinate error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_int((int)mb_io_ptr->mbfp, store->mbAbscissa_id, index, count, store->mbAbscissa);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbAbscissa error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbFrequency_id, index, count, store->mbFrequency);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbFrequency error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbSounderMode_id, index, count, store->mbSounderMode);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbSounderMode error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbReferenceDepth_id, index, count, store->mbReferenceDepth);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbReferenceDepth error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbDynamicDraught_id, index, count, store->mbDynamicDraught);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbDynamicDraught error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbTide_id, index, count, store->mbTide);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbTide error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbSoundVelocity_id, index, count, store->mbSoundVelocity);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbSoundVelocity error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbHeading_id, index, count, store->mbHeading);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbHeading error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbRoll_id, index, count, store->mbRoll);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbRoll error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbPitch_id, index, count, store->mbPitch);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbPitch error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbTransmissionHeave_id, index, count, store->mbTransmissionHeave);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbTransmissionHeave error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbDistanceScale_id, index, count, store->mbDistanceScale);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbDistanceScale error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbDepthScale_id, index, count, store->mbDepthScale);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbDepthScale error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbVerticalDepth_id, index, count, store->mbVerticalDepth);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbVerticalDepth error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbCQuality_id, index, count, store->mbCQuality);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbCQuality error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbCFlag_id, index, count, store->mbCFlag);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbCFlag error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbInterlacing_id, index, count, store->mbInterlacing);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbInterlacing error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbAntennaNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbSamplingRate_id, index, count, store->mbSamplingRate);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbSamplingRate error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbBeamNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbAlongDistance_id, index, count, store->mbAlongDistance);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbAlongDistance error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbBeamNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbAcrossDistance_id, index, count, store->mbAcrossDistance);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbAcrossDistance error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbBeamNbr;
	    nc_status = nc_put_vara_short((int)mb_io_ptr->mbfp, store->mbDepth_id, index, count, store->mbDepth);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbDepth error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbBeamNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbSQuality_id, index, count, store->mbSQuality);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbSQuality error: %s\n", nc_strerror(nc_status));
	    index[0] = *recwrite;
	    index[1] = 0;
	    count[0] = 1;
	    count[1] = store->mbBeamNbr;
	    nc_status = nc_put_vara_text((int)mb_io_ptr->mbfp, store->mbSFlag_id, index, count, store->mbSFlag);
	    	if (verbose >= 1 && nc_status != NC_NOERR) fprintf(stderr, "nc_put_vara mbSQuality error: %s\n", nc_strerror(nc_status));
	    
	    /* check status */
	    if (nc_status != NC_NOERR)
		{
		status = MB_FAILURE;
		*error = MB_ERROR_EOF;
		}		
	    
	    /* set counters */
	    (*recwrite)++;
	    (*datawrite)++;
	    
	    }

	/* print output debug statements */
	if (verbose >= 2)
		{
		fprintf(stderr,"\ndbg2  MBIO function <%s> completed\n",
			function_name);
		fprintf(stderr,"dbg2  Return values:\n");
		fprintf(stderr,"dbg2       error:      %d\n",*error);
		fprintf(stderr,"dbg2  Return status:\n");
		fprintf(stderr,"dbg2       status:  %d\n",status);
		}

	/* return status */
	return(status);
}
/*--------------------------------------------------------------------*/