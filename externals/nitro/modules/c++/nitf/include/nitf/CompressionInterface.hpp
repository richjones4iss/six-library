/* =========================================================================
 * This file is part of NITRO
 * =========================================================================
 *
 * (C) Copyright 2004 - 2014, MDA Information Systems LLC
 *
 * NITRO is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#ifndef __NITF_COMPRESSION_INTERFACE_HPP__
#define __NITF_COMPRESSION_INTERFACE_HPP__

#include <nitf/ImageSubheader.hpp>
#include <nitf/IOInterface.hpp>
#include <nitf/ImageIO.h>


/*!
 *  This is a macro for quickly exposing hooks to a c++ layer
 *  Compressor object. The idea here is to setup everything in
 *  one shot. All the user has to do is create a
 *  nitf_CompressionInterface 'open' method, and declare the
 *  identifier for compression type.
 */
#define NITF_CREATE_CUSTOM_COMPRESSION(_COMPRESSION_ID, \
                                       _COMPRESSION_DLL_NAME, \
                                       _COMPRESSION_ADAPTER_OPEN_FUNC) \
NITF_CXX_GUARD \
static const char* _COMPRESSION_ID##_ident[] = \
{\
    NITF_PLUGIN_COMPRESSION_KEY, #_COMPRESSION_ID, NULL\
};\
\
static nitf_CompressionInterface _COMPRESSION_ID##_INTERFACE_TABLE = {\
    &_COMPRESSION_ADAPTER_OPEN_FUNC,\
    &::nitf::CompressionInterface::adapterStart,\
    &::nitf::CompressionInterface::adapterWriteBlock,\
    &::nitf::CompressionInterface::adapterEnd,\
    &::nitf::CompressionInterface::adapterDestroy};\
\
NITFAPI(const char**) _COMPRESSION_DLL_NAME##_init(nitf_Error *error)\
{\
    return _COMPRESSION_ID##_ident;\
}\
\
NITFAPI(void) _COMPRESSION_ID##_cleanup(void)\
{\
}\
\
NITFAPI(void*) _COMPRESSION_ID##_construct(const char* compressionType,\
                                           nitf_Error* error)\
{\
    if (strcmp(compressionType, #_COMPRESSION_ID) != 0)\
    {\
        nitf_Error_init(error,\
                        "Unsupported compression type",\
                        NITF_CTXT,\
                        NITF_ERR_COMPRESSION);\
        return NULL;\
    }\
    return &_COMPRESSION_ID##_INTERFACE_TABLE;\
}\
NITF_CXX_ENDGUARD

namespace nitf
{

/*!
 *  \class CompressionInterface
 *  \brief Compression object controls all innerworkings
 *         during compression, and gives a c++ api for getting
 *         things done.
 */
class CompressionInterface
{
public:

    //! These are canned methods which turn around
    //  and call the nitf_CompressionControl of your choice
    static NITF_BOOL adapterStart(nitf_CompressionControl* object,
                                  nitf::Uint64 offset,
                                  nitf::Uint64 dataLength,
                                  nitf::Uint64* blockMask,
                                  nitf::Uint64* padMask,
                                  nitf_Error* error);

    static NITF_BOOL adapterWriteBlock(nitf_CompressionControl* object,
                                       nitf_IOInterface* io,
                                       const nitf_Uint8* data,
                                       NITF_BOOL pad,
                                       NITF_BOOL noData,
                                       nitf_Error* error);

    static NITF_BOOL adapterEnd(nitf_CompressionControl* object,
                                nitf_IOInterface* io,
                                nitf_Error* error);

    static void adapterDestroy(nitf_CompressionControl** object);

};

/*!
 *  \class Compressor
 *  \brief This is the c++ interface for nitf_CompressionControl
 */
class Compressor
{
public:
    Compressor() {}
    virtual ~Compressor() {}

    virtual void start(nitf::Uint64 offset,
                       nitf::Uint64 dataLength,
                       nitf::Uint64* blockMask,
                       nitf::Uint64* padMask) = 0;

    virtual void writeBlock(nitf::IOInterface& io,
                            const nitf::Uint8* data,
                            bool pad,
                            bool noData) = 0;

    virtual void end(nitf::IOInterface& io) = 0;
};

}

#endif
