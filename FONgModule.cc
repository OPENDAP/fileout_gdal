// FONgModule.cc

// This file is part of BES GDAL Fileout Module.

#include "config.h"

#include <iostream>

using std::endl;

#include "FONgModule.h"
#include "FONgTransmitter.h"
#include "FONgRequestHandler.h"
#include "BESRequestHandlerList.h"

#include <BESReturnManager.h>

#include <BESServiceRegistry.h>
#include <BESDapNames.h>

#include <TheBESKeys.h>
#include <BESDebug.h>

#define RETURNAS_GEOTIFF "geotiff"

/** @brief initialize the module by adding call backs and registering
 * objects with the framework
 *
 * Registers the request handler to add to a version or help request,
 * and adds the File Out transmitter for a "returnAs netcdf" request.
 * Also adds netcdf as a return for the dap service dods request and
 * registers the debug context.
 *
 * @param modname The name of the module being loaded
 */
void FONgModule::initialize(const string &modname)
{
    BESDEBUG( "fong", "Initializing module " << modname << endl );

    BESRequestHandler *handler = new FONgRequestHandler(modname);
    BESRequestHandlerList::TheList()->add_handler(modname, handler);

    BESDEBUG( "fong", "    adding " << RETURNAS_GEOTIFF << " transmitter" << endl );

    BESReturnManager::TheManager()->add_transmitter(RETURNAS_GEOTIFF, new FONgTransmitter());

    BESDEBUG( "fong", "    adding fong service to dap" << endl );

    BESServiceRegistry::TheRegistry()->add_format(OPENDAP_SERVICE, DATA_SERVICE, RETURNAS_GEOTIFF);

    // TODO Move to the top of the method?
    BESDebug::Register("fong");

    BESDEBUG( "fong", "Done Initializing module " << modname << endl );
}

/** @brief removes any registered callbacks or objects from the
 * framework
 *
 * Any registered callbacks or objects, registered during
 * initialization, are removed from the framework.
 *
 * @param modname The name of the module being removed
 */
void FONgModule::terminate(const string &modname)
{
    BESDEBUG( "fong", "Cleaning module " << modname << endl );
    BESDEBUG( "fong", "    removing " << RETURNAS_GEOTIFF << " transmitter" << endl );

    BESReturnManager::TheManager()->del_transmitter(RETURNAS_GEOTIFF);

    BESDEBUG( "fong", "    removing " << modname << " request handler " << endl );

    BESRequestHandler *rh = BESRequestHandlerList::TheList()->remove_handler(modname);
    if (rh)
        delete rh;

    BESDEBUG( "fong", "Done Cleaning module " << modname << endl );
}

/** @brief dumps information about this object for debugging purposes
 *
 * Displays the pointer value of this instance plus any instance data
 *
 * @param strm C++ i/o stream to dump the information to
 */
void FONgModule::dump(ostream &strm) const
{
    strm << BESIndent::LMarg << "FONgModule::dump - (" << (void *) this << ")" << endl;
}

/** @brief A c function that adds this module to the list of modules to
 * be dynamically loaded.
 */
extern "C"
BESAbstractModule *maker()
{
    return new FONgModule;
}

