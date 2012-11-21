// FONgBaseType.h

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

#ifndef FONgBaseType_h_
#define FONgBaseType_h_ 1

#include <BESObj.h>
#include <BaseType.h>

// using namespace libdap ;

class FONgTransform;
class GDALDataset;

/** @brief A DAP BaseType with file out gdal information included
 *
 * This class represents a DAP BaseType with additional information
 * needed to write it out to a gdal file. Includes a reference to the
 * actual DAP BaseType being converted
 */
class FONgBaseType: public BESObj {
protected:
    string d_name;
    libdap::Type d_type;

public:
    FONgBaseType() {}

    virtual ~FONgBaseType() {}

    virtual string name() { return d_name; }
    virtual string set_name(const string &n) { d_name = n; }

    virtual void extract_coordinates(FONgTransform &t) = 0;
    virtual void set_projection(libdap::DDS *dds, GDALDataset *dest) = 0;

    /** Get the data values for the band(s). Call must delete. */
    virtual double *get_data() = 0;

    virtual void dump(ostream &strm) const {};
};

#endif // FONgBaseType_h_

