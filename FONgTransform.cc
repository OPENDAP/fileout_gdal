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
        d_geo_transform_set(false), d_num_bands(0), d_no_data_type(none)
{
    if (localfile.empty())
        throw BESInternalError("Empty local file name passed to constructor", __FILE__, __LINE__);
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
}

/** @brief can this DAP type be turned into a GeoTiff or JP2 file?
 *
 */
static bool
is_convertable_type(const BaseType *b)
{
    switch (b->type()) {
        case dods_grid_c:
            return true;

        // TODO Add support for Array
        case dods_array_c:
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
    case dods_grid_c:
        return  new FONgGrid(static_cast<Grid*>(v));
#if 0
    case dods_array_c:
        b = new FONgArray(v);
        break;
#endif
    default:
        throw BESInternalError("file out GeoTiff, unable to write unknown variable type", __FILE__, __LINE__);
    }
}

/** @breif scale the values for a better looking result
 * Often datasets use very small (or less often, very large) values
 * to indicate 'no data' or 'missing data'. The GDAL library scales
 * data so that the entire range of data values are represented in
 * a grayscale image. When the no data value is very small this
 * skews the mean of the values to some very small number. This code
 * builds a histogram of the data and then computes a new 'no data'
 * value based on the smallest (or largest) value that is greater
 * than (or less than) the no data value.
 *
 * @note The no data value is determined be looking at attributes and
 * is done by FONgBaseType::extract_coordinates()
 *
 * @param data The data values to fiddle
 */
void FONgTransform::m_scale_data(double *data)
{
    // It is an error to call this if no_data_type() is 'none'
    set<double> hist;
    for (int i = 0; i < width() * height(); ++i)
        hist.insert(data[i]);

    BESDEBUG("fong3", "Hist count: " << hist.size() << endl);

    if (no_data_type() == negative && hist.size() > 1) {
        // Values are sorted by 'weak' <, so this is the smallest value
        // and ++i is the next smallest value. Assume no_data is the
        // smallest value in the data set and ++i is the smallest actual
        // data value. Reset the no_data value to be 1.0 < the smallest
        // actual value. This makes for a good grayscale photometric
        // GeoTiff w/o changing the actuall data values.
        set<double>::iterator i = hist.begin();
        double smallest = *(++i);
        if (fabs(smallest + no_data()) > 1) {
            smallest -= 1.0;

            BESDEBUG("fong3", "New no_data value: " << smallest << endl);

            for (int i = 0; i < width() * height(); ++i) {
                if (data[i] <= no_data()) {
                    data[i] = smallest;
                }
            }
        }
    }
    else {
        set<double>::reverse_iterator r = hist.rbegin();
        double biggest = *(++r);
        if (fabs(no_data() - biggest) > 1) {
            biggest += 1.0;

            BESDEBUG("fong3", "New no_data value: " << biggest << endl);

            for (int i = 0; i < width() * height(); ++i) {
                if (data[i] >= no_data()) {
                    data[i] = biggest;
                }
            }
        }
    }
}

/** @brief Build the geotransform array needed by GDAL
 * This code uses values gleaned by FONgBaseType:extract_coordinates()
 * to build up the six value array of transform paramaeters that
 * GDAL needs to set the geographic transform.
 *
 * @note This method returns a pointer to a class field; don't delete
 * it.
 *
 * @return A pointer to the six element array of parameters (doubles)
 */
double *FONgTransform::geo_transform()
{
   // The values for gt[0] and gt[3] are the geographic coords of the top left pixel
   d_gt[0] = d_top;
   d_gt[3] = d_left;

   // We assume data w/o any rotation (and a north-up image)
   d_gt[2] = 0.0;
   d_gt[4] = 0.0;

   // Compute the lower left values
   d_gt[1] = (d_bottom - d_top) / d_width; // width in pixels; top and bottom in lat
   d_gt[5] = (d_right - d_left) / d_height;

   return d_gt;
}

bool FONgTransform::effectively_two_D(FONgBaseType *fbtp)
{
    if (fbtp->type() == dods_grid_c) {
        Grid *g = static_cast<FONgGrid*>(fbtp)->grid();
        if (g->get_array()->dimensions() == 2)
            return true;
        // count the dimensions with sizes other than 1
        Array *a = g->get_array();
        int dims = 0;
        for (Array::Dim_iter d = a->dim_begin(); d != a->dim_end(); ++d) {
            if (a->dimension_size(d, true) > 1)
                ++dims;
        }

        return dims == 2;
    }

    return false;
}

// Helper function to descend into Structures looking for Grids (and Arrays
// someday).
static void recursive_look_for_vars(Structure *s, FONgTransform &t)
{
    Structure::Vars_iter vi = s->var_begin();
    //DDS::Vars_iter vi = d_dds->var_begin();
    while (vi != s->var_end()) {
        if ((*vi)->send_p() && is_convertable_type(*vi)) {
            BESDEBUG( "fong3", "converting " << (*vi)->name() << endl);

            // Build the delegate
            FONgBaseType *fb = convert(*vi);

            // Get the information needed for the transform
            fb->extract_coordinates(t);
        }
        else if ((*vi)->type() == dods_structure_c) {
            recursive_look_for_vars(static_cast<Structure*>(*vi), t);
        }

        ++vi;
    }
}

// Helper function to scan the DDS top-level for Grids, ...
// Note that FONgBaseType::extract_coordinates() sets a bunch of
// values in the FONgBaseType instance _and_ this instance of
// FONgTransform. One of these is 'num_bands()'. For GeoTiff,
// num_bands() must be 1. This is tested in transform().
static void look_for_vars(DDS *dds, FONgTransform &t)
{
    DDS::Vars_iter vi = dds->var_begin();
    while (vi != dds->var_end()) {
        if ((*vi)->send_p() && is_convertable_type(*vi)) {
            BESDEBUG( "fong3", "converting " << (*vi)->name() << endl);

            // Build the delegate
            FONgBaseType *fb = convert(*vi);

            // Get the information needed for the transform
            fb->extract_coordinates(t);
        }
        else if ((*vi)->type() == dods_structure_c) {
            recursive_look_for_vars(static_cast<Structure*>(*vi), t);
        }

        ++vi;
    }
}

/** @brief Transforms each of the variables of the DataDDS to a GeoTiff file.
 * Scan the DDS for the dataset and find the one Grid (or Array when implemented)
 * that has been projected. Try to render it's content as a GeoTiff. The result
 * is a single-band GeoTiff grayscale photometric file.
 */
void FONgTransform::transform()
{
    // Scan the entire DDS looking for variables that have been 'projected'.
    look_for_vars(d_dds, *this);

    if (num_bands() != 1)
        throw Error("GeoTiff responses can consist of one band only.");

    // Hardcoded that there's only one variable when building a GeoTiff response
    if (!effectively_two_D(var(0)))
        throw Error("GeoTiff responses can consist of one two-dimensional variable; use constraints to reduce he size of Grids and Arrays as needed.");

    GDALAllRegister();

    GDALDriver *Driver = GetGDALDriverManager()->GetDriverByName("GTiff");
    if( Driver == NULL )
        throw Error("Could not process request. (1)");

    char **Metadata = Driver->GetMetadata();
    if (!CSLFetchBoolean(Metadata, GDAL_DCAP_CREATE, FALSE))
        throw Error("Could not make output format.");

    // NB: Changing PHOTOMETIC to MINISWHITE doesn't seem to have any visible affect,
    // although the resulting files differ. jhrg 11/21/12
    char **options = NULL;
    options = CSLSetNameValue(options, "PHOTOMETRIC", "MINISBLACK" ); // The default for GDAL
    d_dest = Driver->Create(d_localfile.c_str(), width(), height(), 1/*num_bands()*/, GDT_Float64, options);
    if (!d_dest)
        throw Error("Could not process request.");

    d_dest->SetGeoTransform(geo_transform());

    FONgBaseType *fbtp = var(0);

    fbtp->set_projection(d_dds, d_dest);

    BESDEBUG("fong3", "Made new temp file and set georeferencing (" << num_var() << " vars)." << endl);

    double *data = fbtp->get_data();

    try {
        // hack the values; because the missing value used with many datasets
        // is often really small it'll skew the mapping of values to the grayscale
        // that GDAL performs. Move the no_data values to something closer to the
        // other values in the dataset.
        BESDEBUG("fong3", "no_data_type(): " << no_data_type() << endl);
        if (no_data_type() != none)
            m_scale_data(data);

        // The extract...() call above ensures the data are in a double array.
        // Not always efficient, but simple

        CPLErr error = d_dest->RasterIO(GF_Write, 0, 0, width(), height(),
                data, width(), height(), GDT_Float64,
                /*BandCount*/1, /*BandMap - 0 --> all bands*/0,
                /*PixelSpace*/0, /*LineSpace*/0, /*BandSpace*/0);

        if (error != CPLE_None)
            throw Error(CPLGetLastErrorMsg());

        delete[] data;
        GDALClose(d_dest);
    }
    catch (...) {
        delete[] data;
        GDALClose(d_dest);

        throw;
    }

}
