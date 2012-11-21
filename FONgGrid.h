// FONgGrid.h

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

#ifndef FONgGrid_h_
#define FONgGrid_h_ 1

#include "FONgBaseType.h"

class FONgTransform;

/** @brief A DAP Grid with file out netcdf information included
 *
 * This class represents a DAP Grid with additional information
 * needed to write it out to a netcdf file. Includes a reference to the
 * actual DAP Grid being converted, the maps of the grid stored as
 * FOncMap instances, and the array of the grid stored as a FONgArray.
 *
 * NetCDF does not have grid representation. For this reason, we flatten
 * out the grid as different arrays (maps) as well as the grid's actual
 * array.
 *
 * It is possible to share maps among grids, so a global map list is
 * kept as well.
 */
class FONgGrid: public FONgBaseType {
private:
    Grid *d_grid;
    Array *d_lat, *d_lon;

    // Sets of string values used to find stuff in attributes
    set<string> d_coards_lat_units;
    set<string> d_coards_lon_units;

    set<string> d_lat_names;
    set<string> d_lon_names;

    bool d_three_dims;

public:
    FONgGrid(Grid *g);
    virtual ~FONgGrid();

    Grid *grid() { return d_grid; }

    bool find_lat_lon_maps();

    bool three_dims() { return d_three_dims; }

    virtual void extract_coordinates(FONgTransform &t);

};

#endif // FONgGrid_h_
