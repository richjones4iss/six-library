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

#include <string.h>

#include <nitf/ComplexityLevel.h>
#include <nitf/Record.hpp>

namespace nitf
{
Record::Record(const Record & x)
{
    setNative(x.getNative());
}

Record & Record::operator=(const Record & x)
{
    if (&x != this)
        setNative(x.getNative());
    return *this;
}

Record::Record(nitf_Record * x)
{
    setNative(x);
    getNativeOrThrow();
}

Record::Record(nitf::Version version) throw(nitf::NITFException)
{
    setNative(nitf_Record_construct(version, &error));
    getNativeOrThrow();
    setManaged(false);
}

nitf::Record Record::clone() const throw(nitf::NITFException)
{
    nitf::Record dolly(nitf_Record_clone(getNativeOrThrow(), &error));
    dolly.setManaged(false);
    return dolly;
}

Record::~Record()
{
}

nitf::Version Record::getVersion() const
{
    return nitf_Record_getVersion(getNativeOrThrow());
}

nitf::FileHeader Record::getHeader()
{
    return nitf::FileHeader(getNativeOrThrow()->header);
}

void Record::setHeader(nitf::FileHeader & value)
{
    //release the one currently "owned"
    nitf::FileHeader fh = nitf::FileHeader(getNativeOrThrow()->header);
    fh.setManaged(false);

    //have the library manage the "new" one
    getNativeOrThrow()->header = value.getNative();
    value.setManaged(true);
}

nitf::Uint32 Record::getNumImages() const
{
    nitf::Uint32 num = nitf_Record_getNumImages(getNativeOrThrow(), &error);
    
    if (NITF_INVALID_NUM_SEGMENTS( num ))
        throw nitf::NITFException(&error);
    
    return num;
}
    
nitf::Uint32 Record::getNumGraphics() const
{

    nitf::Uint32 num = nitf_Record_getNumGraphics(getNativeOrThrow(), &error);

    if (NITF_INVALID_NUM_SEGMENTS( num ))
        throw nitf::NITFException(&error);

    return num;
}

nitf::Uint32 Record::getNumLabels() const
{

    nitf::Uint32 num = nitf_Record_getNumLabels(getNativeOrThrow(), &error);

    if (NITF_INVALID_NUM_SEGMENTS( num ))
        throw nitf::NITFException(&error);

    return num;
}

nitf::Uint32 Record::getNumTexts() const
{
    nitf::Uint32 num = nitf_Record_getNumTexts(getNativeOrThrow(), &error);

    if (NITF_INVALID_NUM_SEGMENTS( num ))
        throw nitf::NITFException(&error);

    return num;
}

nitf::Uint32 Record::getNumDataExtensions() const
{
    nitf::Uint32 num = nitf_Record_getNumDataExtensions(getNativeOrThrow(), 
                                                        &error);

    if (NITF_INVALID_NUM_SEGMENTS( num ))
        throw nitf::NITFException(&error);

    return num;
}

nitf::Uint32 Record::getNumReservedExtensions() const
{
    nitf::Uint32 num = nitf_Record_getNumReservedExtensions(getNativeOrThrow(), 
                                                            &error);

    if (NITF_INVALID_NUM_SEGMENTS( num ))
        throw nitf::NITFException(&error);

    return num;
}


nitf::List Record::getImages()
{
    return nitf::List(getNativeOrThrow()->images);
}

nitf::List Record::getGraphics()
{
    return nitf::List(getNativeOrThrow()->graphics);
}

nitf::List Record::getLabels()
{
    return nitf::List(getNativeOrThrow()->labels);
}

nitf::List Record::getTexts()
{
    return nitf::List(getNativeOrThrow()->texts);
}

nitf::List Record::getDataExtensions()
{
    return nitf::List(getNativeOrThrow()->dataExtensions);
}

nitf::List Record::getReservedExtensions()
{
    return nitf::List(getNativeOrThrow()->reservedExtensions);
}

nitf::ImageSegment Record::newImageSegment(int index)
{
    nitf_ImageSegment* x = nitf_Record_newImageSegment(getNativeOrThrow(), &error);
    if (!x)
        throw nitf::NITFException(&error);
    if (index >= 0) //move it, if we need to
        moveImageSegment(getImages().getSize() - 1, index);
    return nitf::ImageSegment(x);
}

nitf::GraphicSegment Record::newGraphicSegment(int index)
{
    nitf_GraphicSegment* x = nitf_Record_newGraphicSegment(getNativeOrThrow(), &error);
    if (!x)
        throw nitf::NITFException(&error);
    if (index >= 0) //move it, if we need to
        moveGraphicSegment(getGraphics().getSize() - 1, index);
    return nitf::GraphicSegment(x);
}

nitf::TextSegment Record::newTextSegment(int index)
{
    nitf_TextSegment* x = nitf_Record_newTextSegment(getNativeOrThrow(), &error);
    if (!x)
        throw nitf::NITFException(&error);
    if (index >= 0) //move it, if we need to
        moveTextSegment(getTexts().getSize() - 1, index);
    return nitf::TextSegment(x);
}

nitf::DESegment Record::newDataExtensionSegment(int index)
{
    nitf_DESegment* x = nitf_Record_newDataExtensionSegment(getNativeOrThrow(), &error);
    if (!x)
        throw nitf::NITFException(&error);
    if (index >= 0) //move it, if we need to
        moveDataExtensionSegment(getDataExtensions().getSize() - 1, index);
    return nitf::DESegment(x);
}

void Record::removeImageSegment(nitf::Uint32 segmentNumber)
{
    if (NITF_SUCCESS != nitf_Record_removeImageSegment(getNativeOrThrow(), segmentNumber, &error))
        throw nitf::NITFException(&error);
}

void Record::removeGraphicSegment(nitf::Uint32 segmentNumber)
{
    if (NITF_SUCCESS != nitf_Record_removeGraphicSegment(getNativeOrThrow(), segmentNumber, &error))
        throw nitf::NITFException(&error);
}

void Record::removeTextSegment(nitf::Uint32 segmentNumber)
{
    if (NITF_SUCCESS != nitf_Record_removeTextSegment(getNativeOrThrow(), segmentNumber, &error))
        throw nitf::NITFException(&error);
}

void Record::removeLabelSegment(nitf::Uint32 segmentNumber)
{
    if (NITF_SUCCESS != nitf_Record_removeLabelSegment(getNativeOrThrow(), segmentNumber, &error))
        throw nitf::NITFException(&error);
}

void Record::removeDataExtensionSegment(nitf::Uint32 segmentNumber)
{
    if (NITF_SUCCESS != nitf_Record_removeDataExtensionSegment(getNativeOrThrow(), segmentNumber, &error))
        throw nitf::NITFException(&error);
}

void Record::removeReservedExtensionSegment(nitf::Uint32 segmentNumber)
{
    if (NITF_SUCCESS != nitf_Record_removeReservedExtensionSegment(getNativeOrThrow(), segmentNumber, &error))
        throw nitf::NITFException(&error);
}

void Record::moveImageSegment(nitf::Uint32 oldIndex, int newIndex)
{
    if (NITF_SUCCESS != nitf_Record_moveImageSegment(getNativeOrThrow(),
        oldIndex, newIndex, &error))
        throw nitf::NITFException(&error);
}

void Record::moveTextSegment(nitf::Uint32 oldIndex, int newIndex)
{
    if (NITF_SUCCESS != nitf_Record_moveTextSegment(getNativeOrThrow(),
        oldIndex, newIndex, &error))
        throw nitf::NITFException(&error);
}

void Record::moveGraphicSegment(nitf::Uint32 oldIndex, int newIndex)
{
    if (NITF_SUCCESS != nitf_Record_moveGraphicSegment(getNativeOrThrow(),
        oldIndex, newIndex, &error))
        throw nitf::NITFException(&error);
}

void Record::moveDataExtensionSegment(nitf::Uint32 oldIndex, int newIndex)
{
    if (NITF_SUCCESS != nitf_Record_moveDataExtensionSegment(getNativeOrThrow(),
        oldIndex, newIndex, &error))
        throw nitf::NITFException(&error);
}

void Record::moveLabelSegment(nitf::Uint32 oldIndex, int newIndex)
{
    if (NITF_SUCCESS != nitf_Record_moveLabelSegment(getNativeOrThrow(),
        oldIndex, newIndex, &error))
        throw nitf::NITFException(&error);
}

void Record::moveReservedExtensionSegment(nitf::Uint32 oldIndex, int newIndex)
{
    if (NITF_SUCCESS != nitf_Record_moveReservedExtensionSegment(getNativeOrThrow(),
        oldIndex, newIndex, &error))
        throw nitf::NITFException(&error);
}

void Record::setComplexityLevelIfUnset()
{
    // Fill out the complexity level if they didn't set it
    char* const clevelRaw =
            getHeader().getComplianceLevel().getRawData();

    if (!strncmp(clevelRaw, "00", 2))
    {
        const NITF_CLEVEL clevel =
                nitf_ComplexityLevel_measure(getNativeOrThrow(), &error);

        if (clevel == NITF_CLEVEL_CHECK_FAILED)
        {
            throw nitf::NITFException(&error);
        }

        nitf_ComplexityLevel_toString(clevel, clevelRaw);
    }
}
}
