// FONgGrid.cc

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

#include <BESInternalError.h>
#include <BESDebug.h>

#include <Grid.h>
//#include <GeoConstraint.h>      // An abstract class, but defines a useful function
#include <ce_functions.h>
#include <util.h>

#include "FONgTransform.h"
#include "FONgGrid.h"

using namespace libdap;

/** @brief Constructor for FONgGrid that takes a DAP Grid
 *
 * @param g A DAP BaseType that should be a grid
 */
FONgGrid::FONgGrid(Grid *g) : FONgBaseType(), d_grid(g), d_lat(0), d_lon(0)
{
    // Build sets of attribute values for easy searching.
    // Copied from GeoConstriant in libdap. That class is
    // abstract and didn't want to modify libdap's ABI for this hack.

    d_coards_lat_units.insert("degrees_north");
    d_coards_lat_units.insert("degree_north");
    d_coards_lat_units.insert("degree_N");
    d_coards_lat_units.insert("degrees_N");

    d_coards_lon_units.insert("degrees_east");
    d_coards_lon_units.insert("degree_east");
    d_coards_lon_units.insert("degrees_E");
    d_coards_lon_units.insert("degree_E");

    d_lat_names.insert("COADSY");
    d_lat_names.insert("lat");
    d_lat_names.insert("Lat");
    d_lat_names.insert("LAT");

    d_lon_names.insert("COADSX");
    d_lon_names.insert("lon");
    d_lon_names.insert("Lon");
    d_lon_names.insert("LON");
}

/** @brief Destructor that cleans up the grid
 *
 * The DAP Grid instance does not belong to the FONgGrid instance, so it
 * is not deleted.
 */
FONgGrid::~FONgGrid()
{
}

#if 1
/** This is used with find_if(). The GeoConstraint class holds a set of strings
    which are prefixes for variable names. Using the regular find() locates
    only the exact matches, using find_if() with this functor makes is easy
    to treat those set<string> objects as collections of prefixes. */
class is_prefix
{
private:
    string s;
public:
    is_prefix(const string & in): s(in)
    {}

    bool operator()(const string & prefix)
    {
        return s.find(prefix) == 0;
    }
};

/** Look in the containers which hold the units attributes and variable name
    prefixes which are considered as identifying a vector as being a latitude
    or longitude vector.

    @param units A container with a bunch of units attribute values.
    @param names A container with a bunch of variable name prefixes.
    @param var_units The value of the 'units' attribute for this variable.
    @param var_name The name of the variable.
    @return True if the units_value matches any of the accepted attributes
    exactly or if the name_value starts with any of the accepted prefixes */
bool
unit_or_name_match(set < string > units, set < string > names,
                       const string & var_units, const string & var_name)
{
    return (units.find(var_units) != units.end()
            || find_if(names.begin(), names.end(),
                       is_prefix(var_name)) != names.end());
}

#endif
/** A private method called by the constructor that searches for latitude
    and longitude map vectors. This method returns false if either map
    cannot be found. It assumes that the d_grid and d_dds fields are set.

    The d_longitude, d_lon, d_lon_length and d_lon_grid_dim (and matching
    lat) fields are modified.

    @note Rules used to find Maps:<ul>
    <li>Latitude: If the Map has a units attribute of "degrees_north",
    "degree_north", "degree_N", or "degrees_N"</li>
    <li>Longitude: If the map has a units attribute of "degrees_east"
    (eastward positive), "degree_east", "degree_E", or "degrees_E"</li>
    </ul>

    @return True if the maps are found, otherwise False */
bool FONgGrid::find_lat_lon_maps()
{
    Grid::Map_iter m = d_grid->map_begin();

    // Assume that a Grid is correct and thus has exactly as many maps as its
    // array part has dimensions. Thus, don't bother to test the Grid's array
    // dimension iterator for '!= dim_end()'.
    Array::Dim_iter d = d_grid->get_array()->dim_begin();

    // The fields d_latitude and d_longitude may be initialized to null or they
    // may already contain pointers to the maps to use. In the latter case,
    // skip the heuristics used in this code. However, given that all this
    // method does is find the lat/lon maps, if they are given in the ctor,
    // This method will likely not be called at all.

    while (m != d_grid->map_end() && (!d_lat || !d_lon)) {
        string units_value = (*m)->get_attr_table().get_attr("units");
        units_value = remove_quotes(units_value);
        string map_name = (*m)->name();

        // The 'units' attribute must match exactly; the name only needs to
        // match a prefix.
        if (!d_lat && unit_or_name_match(d_coards_lat_units, d_lat_names, units_value, map_name)) {

            // Set both d_latitude (a pointer to the real map vector) and
            // d_lat, a vector of the values represented as doubles. It's easier
            // to work with d_lat, but it's d_latitude that needs to be set
            // when constraining the grid. Also, record the grid variable's
            // dimension iterator so that it's easier to set the Grid's Array
            // (which also has to be constrained).
            d_lat = dynamic_cast < Array * >(*m);
            if (!d_lat)
                throw InternalErr(__FILE__, __LINE__, "Expected an array.");

            if (!d_lat->read_p())
                d_lat->read();
#if 0
            set_lat(extract_double_array(d_lat));   // throws Error
            set_lat_length(d_lat->length());

            set_lat_dim(d);
#endif
        }

        if (!d_lon  && unit_or_name_match(d_coards_lon_units, d_lon_names, units_value, map_name)) {

            d_lon = dynamic_cast < Array * >(*m);
            if (!d_lon)
                throw InternalErr(__FILE__, __LINE__, "Expected an array.");

            if (!d_lon->read_p())
                d_lon->read();
#if 0
            set_lon(extract_double_array(d_longitude));
            set_lon_length(d_longitude->length());

            set_lon_dim(d);

            if (m + 1 == d_grid->map_end())
                set_longitude_rightmost(true);
#endif
        }

        ++m;
        ++d;
    }

    return d_lat && d_lon;
}

/** Extract the size (pixels), element data type and top-left and
 * bottom-right lat/lon corner points for the Grid. Also determine
 * if this is a 2D or 3D Grid and, in the latter case, ensure that
 * the first dimension is not lat or lon. In that case, the geotiff
 * will have N bands, where N is the number of elements from the
 * first dimension in the current selection.
 *
 */
void FONgGrid::extract_coordinates(FONgTransform &t)
{
    BESDEBUG("fong", "Entering FONgGrid::extract_coordinates" << endl);

    try {
        // Find the lat and lon maps for this Grid; throws Error
        // if the are not found.
        find_lat_lon_maps();

        // The array size
        t.set_height(d_lat->length());
        t.set_width(d_lon->length());

        double *lat = extract_double_array(d_lat);
        double *lon = extract_double_array(d_lon);

        t.set_top(lat[0]);
        t.set_left(lon[0]);
        t.set_bottom(lat[t.height() - 1]);
        t.set_right(lon[t.width() -1]);

        // Read this from the 'missing_value' or '_FillValue' attributes
        string missing_value = d_grid->get_attr_table().get_attr("missing_value");
        string fill_value = d_grid->get_attr_table().get_attr("_FillValue");
        if (!missing_value.empty())
            t.set_no_data(missing_value);
        else if (!fill_value.empty())
            t.set_no_data(fill_value);

        // Set the source data type: Always use doubles
        t.set_type(dods_float64_c);

        t.geo_transform_set(true);

        // For GeoTiff there is only one band supported; other return types
        // might support more than one band.
        t.set_num_bands(t.num_bands() + 1);
        t.push_var(this);
    }
    catch (Error &e) {
        throw;
    }
#if 0
    // FIXME
    if (t.is_geo_transform_set()) {
        if (false && ! (90 == t.height()
               && 180 == t.width()
               && 90 == t.top()
               && 21 == t.left()
               && -90 == t.bottom()
               && 180 == t.right()
               && dods_float32_c == t.type()))
        throw Error("To build a GeoTiff response, all variables must have uniform size and location.");
    }
    else {
        // The array size
        t.set_height(90);
        t.set_width(180);

        // Read these from the maps
        t.set_top(-89);
        t.set_left(21);
        t.set_bottom(89);
        t.set_right(379);

        // Read this from the 'missing_value' or '_FillValue' attributes
        t.set_no_data("-9.99999979e+33");
        t.set_type(dods_float32_c);

        t.geo_transform_set(true);
    }

    t.set_num_bands(t.num_bands() + 1);
    t.push_var(this);
#endif
}
