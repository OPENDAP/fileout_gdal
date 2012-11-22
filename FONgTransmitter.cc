// FONgTransmitter.cc

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

#include <cstdlib>

#include <sys/types.h>                  // For umask
#include <sys/stat.h>

#include <iostream>
#include <fstream>

#include <DataDDS.h>
#include <BaseType.h>
#include <escaping.h>

using namespace libdap;

#include "FONgTransmitter.h"
#include "FONgTransform.h"

#include <BESInternalError.h>
#include <BESContextManager.h>
#include <BESDataDDSResponse.h>
#include <BESDapNames.h>
#include <BESDataNames.h>
#include <BESDebug.h>

#include <TheBESKeys.h>

#define FONG_TEMP_DIR "/tmp"
#define FONG_GCS "WGS84"

string FONgTransmitter::temp_dir;
string FONgTransmitter::default_gcs;

/** @brief Construct the FONgTransmitter, adding it with name geotiff to be
 * able to transmit a data response
 *
 * The transmitter is created to add the ability to return OPeNDAP data
 * objects (DataDDS) as a geotiff file.
 *
 * The OPeNDAP data object is written to a geotiff file locally in a
 * temporary directory specified by the BES configuration parameter
 * FONg.Tempdir. If this variable is not found or is not set then it
 * defaults to the macro definition FONG_TEMP_DIR.
 *
 * @note The mapping from a 'returnAs' of "geotiff" to this code
 * is made in the FONgModule class.
 *
 * @see FONgModule
 */
FONgTransmitter::FONgTransmitter() :  BESBasicTransmitter()
{
    // DATA_SERVICE == "dods"
    add_method(DATA_SERVICE, FONgTransmitter::send_data);

    if (FONgTransmitter::temp_dir.empty()) {
        // Where is the temp directory for creating these files
        bool found = false;
        string key = "FONg.Tempdir";
        TheBESKeys::TheKeys()->get_value(key, FONgTransmitter::temp_dir, found);
        if (!found || FONgTransmitter::temp_dir.empty()) {
            FONgTransmitter::temp_dir = FONG_TEMP_DIR;
        }
        string::size_type len = FONgTransmitter::temp_dir.length();
        if (FONgTransmitter::temp_dir[len - 1] == '/') {
            FONgTransmitter::temp_dir = FONgTransmitter::temp_dir.substr(0, len - 1);
        }
    }

    if (FONgTransmitter::default_gcs.empty()) {
        // Use what as teh default Geographic coordinate system?
        bool found = false;
        string key = "FONg.Default_GCS";
        TheBESKeys::TheKeys()->get_value(key, FONgTransmitter::default_gcs, found);
        if (!found || FONgTransmitter::default_gcs.empty()) {
            FONgTransmitter::default_gcs = FONG_GCS;
        }
    }
}

/** @brief The static method registered to transmit OPeNDAP data objects as
 * a netcdf file.
 *
 * This function takes the OPeNDAP DataDDS object, reads in the data (can be
 * used with any data handler), transforms the data into a netcdf file, and
 * streams back that netcdf file back to the requester using the stream
 * specified in the BESDataHandlerInterface.
 *
 * @param obj The BESResponseObject containing the OPeNDAP DataDDS object
 * @param dhi BESDataHandlerInterface containing information about the
 * request and response
 * @throws BESInternalError if the response is not an OPeNDAP DataDDS or if
 * there are any problems reading the data, writing to a netcdf file, or
 * streaming the netcdf file
 */
void FONgTransmitter::send_data(BESResponseObject *obj, BESDataHandlerInterface &dhi)
{
    BESDataDDSResponse *bdds = dynamic_cast<BESDataDDSResponse *>(obj);
    if (!bdds) {
        throw BESInternalError("cast error", __FILE__, __LINE__);
    }

    DataDDS *dds = bdds->get_dds();
    if (!dds) {
        string err = (string) "No DataDDS has been created for transmit";
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }

    ostream &strm = dhi.get_output_stream();
    if (!strm) {
        string err = (string) "Output stream is not set, cannot return as";
        BESInternalError pe(err, __FILE__, __LINE__);
        throw pe;
    }

    BESDEBUG("fong2", "FONgTransmitter::send_data - parsing the constraint" << endl);

    // ticket 1248 jhrg 2/23/09
    string ce = www2id(dhi.data[POST_CONSTRAINT], "%", "%20%26");
    try {
        bdds->get_ce().parse_constraint(ce, *dds);
    }
    catch (Error &e) {
        string em = e.get_error_message();
        string err = "Failed to parse the constraint expression: " + em;
        throw BESInternalError(err, __FILE__, __LINE__);
    }
    catch (...) {
        string err = (string) "Failed to parse the constraint expression: " + "Unknown exception caught";
        throw BESInternalError(err, __FILE__, __LINE__);
    }

    // now we need to read the data
    BESDEBUG("fong2", "FONgTransmitter::send_data - reading data into DataDDS" << endl);

    string temp_file_name = FONgTransmitter::temp_dir + '/' + "geotiffXXXXXX";
    vector<char> temp_full(temp_file_name.length() + 1);
    string::size_type len = temp_file_name.copy(&temp_full[0], temp_file_name.length());
    temp_full[len] = '\0';
    // cover the case where older versions of mkstemp() create the file using
    // a mode of 666.
    mode_t original_mode = umask(077);
    int fd = mkstemp(&temp_full[0]);
    umask(original_mode);

    if (fd == -1)
        throw BESInternalError("Failed to open the temporary file: " + temp_file_name, __FILE__, __LINE__);

    // transform the OPeNDAP DataDDS to the geotiff file
    BESDEBUG("fong2", "FONgTransmitter::send_data - transforming into temporary file " << &temp_full[0] << endl);

    try {
        FONgTransform ft(dds, bdds->get_ce(), &temp_full[0]);

        // transform() opens the temporary file, dumps data to it and closes it.
        ft.transform();

        BESDEBUG("fong2", "FONgTransmitter::send_data - transmitting temp file " << &temp_full[0] << endl );

        FONgTransmitter::return_temp_stream(&temp_full[0], strm);
    }
    catch (BESError &e) {
        close(fd);
        (void) unlink(&temp_full[0]);
        throw;
    }
    catch (Error &e) {
        close(fd);
        (void) unlink(&temp_full[0]);
        throw BESInternalError(e.get_error_message(), __FILE__, __LINE__);
        throw;
    }

    catch (...) {
        close(fd);
        (void) unlink(&temp_full[0]);
        throw BESInternalError("Fileout GeoTiff, was not able to transform to geotiff, unknown error", __FILE__, __LINE__);
    }

    close(fd);
    (void) unlink(&temp_full[0]);

    BESDEBUG("fong2", "FONgTransmitter::send_data - done transmitting to geotiff" << endl);
}

/** @brief stream the temporary file back to the requester
 *
 * Streams the temporary file specified by filename to the specified
 * C++ ostream
 *
 * @param filename The name of the file to stream back to the requester
 * @param strm C++ ostream to write the contents of the file to
 * @throws BESInternalError if problem opening the file
 */
void FONgTransmitter::return_temp_stream(const string &filename, ostream &strm)
{
    ifstream os;
    os.open(filename.c_str(), ios::binary | ios::in);
    if (!os)
        throw BESInternalError("Cannot connect to data source", __FILE__, __LINE__);

    char block[4096];
    os.read(block, sizeof block);
    int nbytes = os.gcount();
    if (nbytes == 0) {
        os.close();
        throw BESInternalError("Internal server error, got zero count on stream buffer.", __FILE__, __LINE__);
    }

    bool found = false;
    string context = "transmit_protocol";
    string protocol = BESContextManager::TheManager()->get_context(context, found);
    if (protocol == "HTTP") {
        strm << "HTTP/1.0 200 OK\n";
        strm << "Content-type: application/octet-stream\n";
        strm << "Content-Description: " << "BES dataset" << "\n";
        strm << "Content-Disposition: filename=" << filename << ".tif;\n\n";
        strm << flush;
    }
    strm.write(block, nbytes);

    while (os) {
        os.read(block, sizeof block);
        nbytes = os.gcount();
        strm.write(block, nbytes);
    }

    os.close();
}

