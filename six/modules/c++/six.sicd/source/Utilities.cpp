/* =========================================================================
 * This file is part of six.sicd-c++
 * =========================================================================
 *
 * (C) Copyright 2004 - 2014, MDA Information Systems LLC
 *
 * six.sicd-c++ is free software; you can redistribute it and/or modify
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
 * License along with this program; If not,
 * see <http://www.gnu.org/licenses/>.
 *
 */

#include <io/StringStream.h>
#include <six/Utilities.h>
#include <six/NITFReadControl.h>
#include <six/sicd/AreaPlaneUtility.h>
#include <six/sicd/ComplexXMLControl.h>
#include <six/sicd/Utilities.h>

#include <math/Utilities.h>
#include <types/RowCol.h>

namespace
{
void getErrors(const six::sicd::ComplexData& data,
               scene::Errors& errors)
{
    six::getErrors(data.errorStatistics.get(),
                   types::RgAz<double>(data.grid->row->sampleSpacing,
                                       data.grid->col->sampleSpacing),
                   errors);
}

template <typename ValueType>
six::Region buildRegion(const types::RowCol<size_t>& offset,
                        const types::RowCol<size_t>& extent,
                        ValueType* buffer)
{
    six::Region retv;
    retv.setStartRow(offset.row);
    retv.setStartCol(offset.col);
    retv.setNumRows(extent.row);
    retv.setNumCols(extent.col);
    retv.setBuffer(reinterpret_cast<six::UByte*>(buffer));
    return retv;
}

// Reads in ~32 MB of rows at a time, converts to complex<float>, and keeps
// going until reads everything
void readAndConvertSICD(six::NITFReadControl& reader,
                        size_t imageNumber,
                        const types::RowCol<size_t>& offset,
                        const types::RowCol<size_t>& extent,
                        std::complex<float>* buffer)
{


    // One for the real component, one for imaginary of each pixel
    const size_t elementsPerRow = extent.col * 2;

    // Get at least 32MB per read
    const size_t rowsAtATime =
            (32000000 / (elementsPerRow * sizeof(short))) + 1;

    // Allocate temp buffer
    std::vector<short> tempVector(elementsPerRow * rowsAtATime);
    short* const tempBuffer = &tempVector[0];

    const size_t endRow = offset.row + extent.row;

    for (size_t row = offset.row, rowsToRead = rowsAtATime;
         row < endRow;
         row += rowsToRead)
    {

        // If we would read beyond the input buffer, don't
        if (row + rowsToRead > endRow)
        {
            rowsToRead = endRow - row;
        }

        // Read into the temp buffer
        types::RowCol<size_t> swathOffset(row, offset.col);
        types::RowCol<size_t> swathExtent(rowsToRead, extent.col);
        six::Region region = buildRegion(swathOffset, swathExtent, tempBuffer);
        reader.interleaved(region, imageNumber);

        // Take each Int16 out of the temp buffer and put it into the real
        // buffer as a Float32
        float* const bufferPtr = reinterpret_cast<float*>(buffer) +
                ((row - offset.row) * elementsPerRow);

        for (size_t index = 0; index < elementsPerRow * rowsToRead; index++)
        {
            bufferPtr[index] = tempBuffer[index];
        }
    }
}
}

namespace six
{
namespace sicd
{
scene::SceneGeometry*
Utilities::getSceneGeometry(const ComplexData* data)
{
    scene::SceneGeometry *geom =
            new scene::SceneGeometry(data->scpcoa->arpVel,
                                     data->scpcoa->arpPos,
                                     data->geoData->scp.ecf,
                                     data->grid->row->unitVector,
                                     data->grid->col->unitVector);

    return geom;
}

scene::ProjectionModel*
Utilities::getProjectionModel(const ComplexData* data,
                              const scene::SceneGeometry* geom)
{
    const six::ComplexImageGridType gridType = data->grid->type;
    const int lookDir = (data->scpcoa->sideOfTrack == 1) ? 1 : -1;

    // Parse error statistics so we can pass these to the projection model as
    // well
    scene::Errors errors;
    ::getErrors(*data, errors);

    switch ((int) gridType)
    {
    case six::ComplexImageGridType::RGAZIM:
        return new scene::RangeAzimProjectionModel(
                                                   data->pfa->polarAnglePoly,
                                                   data->pfa->spatialFrequencyScaleFactorPoly,
                                                   geom->getSlantPlaneZ(),
                                                   data->grid->row->unitVector,
                                                   data->grid->col->unitVector,
                                                   data->geoData->scp.ecf,
                                                   data->position->arpPoly,
                                                   data->grid->timeCOAPoly,
                                                   lookDir,
                                                   errors);

    case six::ComplexImageGridType::RGZERO:
        return new scene::RangeZeroProjectionModel(
                                                   data->rma->inca->timeCAPoly,
                                                   data->rma->inca->dopplerRateScaleFactorPoly,
                                                   data->rma->inca->rangeCA,
                                                   geom->getSlantPlaneZ(),
                                                   data->grid->row->unitVector,
                                                   data->grid->col->unitVector,
                                                   data->geoData->scp.ecf,
                                                   data->position->arpPoly,
                                                   data->grid->timeCOAPoly,
                                                   lookDir,
                                                   errors);
    case six::ComplexImageGridType::XRGYCR:
        // Note: This case has not been tested due to a lack of test data
        return new scene::XRGYCRProjectionModel(geom->getSlantPlaneZ(),
                                                data->grid->row->unitVector,
                                                data->grid->col->unitVector,
                                                data->geoData->scp.ecf,
                                                data->position->arpPoly,
                                                data->grid->timeCOAPoly,
                                                lookDir,
                                                errors);
    case six::ComplexImageGridType::XCTYAT:
        // Note: This case has not been tested due to a lack of test data
        return new scene::XCTYATProjectionModel(geom->getSlantPlaneZ(),
                                                data->grid->row->unitVector,
                                                data->grid->col->unitVector,
                                                data->geoData->scp.ecf,
                                                data->position->arpPoly,
                                                data->grid->timeCOAPoly,
                                                lookDir,
                                                errors);
    case six::ComplexImageGridType::PLANE:
        // Note: This case has not been tested due to a lack of test data
        return new scene::PlaneProjectionModel(geom->getSlantPlaneZ(),
                                               data->grid->row->unitVector,
                                               data->grid->col->unitVector,
                                               data->geoData->scp.ecf,
                                               data->position->arpPoly,
                                               data->grid->timeCOAPoly,
                                               lookDir,
                                               errors);
    default:
        throw except::Exception(Ctxt("Invalid grid type: " +
                gridType.toString()));
    }
}

std::auto_ptr<scene::ProjectionPolynomialFitter>
Utilities::getPolynomialFitter(const ComplexData& complexData)
{
    std::auto_ptr<scene::SceneGeometry> geometry(
            getSceneGeometry(&complexData));
    std::auto_ptr<scene::ProjectionModel> projectionModel(
            getProjectionModel(&complexData, geometry.get()));
    AreaPlane areaPlane;
    if (AreaPlaneUtility::hasAreaPlane(complexData))
    {
        areaPlane = *complexData.radarCollection->area->plane;
    }
    else
    {
        AreaPlaneUtility::deriveAreaPlane(complexData, areaPlane);
    }

    const RowColDouble sampleSpacing(areaPlane.xDirection->spacing,
            areaPlane.yDirection->spacing);
    const scene::PlanarGridECEFTransform ecefTransform(
            sampleSpacing,
            areaPlane.referencePoint.rowCol,
            areaPlane.xDirection->unitVector,
            areaPlane.yDirection->unitVector,
            areaPlane.referencePoint.ecef);

    types::RowCol<size_t> offset;
    types::RowCol<size_t> extent;
    complexData.getOutputPlaneOffsetAndExtent(areaPlane, offset, extent);
    return std::auto_ptr<scene::ProjectionPolynomialFitter>(
            new scene::ProjectionPolynomialFitter(
                *projectionModel,
                ecefTransform,
                offset,
                extent));
}

void Utilities::getValidDataPolygon(
        const ComplexData& sicdData,
        const scene::ProjectionModel& projection,
        std::vector<types::RowCol<double> >& validData)
{
    validData.clear();

    const std::vector<six::RowColInt>& origValidData =
            sicdData.imageData->validData;

    if (!origValidData.empty())
    {
        // Life is easy - we're just casting
        validData.resize(origValidData.size());
        for (size_t ii = 0; ii < origValidData.size(); ++ii)
        {
            validData[ii] = origValidData[ii];
        }
    }
    else if (sicdData.radarCollection->area.get() &&
             sicdData.radarCollection->area->plane.get())
    {
        // We have an output plane defined, so project the four output plane
        // corners back to the slant plane and that should approximate the
        // valid data polygon

        // Output plane corners
        types::RowCol<size_t> opOffset;
        types::RowCol<size_t> opDims;
        sicdData.getOutputPlaneOffsetAndExtent(opOffset, opDims);

        const types::RowCol<double> lastOPPixel(opOffset + opDims - 1);

        std::vector<types::RowCol<double> > opCorners(4);
        opCorners[0] = opOffset;
        opCorners[1] = types::RowCol<double>(static_cast<double>(opOffset.row),
				static_cast<double>(lastOPPixel.col));
        opCorners[2] = types::RowCol<double>(
				static_cast<double>(lastOPPixel.row),
				static_cast<double>(opOffset.col));
        opCorners[3] = lastOPPixel;

        const six::sicd::AreaPlane& areaPlane =
                *sicdData.radarCollection->area->plane;

        const six::sicd::AreaDirectionParameters& x(*areaPlane.xDirection);
        const six::sicd::AreaDirectionParameters& y(*areaPlane.yDirection);

        // Normalize the output plane unit vectors just in case
        six::Vector3 opRowUnitVector = x.unitVector;
        six::Vector3 opColUnitVector = y.unitVector;
        opRowUnitVector.normalize();
        opColUnitVector.normalize();

        const scene::PlanarGridECEFTransform gridTransform(
                types::RowCol<double>(x.spacing, y.spacing),
                areaPlane.getAdjustedReferencePoint(),
                opRowUnitVector,
                opColUnitVector,
                areaPlane.referencePoint.ecef);

        const types::RowCol<double> spSCP(sicdData.imageData->scpPixel);
        const types::RowCol<double> spOrigOffset(
				static_cast<double>(sicdData.imageData->firstRow),
				static_cast<double>(sicdData.imageData->firstCol));

        const types::RowCol<double> spOffset(
                spSCP.row - spOrigOffset.row,
                spSCP.col - spOrigOffset.col);

        const six::sicd::Grid& grid(*sicdData.grid);
        const types::RowCol<double> spSampleSpacing(grid.row->sampleSpacing,
                                                    grid.col->sampleSpacing);

        validData.resize(opCorners.size());
        bool spCornerIsInBounds = false;
        for (size_t ii = 0; ii < opCorners.size(); ++ii)
        {
            // Convert OP pixel to ECEF
            const scene::Vector3 sPos = gridTransform.rowColToECEF(opCorners[ii]);

            // Convert this to a continuous surface point in the slant plane
            double timeCOA(0.0);
            const types::RowCol<double> imagePt =
                    projection.sceneToImage(sPos, &timeCOA);

            // Convert this to pixels
            // Need to offset by both the SCP and account for an AOI SICD
            types::RowCol<double>& spCorner(validData[ii]);
            spCorner = (imagePt / spSampleSpacing + spOffset);

            if (spCorner.row >= 0 &&
                spCorner.row < sicdData.imageData->numRows &&
                spCorner.col >= 0 &&
                spCorner.col < sicdData.imageData->numCols)
            {
                spCornerIsInBounds = true;
            }
        }

        // If all of the points are out of bounds, the whole image is valid,
        // so don't bother to keep this
        if (!spCornerIsInBounds)
        {
            validData.clear();
        }
    }

    // NOTE: If we don't have an output plane defined, do nothing.  Note that
    //       usually if an output plane isn't defined in the SICD, we define
    //       one ourselves by projecting the slant plane corners into the
    //       output plane.  So projecting those back into the slant plane
    //       isn't going to tell us anything new...
}

void Utilities::readSicd(const std::string& sicdPathname,
             const std::vector<std::string>& schemaPaths,
             std::auto_ptr<ComplexData>& complexData,
             std::vector<std::complex<float> >& widebandData)
{
    six::XMLControlRegistry xmlRegistry;
    xmlRegistry.addCreator(six::DataType::COMPLEX,
            new six::XMLControlCreatorT<
            six::sicd::ComplexXMLControl>());

    six::NITFReadControl reader;
    reader.setXMLControlRegistry(&xmlRegistry);
    reader.load(sicdPathname, schemaPaths);

    complexData = getComplexData(reader);
    getWidebandData(reader, *(complexData.get()), widebandData);

    // This tells the reader that it doesn't
    // own an XMLControlRegistry
    reader.setXMLControlRegistry(NULL);
}

std::auto_ptr<ComplexData> Utilities::getComplexData(NITFReadControl& reader)
{
    const six::Data* data = reader.getContainer()->getData(0);

    if (data->getDataType() != six::DataType::COMPLEX)
    {
        throw except::Exception(Ctxt(data->getName() + " is not a SICD"));
    }

    // Now that we know it's a SICD we can safely cast to a ComplexData
    // We can't just return a pointer to the data though because it's owned by
    // the container which is owned by the reader (and the reader will go out
    // of scope when this function returns), so we need to clone it.
    // Note that you don't have to do this yourself if in your usage the
    // reader stays in scope.
    // TODO: If the container held shared pointers we wouldn't need to do this
    std::auto_ptr<ComplexData> complexData(
            reinterpret_cast<ComplexData*>(data->clone()));
    return complexData;
}

std::auto_ptr<ComplexData> Utilities::getComplexData(
        const std::string& pathname,
        const std::vector<std::string>& schemaPaths)
{
    if (sys::Path::splitExt(pathname).second == ".xml")
    {
        logging::NullLogger log;
        return parseDataFromFile(pathname, schemaPaths, log);
    }

    else
    {
        six::XMLControlRegistry xmlRegistry;
        xmlRegistry.addCreator(six::DataType::COMPLEX,
                new six::XMLControlCreatorT<
                        six::sicd::ComplexXMLControl>());

        six::NITFReadControl reader;
        reader.setXMLControlRegistry(&xmlRegistry);
        reader.load(pathname, schemaPaths);

        return getComplexData(reader);
    }
}

void Utilities::getWidebandData(NITFReadControl& reader,
                                const ComplexData& complexData,
                                const types::RowCol<size_t>& offset,
                                const types::RowCol<size_t>& extent,
                                std::complex<float>* buffer)
{
    const PixelType pixelType = complexData.getPixelType();
    const size_t imageNumber = 0;

    const size_t requiredBufferBytes = sizeof(std::complex<float>)
                                            * extent.area();

    if (buffer == NULL)
    {
        // for some reason we don't have a buffer

        throw except::Exception(Ctxt("Null buffer provided to getWidebandData"
                    + std::string(" when a ")
                    + str::toString(requiredBufferBytes)
                    + std::string(" byte buffer was expected")));
    }

    if (pixelType == PixelType::RE32F_IM32F)
    {
        six::Region region = buildRegion(offset, extent, buffer);
        reader.interleaved(region, imageNumber);
    }
    else if (pixelType == PixelType::RE16I_IM16I)
    {
        readAndConvertSICD(reader,
                           imageNumber,
                           offset,
                           extent,
                           buffer);
    }
    else
    {
        throw except::Exception(Ctxt(
                complexData.getName() + " has an unknown pixel type"));
    }

}
void Utilities::getWidebandData(NITFReadControl& reader,
                                const ComplexData& complexData,
                                std::complex<float>* buffer)
{
    types::RowCol<size_t> offset(0,0);

    types::RowCol<size_t> extent(
            complexData.getNumRows(),
            complexData.getNumCols()
            );

    getWidebandData(reader, complexData, offset, extent, buffer);
}

void Utilities::getWidebandData(NITFReadControl& reader,
                                const ComplexData& complexData,
                                const types::RowCol<size_t>& offset,
                                const types::RowCol<size_t>& extent,
                                std::vector<std::complex<float> >& buffer)
{
    const size_t requiredNumElements = extent.area();
    buffer.resize(requiredNumElements);

    if (requiredNumElements > 0)
    {
        getWidebandData(reader, complexData, offset, extent, &buffer[0]);
    }
}

void Utilities::getWidebandData(NITFReadControl& reader,
                                const ComplexData& complexData,
                                std::vector<std::complex<float> >& buffer)
{
    types::RowCol<size_t> offset;

    types::RowCol<size_t> extent(
            complexData.getNumRows(),
            complexData.getNumCols()
            );

    getWidebandData(reader, complexData, offset, extent, buffer);
}

void Utilities::getWidebandData(
        const std::string& sicdPathname,
        const std::vector<std::string>& /*schemaPaths*/,
        const ComplexData& complexData,
        const types::RowCol<size_t>& offset,
        const types::RowCol<size_t>& extent,
        std::complex<float>* buffer)
{
    six::XMLControlRegistry xmlRegistry;
    xmlRegistry.addCreator(six::DataType::COMPLEX,
                           new six::XMLControlCreatorT<
                                   six::sicd::ComplexXMLControl>());
    six::NITFReadControl reader;
    reader.setXMLControlRegistry(&xmlRegistry);
    reader.load(sicdPathname);

    getWidebandData(reader, complexData, offset, extent, buffer);
}

void Utilities::getWidebandData(
        const std::string& sicdPathname,
        const std::vector<std::string>& schemaPaths,
        const ComplexData& complexData,
        std::complex<float>* buffer)
{
    types::RowCol<size_t> offset(0,0);

    types::RowCol<size_t> extent(
            complexData.getNumRows(),
            complexData.getNumCols()
            );

    getWidebandData(sicdPathname, schemaPaths, complexData,
            offset, extent, buffer);
}

Vector3 Utilities::getGroundPlaneNormal(const ComplexData& data)
{
    Vector3 groundPlaneNormal;

    if (data.radarCollection->area.get() &&
        data.radarCollection->area->plane.get())
    {
        const AreaPlane& areaPlane = *data.radarCollection->area->plane;
        groundPlaneNormal = math::linear::cross(areaPlane.xDirection->unitVector,
                areaPlane.yDirection->unitVector);
    }
    else
    {
        groundPlaneNormal = data.geoData->scp.ecf;
    }

    return groundPlaneNormal.unit();
}

Poly1D Utilities::nPoly(PolyXYZ poly, size_t idx)
{
    if (idx >= 3)
    {
        throw except::Exception(Ctxt("Idx should be < 3"));
    }

    std::vector<double> coefs;
    for (size_t ii = 0; ii < poly.size(); ++ii)
    {
        coefs.push_back(poly[ii][idx]);
    }
    Poly1D ret(coefs);
    return ret;
}

bool Utilities::isClockwise(const std::vector<RowColInt>& vertices,
                            bool isUpPositive)
{
    if (vertices.size() < 3)
    {
        throw except::Exception(Ctxt(
                "Polygons must have at least three points"));
    }

    // If the signed area is positive, and y values are ascending,
    // then it is clockwise
    sys::SSize_T area = 0;
    for (size_t ii = 0; ii < vertices.size(); ++ii)
    {
        const sys::SSize_T x1 = static_cast<sys::SSize_T>(vertices[ii].col);
        const sys::SSize_T y1 = static_cast<sys::SSize_T>(vertices[ii].row);

        const size_t nextIndex = (ii == vertices.size() - 1) ? 0 : ii + 1;

        const sys::SSize_T x2 =
                static_cast<sys::SSize_T>(vertices[nextIndex].col);
        const sys::SSize_T y2 =
                static_cast<sys::SSize_T>(vertices[nextIndex].row);

        area += (x1 * y2 - x2 * y1);
    }
    if (isUpPositive)
    {
        area *= -1;
    }
    return (area > 0);
}

std::auto_ptr<ComplexData> Utilities::parseData(
        ::io::InputStream& xmlStream,
        const std::vector<std::string>& schemaPaths,
        logging::Logger& log)
{
    XMLControlRegistry xmlRegistry;
    xmlRegistry.addCreator(DataType::COMPLEX,
                           new XMLControlCreatorT<ComplexXMLControl>());

    std::auto_ptr<Data> data(six::parseData(
            xmlRegistry, xmlStream, schemaPaths, log));

     std::auto_ptr<ComplexData> complexData(reinterpret_cast<ComplexData*>(
             data.release()));

     return complexData;
}

std::auto_ptr<ComplexData> Utilities::parseDataFromFile(
        const std::string& pathname,
        const std::vector<std::string>& schemaPaths,
        logging::Logger& log)
{
    io::FileInputStream inStream(pathname);
    return parseData(inStream, schemaPaths, log);
}

std::auto_ptr<ComplexData> Utilities::parseDataFromString(
    const std::string& xmlStr,
    const std::vector<std::string>& schemaPaths,
    logging::Logger& log)
{
    io::StringStream inStream;
    inStream.write(xmlStr);
    return parseData(inStream, schemaPaths, log);
}

std::string Utilities::toXMLString(const ComplexData& data,
                                   const std::vector<std::string>& schemaPaths,
                                   logging::Logger* logger)
{
    XMLControlRegistry xmlRegistry;
    xmlRegistry.addCreator(DataType::COMPLEX,
                           new XMLControlCreatorT<ComplexXMLControl>());

    logging::NullLogger nullLogger;
    return ::six::toValidXMLString(&data,
                                   schemaPaths,
                                   (logger == NULL) ? &nullLogger : logger,
                                   &xmlRegistry);
}

std::auto_ptr<ComplexData> Utilities::createFakeComplexData()
{
    std::auto_ptr<ComplexData> data(new six::sicd::ComplexData());
    data->position->arpPoly = six::PolyXYZ(5);
    data->position->arpPoly[0][0] = 4.45303008e6;
    data->position->arpPoly[1][0] = 5.75153322e3;
    data->position->arpPoly[2][0] = -2.7014602e0;
    data->position->arpPoly[3][0] = -1.2323143e-3;
    data->position->arpPoly[4][0] = 2.76797758e-7;
    data->position->arpPoly[5][0] = 8.57355543e-11;

    data->position->arpPoly[0][1] = 1.49876146e6;
    data->position->arpPoly[1][1] = 4.95848815e1;
    data->position->arpPoly[2][1] = -1.3299011e0;
    data->position->arpPoly[3][1] = 1.20353768e-4;
    data->position->arpPoly[4][1] = 1.79750748e-7;
    data->position->arpPoly[5][1] = -1.8053533e-11;

    data->grid->timeCOAPoly = six::Poly2D(2, 2);
    data->grid->timeCOAPoly[0][0] = 1.9789488e0;
    data->grid->timeCOAPoly[0][1] = 2.6269628e-4;
    data->grid->timeCOAPoly[0][2] = 6.1316813e-16;

    data->grid->timeCOAPoly[1][0] = -8.953947e-9;
    data->grid->timeCOAPoly[1][1] = -9.8252154e-13;
    data->grid->timeCOAPoly[1][2] = -9.6216278e-24;

    data->grid->timeCOAPoly[0][0] = 3.3500568e-17;
    data->grid->timeCOAPoly[0][1] = 3.6743435e-21;
    data->grid->timeCOAPoly[0][2] = -3.95088507e-28;

    data->geoData->scp.llh.setLat(4.785840035e1);
    data->geoData->scp.llh.setLon(1.213899391e1);
    data->geoData->scp.llh.setAlt(4.880295281e2);

    data->geoData->scp.ecf[0] = 4.19186033e6;
    data->geoData->scp.ecf[1] = 9.01641404e5;
    data->geoData->scp.ecf[2] = 4.70668874e6;

    data->geoData->imageCorners.getCorner(0).setLat(22.0493529164731);
    data->geoData->imageCorners.getCorner(0).setLon(-159.744920871161);

    data->geoData->imageCorners.getCorner(1).setLat(21.999989220946);
    data->geoData->imageCorners.getCorner(1).setLon(-159.75130136289);

    data->geoData->imageCorners.getCorner(2).setLat(22.0103386157109);
    data->geoData->imageCorners.getCorner(2).setLon(-159.818163639552);

    data->geoData->imageCorners.getCorner(3).setLat(22.0593288745824);
    data->geoData->imageCorners.getCorner(3).setLon(-159.809407055003);

    // Don't have data for this. Just putting something in to prevent
    // a segfault.
    // Later, we can switch on ImageFormation type
    data->pfa.reset(new six::sicd::PFA());
    data->pfa->polarAnglePoly = six::Poly1D(1);
    data->pfa->spatialFrequencyScaleFactorPoly = six::Poly1D(1);

    data->collectionInformation->radarMode = six::RadarModeType::SPOTLIGHT;

    data->imageData->validData = std::vector<six::RowColInt>(8);
    data->imageData->validData[0] = six::RowColInt(0, 0);
    data->imageData->validData[1] = six::RowColInt(0, 6163);
    data->imageData->validData[2] = six::RowColInt(2760, 6163);
    data->imageData->validData[3] = six::RowColInt(7902, 6163);
    data->imageData->validData[4] = six::RowColInt(11623, 6163);
    data->imageData->validData[5] = six::RowColInt(11790, 6163);
    data->imageData->validData[6] = six::RowColInt(11790, 0);
    data->imageData->validData[7] = six::RowColInt(1028, 0);


    // The fields below here aren't really used,
    // just need to be filled with something so things are valid
    data->imageCreation.reset(new ImageCreation());
    data->grid->row->unitVector[0] = 0;
    data->grid->row->unitVector[1] = 0;
    data->grid->row->unitVector[2] = 0;
    data->grid->row->sampleSpacing = 0;
    data->grid->row->impulseResponseWidth = 0;
    data->grid->row->impulseResponseBandwidth = 0;
    data->grid->row->kCenter = 0;
    data->grid->row->deltaK1 = 0;
    data->grid->row->deltaK2 = 0;

    data->grid->col->unitVector[0] = 0;
    data->grid->col->unitVector[1] = 0;
    data->grid->col->unitVector[2] = 0;
    data->grid->col->sampleSpacing = 0;
    data->grid->col->impulseResponseWidth = 0;
    data->grid->col->impulseResponseBandwidth = 0;
    data->grid->col->kCenter = 0;
    data->grid->col->deltaK1 = 0;
    data->grid->col->deltaK2 = 0;

    data->timeline->collectDuration = 0;
    data->radarCollection->txFrequencyMin = 0;
    data->radarCollection->txFrequencyMax = 0;
    data->radarCollection->txPolarization = PolarizationType::V;
    data->radarCollection->rcvChannels.resize(1);
    data->radarCollection->rcvChannels[0].reset(new ChannelParameters());
    data->radarCollection->rcvChannels[0]->txRcvPolarization =
            DualPolarizationType::V_V;

    data->imageFormation->rcvChannelProcessed.reset(new RcvChannelProcessed());
    data->imageFormation->rcvChannelProcessed->numChannelsProcessed = 1;
    data->imageFormation->rcvChannelProcessed->prfScaleFactor = 0;
    data->imageFormation->rcvChannelProcessed->channelIndex.push_back(0);
    data->imageFormation->txRcvPolarizationProc = DualPolarizationType::V_V;
    data->imageFormation->tStartProc = 0;
    data->imageFormation->tEndProc = 0;
    data->imageFormation->txFrequencyProcMin = 0;
    data->imageFormation->txFrequencyProcMax = 0;

    data->scpcoa->scpTime = 0;
    data->fillDerivedFields();

    data->pfa->krg1 = 0;
    data->pfa->krg2 = 0;
    data->pfa->kaz1 = 0;
    data->pfa->kaz2 = 0;
    return data;
}
}
}

