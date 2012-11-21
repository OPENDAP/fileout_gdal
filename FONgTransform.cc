// FONgTransform.cc

// This file is part of BES GDAL File Out Module

// Copyright (c) 2012 OPeNDAP, Inc.
// Author: James Gallagher <jgallagher@opendap.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation; either
// version 2.1 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// You can contact University Corporation for Atmospheric Research at
// 3080 Center Green Drive, Boulder, CO 80301

#include "config.h"

#include <gdal.h>
#include <gdal_priv.h>
#include <ogr_spatialref.h>

#include <DDS.h>
#include <ConstraintEvaluator.h>

#include <Structure.h>
#include <Array.h>
#include <Grid.h>
#include <ce_functions.h>

#include <BESDebug.h>
#include <BESInternalError.h>

#include "FONgTransform.h"

#include "FONgBaseType.h"
#include "FONgGrid.h"

using namespace std;
using namespace libdap;

/** @brief Constructor that creates transformation object from the specified
 * DataDDS object to the specified file
 *
 * @param dds DataDDS object that contains the data structure, attributes
 * and data
 * @param dhi The data interface containing information about the current
 * request
 * @param localfile GeoTiff to create and write the information to
 * @throws BESInternalError
 */
FONgTransform::FONgTransform(DDS *dds, ConstraintEvaluator &evaluator, const string &localfile) :
        d_dds(dds), d_evaluator(evaluator), d_localfile(localfile),
        d_geo_transform_set(false), d_type(dods_null_c), d_num_bands(0)
{
    if (localfile.empty())
        throw BESInternalError("Empty local file name passed to constructor", __FILE__, __LINE__);

#if 0
    // if there is a variable, attribute, dimension name that is not
    // compliant with geotiff naming conventions then we will create
    // a new name. If the new name does not begin with an alpha
    // character then we will prefix it with name_prefix. We will
    // get this prefix from the type of data that we are reading in,
    // such as nc, h4, h5, ff, jg, etc...
    dhi.first_container();
    if (dhi.container) {
        FONgUtils::name_prefix = dhi.container->get_container_type() + "_";
    }
    else {
        FONgUtils::name_prefix = "nc_";
    }
#endif
}

/** @brief Destructor
 *
 * Cleans up any temporary data created during the transformation
 */
FONgTransform::~FONgTransform()
{
    vector<FONgBaseType *>::iterator i = d_fong_vars.begin();
    vector<FONgBaseType *>::iterator e = d_fong_vars.end();
    while (i != e) {
        delete (*i++);
    }

#if 0
    bool done = false;
    while (!done) {
        vector<FONgBaseType *>::iterator i = _fong_vars.begin();
        vector<FONgBaseType *>::iterator e = _fong_vars.end();
        if (i == e) {
            done = true;
        }
        else {
            // These are the FONg types, not the actual ones
            FONgBaseType *b = (*i);
            delete b;
            _fong_vars.erase(i);
        }
    }
#endif
}

static bool
is_convertable_type(const BaseType *b)
{
    switch (b->type()) {
        case dods_grid_c:
            return true;

            // TODO Add these once the handler works with Grid
        case dods_array_c:
        case dods_structure_c:
        default:
            return false;
    }
}

/** @brief creates a FONg object for the given DAP object
 *
 * @param v The DAP object to convert
 * @returns The FONg object created via the DAP object
 * @throws BESInternalError if the DAP object is not an expected type
 */
static FONgBaseType *convert(BaseType *v)
{
    switch (v->type()) {
#if 0
    case dods_str_c:
    case dods_url_c:
        b = new FONgStr(v);
        break;
    case dods_byte_c:
        b = new FONgByte(v);
        break;
    case dods_int16_c:
    case dods_uint16_c:
        b = new FONgShort(v);
        break;
    case dods_int32_c:
    case dods_uint32_c:
        b = new FONgInt(v);
        break;
    case dods_float32_c:
        b = new FONgFloat(v);
        break;
    case dods_float64_c:
        b = new FONgDouble(v);
        break;
#endif
    case dods_grid_c:
        return  new FONgGrid(static_cast<Grid*>(v));

#if 0
    case dods_array_c:
        b = new FONgArray(v);
        break;
    case dods_structure_c:
        b = new FONgStructure(v);
        break;
    case dods_sequence_c:
        b = new FONgSequence(v);
        break;
#endif
    default:
        throw BESInternalError("file out geotiff, unable to write unknown variable type", __FILE__, __LINE__);
    }
}

static GDALDataType DAP_to_GDAL_type(Type t)
{
    switch (t) {
    case dods_float32_c:
        return GDT_Float32;
    default:
        throw InternalErr(__FILE__, __LINE__, "Unknown GDAL type");
    }
}

double *FONgTransform::geo_transform()
{
   // The values for gt[0] and gt[3] are the geographic coords of the top left pixel
   d_gt[0] = d_top;
   d_gt[3] = d_left;

   // We assume data w/o any rotation (a north-up image)
   d_gt[2] = 0.0;
   d_gt[4] = 0.0;

   // Compute the lower left values
   d_gt[1] = (d_bottom - d_top) / d_width; // width in pixels; top and bottom in lat
   d_gt[5] = (d_right - d_left) / d_height;

   return d_gt;
}

/** @brief Transforms each of the variables of the DataDDS to the GeoTiff
 * or JPEG2000 file.
 *
 */
void FONgTransform::transform()
{
    // Convert the DDS into an internal format to keep track of
    // variables, arrays, shared dimensions, grids, common maps,
    // embedded structures. It only grabs the variables that are to be
    // sent.

    // What needs to be determined:
    // Number of bands and their type and pixel size. These must be uniform.
    // Use the geo-ftransform parameters of the first variable to set the
    // values for the transform. Test each remaining variable to be sure its
    // parameters match those of the first variable.
    DDS::Vars_iter vi = d_dds->var_begin();
    while (vi != d_dds->var_end()) {
        if ((*vi)->send_p() && is_convertable_type(*vi)) {
            BESDEBUG( "fong", "converting " << (*vi)->name() << endl );

            // Build the delegate
            FONgBaseType *fb = convert(*vi);

            // Get the information needed for the transform
            fb->extract_coordinates(*this);

            // Read the data if extract_coordinates() worked (throws on error)
            (*vi)->intern_data(d_evaluator, *d_dds);
        }

        ++vi;
    }

    BESDEBUG( "fong", "Past conversion loop" << endl );

    GDALAllRegister();

    GDALDriver *Driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if( Driver == NULL )
        throw Error("Could not process request. (1)");

    char **Metadata = Driver->GetMetadata();
    if (!CSLFetchBoolean(Metadata, GDAL_DCAP_CREATE, FALSE))
        throw Error("Could not make output format.");

    const char* options_list = GDALGetDriverCreationOptionList (Driver);
    BESDEBUG("fong", "Options list: " << options_list << endl);

    // char **Options = NULL; args: name, x, y, nBands
    // FIXME num_bands hack
    char *options[] = { (char*)"GDAL_NODATA=-9.99999979e+33" };
    d_dest = Driver->Create(d_localfile.c_str(), width(), height(), 1/*num_bands()*/, GDT_Byte/*DAP_to_GDAL_type(type())*/, NULL);
    if (!d_dest) {
        throw Error("Could not process request.");
    }

#if 0
    double adfGeoTransform[6] = { 444720, 30, 0, 3751320, 0, -30 };
    OGRSpatialReference srs;
    char *pszSRS_WKT = NULL;
    GDALRasterBand *poBand;
    GByte abyRaster[512*512];

    poDstDS->SetGeoTransform( adfGeoTransform );

    srs.SetUTM( 11, TRUE );
    srs.SetWellKnownGeogCS( "NAD27" );
    srs.exportToWkt( &pszSRS_WKT );
    poDstDS->SetProjection( pszSRS_WKT );
    CPLFree( pszSRS_WKT );

    poBand = poDstDS->GetRasterBand(1);
    poBand->RasterIO( GF_Write, 0, 0, 512, 512,
                      abyRaster, 512, 512, GDT_Byte, 0, 0 );
#endif

    // Assume north-up image
    //double geo_trans_array[6] = { top(), width(), 0, left(), 0, height() };
    d_dest->SetGeoTransform(geo_transform());

    // We don't know this... srs.SetUTM( 11, TRUE );
    // Guess at this; can use EPSG codes too
    OGRSpatialReference srs;
    srs.SetWellKnownGeogCS( "WGS84" );
    char *srs_wkt= NULL;
    srs.exportToWkt( &srs_wkt );
    d_dest->SetProjection( srs_wkt );
    CPLFree( srs_wkt );

    BESDEBUG("fong", "Made new temp file and set georeferencing (" << num_var() << " vars)." << endl);

    // FIXME it works for float32 only
    // FIXME num_bands hack

    // TODO check this once the code handles Array

#if 0
    // FIXME hardcoded zero
    Grid *g = static_cast<FONgGrid*>(var(0))->grid();
    Array *a = static_cast<Array*>(g->array_var());

    // Read all the data in one shot
    vector<float> data(width() * height());
    BESDEBUG("fong", "data holds " << width() * height() << " elements" << endl);

    a->value(&data[0]);
    BESDEBUG("fong", "Got Array '" << a->name() << "' from the grid" << endl);
#endif

    Grid *g = static_cast<FONgGrid*>(var(0))->grid();

    if (!g->get_array()->read_p())
        g->get_array()->read();

    // This code assumes read() has been called.
    double *data = extract_double_array(g->get_array());

    // hack the values; because the missing value used with many datasets
    // is really small, move it to some number just below the smallest value
    // That is not the no_data value. Don't do any of this if there's no
    // 'no_data' value.

    // FIXME Assumed no_data is really small
    // Find the smallest value that's not 'no_data'
    double small = 0.0;
    for (int i = 0; i < width() * height(); ++i)
        if (data[i] > no_data() && data[i] < small)
            small = data[i];

    BESDEBUG("fong", "Min value: " << small << endl);

    for (int i = 0; i < width() * height(); ++i) {
        if (data[i] <= no_data()) {
            BESDEBUG("fong2", "Found a value to ignore" << endl);
            data[i] = small;
        }
    }

    // The extract...() call above ensures the data are in a double array.
    // Not always efficient, but simple

    CPLErr error = d_dest->RasterIO(GF_Write, 0, 0, width(), height(),
            &data[0], width(), height(), GDT_Float64,
            /*BandCount*/1, /*BandMap - 0 --> all bands*/0,
            /*PixelSpace*/0, /*LineSpace*/0, /*BandSpace*/0);

    GDALClose(d_dest);
}
