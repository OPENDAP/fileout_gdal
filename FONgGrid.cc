// FONgGrid.cc

// This file is part of BES Netcdf File Out Module

// Copyright (c) 2004,2005 University Corporation for Atmospheric Research
// Author: Patrick West <pwest@ucar.edu> and Jose Garcia <jgarcia@ucar.edu>
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

// (c) COPYRIGHT University Corporation for Atmospheric Research 2004-2005
// Please read the full copyright statement in the file COPYRIGHT_UCAR.
//
// Authors:
//      pwest       Patrick West <pwest@ucar.edu>
//      jgarcia     Jose Garcia <jgarcia@ucar.edu>

#include <BESInternalError.h>
#include <BESDebug.h>

#include <Grid.h>

#include "FONgTransform.h"
#include "FONgGrid.h"

using namespace libdap;

/** @brief Constructor for FONgGrid that takes a DAP Grid
 *
 * @param g A DAP BaseType that should be a grid
 */
FONgGrid::FONgGrid(Grid *g) : FONgBaseType(), d_grid(g)
{
}

/** @brief Destructor that cleans up the grid
 *
 * The DAP Grid instance does not belong to the FONgGrid instance, so it
 * is not deleted.
 */
FONgGrid::~FONgGrid()
{
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

    // How many dimensions are in the constrained grid?
    d_three_dims = d_grid->get_array()->dimensions(true) == 3;

    // Find the lat and lon map vectors
    Grid::Map_iter i = d_grid->map_begin();
    while (i != d_grid->map_end()) {
        // FIXME Find the lat and lon dimensions
        // and make sure that the Grid has been properly constrained
        ++i;
    }

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
}
