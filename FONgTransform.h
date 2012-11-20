// FONgTransform.h

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

#ifndef FONgTransfrom_h_
#define FONgTransfrom_h_ 1

//#include <string>
//#include <vector>
//#include <map>

//#include <gdal_priv.h>

//#include <DDS.h>
//#include <Array.h>

//using namespace std;
//using namespace libdap;

//#include <BESObj.h>
//#include <BESDataHandlerInterface.h>

//class ConstraintEvaluator;
class FONgBaseType;
class GDALDataset;
class BESDataHandlerInterface;

/** @brief Transformation object that converts an OPeNDAP DataDDS to a
 * GeoTiff file
 *
 * This class transforms variables in a DataDDS object to a GeoTiff file. For
 * more information on the transformation please refer to the OpeNDAP
 * documents wiki.
 */
class FONgTransform : public BESObj
{
private:
    GDALDataset *d_dest;

    libdap::DDS *d_dds;
    libdap::ConstraintEvaluator &d_evaluator;

    string d_localfile;

    vector<FONgBaseType *> d_fong_vars;

    // used when there is more than one variable
    // TODO keep the 'more than one var' feature?
    bool d_geo_transform_set;

    // Collect data here
    double d_width, d_height, d_top, d_left, d_bottom, d_right;
    string d_no_data;

    // Build GeoTransform info here
    double d_gt[6];

    libdap::Type d_type;
    int d_num_bands;

    int d_size_x;   // TODO needed? or use t1 and t5
    int d_size_y;

public:
    FONgTransform(libdap::DDS *dds, libdap::ConstraintEvaluator &evaluator, const string &localfile);
    virtual ~FONgTransform();

    virtual void transform();

    bool is_geo_transform_set() { return d_geo_transform_set; }
    void geo_transform_set(bool state) { d_geo_transform_set = state; }

    libdap::Type type() { return d_type; }
    void set_type(libdap::Type t) { d_type = t; }

    string no_data() { return d_no_data; }
    void set_no_data(const string &nd) { d_no_data = nd; }

    int num_bands() { return d_num_bands; }
    void set_num_bands(int n) { d_num_bands = n; }

    void push_var(FONgBaseType *v) { d_fong_vars.push_back(v); }
    int num_var() { return d_fong_vars.size(); }
#if 0
    typedef vector<FONgBaseType *>::iterator var_iter;
    var_iter var_begin() { return d_fong_vars.begin(); }
    var_iter var_end() { return d_fong_vars.end(); }
#endif
    FONgBaseType *var(int i) { return d_fong_vars.at(i); }

    // Image/band height and width in pixels
    virtual void set_width(int width) { d_width = width; }
    virtual void set_height(int height) { d_height = height; }

    virtual int width() { return d_width; }
    virtual int height() { return d_height; }

    // The top-left corner of the top-left pixel
    virtual void set_top(int top) { d_top = top; }
    virtual void set_left(int left) { d_left = left; }

    virtual double top() { return d_top; }
    virtual double left() { return d_left; }

    // The top-left corner of the top-left pixel
    virtual void set_bottom(int top) { d_bottom = top; }
    virtual void set_right(int left) { d_right = left; }

    virtual double bottom() { return d_bottom; }
    virtual double right() { return d_right; }

    virtual double *geo_transform();

    virtual void dump(ostream &strm) const {}
};

#endif // FONgTransfrom_h_

