// FONgBaseType.h

// This file is part of BES GDAL File Out Module

#ifndef FONgBaseType_h_
#define FONgBaseType_h_ 1

#include <BESObj.h>
#include <BaseType.h>

using namespace libdap ;

class FONgTransform;

/** @brief A DAP BaseType with file out gdal information included
 *
 * This class represents a DAP BaseType with additional information
 * needed to write it out to a gdal file. Includes a reference to the
 * actual DAP BaseType being converted
 */
class FONgBaseType: public BESObj {
protected:

    string d_name;

public:
    FONgBaseType() {}

    virtual ~FONgBaseType() {}

    virtual string name() { return d_name; }
    virtual string set_name(const string &n) { d_name = n; }

    virtual void extract_coordinates(FONgTransform &t) = 0;

    virtual void dump(ostream &strm) const {};
};

#endif // FONgBaseType_h_

