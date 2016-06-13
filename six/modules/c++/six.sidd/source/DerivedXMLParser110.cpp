/* =========================================================================
 * This file is part of six.sidd-c++
 * =========================================================================
 *
 * (C) Copyright 2004 - 2015, MDA Information Systems LLC
 *
 * six.sidd-c++ is free software; you can redistribute it and/or modify
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

#include <sstream>

#include <six/SICommonXMLParser10x.h>
#include <six/sidd/DerivedDataBuilder.h>
#include <six/sidd/DerivedXMLParser110.h>

namespace
{
typedef xml::lite::Element* XMLElem;
typedef xml::lite::Attributes XMLAttributes;

template <typename T>
bool isDefined(const T& enumVal)
{
    return six::Init::isDefined(enumVal.value);
}

template <typename T>
bool isUndefined(const T& enumVal)
{
    return six::Init::isUndefined(enumVal.value);
}

template <typename SmartPtrT>
void confirmNonNull(const SmartPtrT& ptr,
                    const std::string& name,
                    const std::string& suffix = "")
{
    if (ptr.get() == NULL)
    {
        std::string msg = name + " is required";
        if (!suffix.empty())
        {
            msg += " " + suffix;
        }

        throw except::Exception(Ctxt(msg));
    }
}
}

namespace six
{
namespace sidd
{
const char DerivedXMLParser110::VERSION[] = "1.1.0";
const char DerivedXMLParser110::SI_COMMON_URI[] = "urn:SICommon:1.0";
const char DerivedXMLParser110::ISM_URI[] = "urn:us:gov:ic:ism:13";

DerivedXMLParser110::DerivedXMLParser110(logging::Logger* log,
                                         bool ownLog) :
    DerivedXMLParser(VERSION,
                     std::auto_ptr<six::SICommonXMLParser>(
                         new six::SICommonXMLParser10x(versionToURI(VERSION),
                                                       false,
                                                       SI_COMMON_URI,
                                                       log)),
                     log,
                     ownLog)
{
}

DerivedData* DerivedXMLParser110::fromXML(
        const xml::lite::Document* doc) const
{
    XMLElem root = doc->getRootElement();

    XMLElem productCreationElem        = getFirstAndOnly(root, "ProductCreation");
    XMLElem displayElem                = getFirstAndOnly(root, "Display");
    XMLElem geographicAndTargetElem    = getFirstAndOnly(root, "GeographicAndTarget");
    XMLElem measurementElem            = getFirstAndOnly(root, "Measurement");
    XMLElem exploitationFeaturesElem   = getFirstAndOnly(root, "ExploitationFeatures");
    XMLElem productProcessingElem      = getOptional(root, "ProductProcessing");
    XMLElem downstreamReprocessingElem = getOptional(root, "DownstreamReprocessing");
    XMLElem errorStatisticsElem        = getOptional(root, "ErrorStatistics");
    XMLElem radiometricElem            = getOptional(root, "Radiometric");
    XMLElem matchInfoElem              = getOptional(root, "MatchInfo");
    XMLElem compressionElem            = getOptional(root, "Compression");
    XMLElem dedElem                    = getOptional(root, "DigitalElevationData");
    XMLElem annotationsElem            = getOptional(root, "Annotations");


    DerivedDataBuilder builder;
    DerivedData *data = builder.steal(); //steal it

    // see if PixelType has MONO or RGB
    PixelType pixelType = six::toType<PixelType>(
            getFirstAndOnly(displayElem, "PixelType")->getCharacterData());
    builder.addDisplay(pixelType);

    // create GeographicAndTarget
    builder.addGeographicAndTarget();

    // create Measurement
    six::ProjectionType projType = ProjectionType::NOT_SET;
    if (getOptional(measurementElem, "GeographicProjection"))
        projType = ProjectionType::GEOGRAPHIC;
    else if (getOptional(measurementElem, "CylindricalProjection"))
            projType = ProjectionType::CYLINDRICAL;
    else if (getOptional(measurementElem, "PlaneProjection"))
        projType = ProjectionType::PLANE;
    else if (getOptional(measurementElem, "PolynomialProjection"))
        projType = ProjectionType::POLYNOMIAL;
    builder.addMeasurement(projType);

    // create ExploitationFeatures
    std::vector<XMLElem> elements;
    exploitationFeaturesElem->getElementsByTagName("ExploitationFeatures",
                                                  elements);
    builder.addExploitationFeatures(elements.size());

    parseProductCreationFromXML(productCreationElem, data->productCreation.get());
    parseDisplayFromXML(displayElem, *data->display);
    parseGeographicTargetFromXML(geographicAndTargetElem, *data->geographicAndTarget);
    parseMeasurementFromXML(measurementElem, data->measurement.get());
    parseExploitationFeaturesFromXML(exploitationFeaturesElem, data->exploitationFeatures.get());

    if (productProcessingElem)
    {
        builder.addProductProcessing();
        parseProductProcessingFromXML(productProcessingElem,
                                      data->productProcessing.get());
    }
    if (downstreamReprocessingElem)
    {
        builder.addDownstreamReprocessing();
        parseDownstreamReprocessingFromXML(downstreamReprocessingElem,
                                           data->downstreamReprocessing.get());
    }
    if (errorStatisticsElem)
    {
        builder.addErrorStatistics();
        common().parseErrorStatisticsFromXML(errorStatisticsElem,
                                             data->errorStatistics.get());
    }
    if (radiometricElem)
    {
        builder.addRadiometric();
        common().parseRadiometryFromXML(radiometricElem,
                                        data->radiometric.get());
    }
    if (matchInfoElem)
    {
        builder.addMatchInformation();
        common().parseMatchInformationFromXML(matchInfoElem, data->matchInformation.get());
    }
    if (compressionElem)
    {
        builder.addCompression();
        parseCompressionFromXML(compressionElem, *data->compression);
    }
    if (dedElem)
    {
        builder.addDigitalElevationData();
        parseDigitalElevationDataFromXML(dedElem, *data->digitalElevationData);
    }
    if (annotationsElem)
    {
        // 1 to unbounded
        std::vector<XMLElem> annChildren;
        annotationsElem->getElementsByTagName("Annotation", annChildren);
        data->annotations.resize(annChildren.size());
        for (unsigned int i = 0, size = annChildren.size(); i < size; ++i)
        {
            data->annotations[i].reset(new Annotation());
            parseAnnotationFromXML(annChildren[i], data->annotations[i].get());
        }
    }
    return data;
}

xml::lite::Document* DerivedXMLParser110::toXML(const DerivedData* derived) const
{
    xml::lite::Document* doc = new xml::lite::Document();
    XMLElem root = newElement("SIDD");
    doc->setRootElement(root);

    convertProductCreationToXML(derived->productCreation.get(), root);
    convertDisplayToXML(*derived->display, root);
    convertGeographicTargetToXML(*derived->geographicAndTarget, root);
    convertMeasurementToXML(derived->measurement.get(), root);
    convertExploitationFeaturesToXML(derived->exploitationFeatures.get(),
                                     root);

    // optional
    if (derived->downstreamReprocessing.get())
    {
        convertDownstreamReprocessingToXML(
                derived->downstreamReprocessing.get(), root);
    }
    // optional
    if (derived->errorStatistics.get())
    {
        common().convertErrorStatisticsToXML(derived->errorStatistics.get(),
                                             root);
    }
    // optional
    if (derived->matchInformation.get())
    {
        common().convertMatchInformationToXML(*derived->matchInformation,
                                              root);
    }
    // optional
    if (derived->radiometric.get())
    {
        common().convertRadiometryToXML(derived->radiometric.get(), root);
    }
    // optional
    if (derived->compression.get())
    {
       convertCompressionToXML(*derived->compression, root);
    }
    // optional
    if (derived->digitalElevationData.get())
    {
        convertDigitalElevationDataToXML(*derived->digitalElevationData,
                                         root);
    }
    // optional
    if (derived->productProcessing.get())
    {
        convertProductProcessingToXML(derived->productProcessing.get(), root);
    }
    // optional
    if (!derived->annotations.empty())
    {
        XMLElem annotationsElem = newElement("Annotations", root);
        for (size_t i = 0, num = derived->annotations.size(); i < num; ++i)
        {
            convertAnnotationToXML(derived->annotations[i].get(),
                                   annotationsElem);
        }
    }

    //set the ElemNS
    root->setNamespacePrefix("", getDefaultURI());
    root->setNamespacePrefix("si", SI_COMMON_URI);
    root->setNamespacePrefix("sfa", SFA_URI);
    root->setNamespacePrefix("ism", ISM_URI);

    return doc;
}

void DerivedXMLParser110::parseDerivedClassificationFromXML(
        const XMLElem classificationElem,
        DerivedClassification& classification) const
{
    DerivedXMLParser::parseDerivedClassificationFromXML(classificationElem, classification);
    const XMLAttributes& classificationAttributes
        = classificationElem->getAttributes();

    getAttributeList(classificationAttributes,
        "ism:compliesWith",
        classification.compliesWith);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:exemptFrom",
        classification.exemptFrom);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:joint",
        classification.joint);
    // optional
    getAttributeListIfExists(classificationAttributes,
        "ism:atomicEnergyMarkings",
        classification.atomicEnergyMarkings);
    // optional
    getAttributeListIfExists(classificationAttributes,
        "ism:displayOnlyTo",
        classification.displayOnlyTo);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:noticeType",
        classification.noticeType);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:noticeReason",
        classification.noticeReason);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:noticeDate",
        classification.noticeDate);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:unregisteredNoticeType",
        classification.unregisteredNoticeType);
    // optional
    getAttributeIfExists(classificationAttributes,
        "ism:externalNotice",
        classification.externalNotice);
}

void DerivedXMLParser110::parseCompressionFromXML(const XMLElem compressionElem,
                                                 Compression& compression) const
{
    XMLElem j2kElem = getFirstAndOnly(compressionElem, "J2K");
    XMLElem originalElem = getFirstAndOnly(j2kElem, "Original");
    XMLElem parsedElem   = getOptional(j2kElem, "Parsed");

    parseJ2KCompression(originalElem, compression.original);
    if (parsedElem)
    {
        compression.parsed.reset(new J2KCompression());
        parseJ2KCompression(parsedElem, *compression.parsed);
    }
}

void DerivedXMLParser110::parseJ2KCompression(const XMLElem j2kElem,
                                              J2KCompression& j2k) const
{
    parseInt(getFirstAndOnly(j2kElem, "NumWaveletLevels"),
            j2k.numWaveletLevels);
    parseInt(getFirstAndOnly(j2kElem, "NumBands"),
            j2k.numBands);

    XMLElem layerInfoElems = getFirstAndOnly(j2kElem, "LayerInfo");
    std::vector<XMLElem> layerElems;
    layerInfoElems->getElementsByTagName("Layer", layerElems);

    size_t numLayers = layerElems.size();
    j2k.layerInfo.resize(numLayers);

    for (size_t ii = 0; ii < layerElems.size(); ++ii)
    {
        parseDouble(getFirstAndOnly(layerElems[ii], "Bitrate"),
                    j2k.layerInfo[ii].bitRate);
    }
}

void DerivedXMLParser110::parseDisplayFromXML(const XMLElem displayElem,
                                              Display& display) const
{
    //pixelType previously set
    XMLElem bandInfoElem = getOptional(displayElem, "BandInformation");
    if (bandInfoElem)
    {
        display.bandInformation.reset(new BandInformation());
        parseBandInformationFromXML(bandInfoElem, *display.bandInformation);
    }

    std::vector<XMLElem> nonInteractiveProcessingElems;
    displayElem->getElementsByTagName("NonInteractiveProcessing",
            nonInteractiveProcessingElems);

    display.nonInteractiveProcessing.resize(nonInteractiveProcessingElems.size());
    for (size_t ii = 0; ii < nonInteractiveProcessingElems.size(); ++ii)
    {
        display.nonInteractiveProcessing[ii].reset(new NonInteractiveProcessing());
        parseNonInteractiveProcessingFromXML(nonInteractiveProcessingElems[ii],
                *display.nonInteractiveProcessing[ii]);
    }

    std::vector<XMLElem> interactiveProcessingElems;
    displayElem->getElementsByTagName("InteractiveProcessing",
            interactiveProcessingElems);

    display.interactiveProcessing.resize(interactiveProcessingElems.size());
    for (size_t ii = 0; ii < interactiveProcessingElems.size(); ++ii)
    {
        display.interactiveProcessing[ii].reset(new InteractiveProcessing());
        parseInteractiveProcessingFromXML(interactiveProcessingElems[ii],
                *display.interactiveProcessing[ii]);
    }

    std::vector<XMLElem> extensions;
    displayElem->getElementsByTagName("DisplayExtention", extensions);
    for (size_t ii = 0; ii < extensions.size(); ++ii)
    {
        std::string name;
        getAttributeIfExists(extensions[ii]->getAttributes(), "name", name);
        std::string value;
        parseString(extensions[ii], value);
        Parameter parameter(value);
        parameter.setName(name);
        display.displayExtensions.push_back(parameter);
    }
}

void DerivedXMLParser110::parseBandInformationFromXML(const XMLElem bandElem,
            BandInformation& bandInformation) const
{
    std::vector<XMLElem> bandDescriptors;
    bandElem->getElementsByTagName("BandDescriptor", bandDescriptors);
    bandInformation.bandDescriptors = std::vector<std::string>();
    bandInformation.bandDescriptors.resize(bandDescriptors.size());
    for (size_t ii = 0; ii < bandDescriptors.size(); ++ii)
    {
        parseString(bandDescriptors[ii], bandInformation.bandDescriptors[ii]);
    }

    XMLElem displayFlagElem = getOptional(bandElem, "DisplayFlag");
    if (displayFlagElem)
    {
        parseInt(displayFlagElem, bandInformation.displayFlag);
    }
    else
    {
        bandInformation.displayFlag = six::Init::undefined<size_t>();
    }
}

void DerivedXMLParser110::parseNonInteractiveProcessingFromXML(
            const XMLElem procElem,
            NonInteractiveProcessing& nonInteractiveProcessing) const
{
    XMLElem productGenerationOptions = getFirstAndOnly(procElem,
            "ProductGenerationOptions");
    XMLElem rrdsElem = getFirstAndOnly(procElem, "RRDS");

    parseProductGenerationOptionsFromXML(productGenerationOptions,
        nonInteractiveProcessing.productGenerationOptions);
    parseRRDSFromXML(rrdsElem, nonInteractiveProcessing.rrds);
}

void DerivedXMLParser110::parseProductGenerationOptionsFromXML(
            const XMLElem optionsElem,
            ProductGenerationOptions& options) const
{
    XMLElem bandElem = getOptional(optionsElem, "BandEqualization");
    XMLElem restoration = getOptional(optionsElem,
            "ModularTransferFunctionRestoration");
    XMLElem remapElem = getOptional(optionsElem, "DataRemapping");
    XMLElem correctionElem = getOptional(optionsElem,
            "AsymmetricPixelCorrection");

    if (bandElem)
    {
        options.bandEqualization.reset(new BandEqualization());
        parseBandEqualizationFromXML(bandElem, *options.bandEqualization);
    }

    if (restoration)
    {
        options.modularTransferFunctionRestoration.reset(new Filter());
        parseFilterFromXML(restoration,
                           *options.modularTransferFunctionRestoration);
    }

    if (remapElem)
    {
        options.dataRemapping.reset(new LookupTable());
        parseLookupTableFromXML(remapElem, *options.dataRemapping);
    }

    if (correctionElem)
    {
        options.asymmetricPixelCorrection.reset(new Filter());
        parseFilterFromXML(correctionElem, *options.asymmetricPixelCorrection);
    }
}

void DerivedXMLParser110::parseLookupTableFromXML(
            const XMLElem lookupElem,
            LookupTable& lookupTable) const
{
    parseString(getFirstAndOnly(lookupElem, "LUTName"), lookupTable.lutName);
    XMLElem customElem = getOptional(lookupElem, "Custom");
    XMLElem predefinedElem = getOptional(lookupElem, "Predefined");

    bool ok = false;
    if (customElem)
    {
        if (!predefinedElem)
        {
            ok = true;
            XMLElem lutInfoElem = getFirstAndOnly(customElem, "LUTInfo");
            const XMLAttributes& attributes = lutInfoElem->getAttributes();
            size_t numBands;
            size_t size;
            getAttributeIfExists(attributes, "numBands", numBands);
            getAttributeIfExists(attributes, "size", size);
            lookupTable.custom.reset(new LookupTable::Custom(numBands, numBands));
            std::vector<XMLElem> lutElems;
            lutInfoElem->getElementsByTagName("LUTValues", lutElems);

            for (size_t ii = 0; ii < lutElems.size(); ++ii)
            {
                std::auto_ptr<LUT> lut = parseSingleLUT(lutElems[ii], size);
                lookupTable.custom->lutValues[ii] = *lut;
            }
        }
    }
    else if (predefinedElem)
    {
        ok = true;
        lookupTable.predefined.reset(new LookupTable::Predefined());
        bool innerOk = false;
        XMLElem dbNameElem = getOptional(predefinedElem, "DatabaseName");
        XMLElem familyElem = getOptional(predefinedElem, "RemapFamily");
        XMLElem memberElem = getOptional(predefinedElem, "RemapMember");

        if (dbNameElem)
        {
            if (!familyElem && !memberElem)
            {
                innerOk = true;
                parseString(dbNameElem, lookupTable.predefined->databaseName);
            }
        }
        else if (familyElem && memberElem)
        {
            innerOk = true;
            parseInt(familyElem, lookupTable.predefined->remapFamily);
            parseInt(memberElem, lookupTable.predefined->remapMember);
        }
        if (innerOk == false)
        {
            throw except::Exception(Ctxt("Exactly one of databaseName or remapFamiy + remapMember must be set"));
        }
    }
    if (ok == false)
    {
        throw except::Exception(Ctxt("Exactly one of Custom or Predefined must be set for LookupTable"));
    }
}

void DerivedXMLParser110::parseBandEqualizationFromXML(const XMLElem bandElem,
                                                       BandEqualization& band) const
{
    parseEnum(getFirstAndOnly(bandElem, "Algorithm"), band.algorithm);

    std::vector<XMLElem> lutElems;
    bandElem->getElementsByTagName("BandLUT", lutElems);
    band.bandLUTs.resize(lutElems.size());
    for (size_t ii = 0; ii < lutElems.size(); ++ii)
    {
        band.bandLUTs[ii].reset(new LookupTable());
        parseLookupTableFromXML(lutElems[ii], *band.bandLUTs[ii]);
    }
}

void DerivedXMLParser110::parseRRDSFromXML(const XMLElem rrdsElem,
            RRDS& rrds) const
{
    parseEnum(getFirstAndOnly(rrdsElem, "DownsamplingMethod"), rrds.downsamplingMethod);

    bool hasMoreFields = true;
    if (rrds.downsamplingMethod.toString() == "DECIMATE" || rrds.downsamplingMethod.toString() == "MAX_PIXEL")
    {
        hasMoreFields = false;
    }
    XMLElem antiAliasElem = getOptional(rrdsElem, "AntiAlias");
    XMLElem interpolationElem = getOptional(rrdsElem, "Interpolation");

    if (hasMoreFields && (antiAliasElem == NULL || interpolationElem == NULL))
    {
        throw except::Exception(Ctxt("Both AntiAlias and Interpolation required unless DownsamplingMethod = DECIMATE or MAX_PIXEL"));
    }
    if (hasMoreFields == false && (antiAliasElem || interpolationElem))
    {
        throw except::Exception(Ctxt("If DownsamplingMethod = DECIMATE or MAX_PIXEL, neither AntiAlias nor Interpolation allowed"));
    }

    if (antiAliasElem)
    {
        rrds.antiAlias.reset(new Filter());
        parseFilterFromXML(antiAliasElem, *rrds.antiAlias);
    }
    if (interpolationElem)
    {
        rrds.interpolation.reset(new Filter());
        parseFilterFromXML(interpolationElem, *rrds.interpolation);
    }
}

void DerivedXMLParser110::parseFilterFromXML(const XMLElem filterElem,
    Filter& filter) const
{
    parseString(getFirstAndOnly(filterElem, "FilterName"), filter.filterName);
    XMLElem kernelElem = getOptional(filterElem, "FilterKernel");
    XMLElem bankElem = getOptional(filterElem, "FilterBank");

    bool ok = false;
    if (kernelElem)
    {
        if (!bankElem)
        {
            ok = true;
            filter.filterKernel.reset(new Filter::Kernel());
            parseKernelFromXML(kernelElem, *filter.filterKernel);
        }
    }
    else if (bankElem)
    {
        ok = true;
        filter.filterBank.reset(new Filter::Bank());
        parseBankFromXML(bankElem, *filter.filterBank);
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Exactly one of FilterKernel or FilterBank must be set"));
    }
    parseEnum(getFirstAndOnly(filterElem, "Operation"), filter.operation);
}

void DerivedXMLParser110::parsePredefinedFilterFromXML(const XMLElem predefinedElem,
     Filter::Predefined& predefined) const
{
    bool ok = false;
    XMLElem dbNameElem = getOptional(predefinedElem, "DatabaseName");
    XMLElem familyElem = getOptional(predefinedElem, "FilterFamily");
    XMLElem filterMember = getOptional(predefinedElem, "FilterMember");

    if (dbNameElem)
    {
        if (!familyElem && !filterMember)
        {
            ok = true;
            parseEnum(dbNameElem, predefined.databaseName);
        }
    }
    else if (familyElem && filterMember)
    {
        ok = true;

        parseInt(familyElem, predefined.filterFamily);
        parseInt(familyElem, predefined.filterMember);
    }
    if (!ok)
    {
        throw except::Exception(Ctxt(
            "Exactly one of either dbName or FilterFamily and "
            "FilterMember must be defined"));
    }
}

void DerivedXMLParser110::parseKernelFromXML(const XMLElem kernelElem,
     Filter::Kernel& kernel) const
{
    XMLElem predefinedElem = getOptional(kernelElem, "Predefined");
    XMLElem customElem = getOptional(kernelElem, "Custom");

    bool ok = false;
    if (predefinedElem)
    {
        if (!customElem)
        {
            ok = true;
            kernel.predefined.reset(new Filter::Predefined());
            parsePredefinedFilterFromXML(predefinedElem, *kernel.predefined);
        }
    }
    else if (customElem)
    {
        ok = true;
        kernel.custom.reset(new Filter::Kernel::Custom());
        XMLElem filterCoef = getFirstAndOnly(customElem, "FilterCoefficients");
        const XMLAttributes& attributes = filterCoef->getAttributes();
        getAttributeIfExists(attributes, "numRows", kernel.custom->size.row);
        getAttributeIfExists(attributes, "numCols", kernel.custom->size.col);

        if (six::Init::isUndefined(kernel.custom->size.row) || six::Init::isUndefined(kernel.custom->size.col))
        {
            throw except::Exception("Expected row and col attributes in FilterCoefficients element of Custom");
        }

        std::vector<XMLElem> coefficients;
        filterCoef->getElementsByTagName("Coef", coefficients);
        size_t numCoefs = coefficients.size();
        kernel.custom->filterCoef.resize(numCoefs);
        for (size_t ii = 0; ii < numCoefs; ++ii)
        {
            parseDouble(coefficients[ii], kernel.custom->filterCoef[ii]);
        }
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Exactly one of Custom or Predefined must be set for FilterKernel"));
    }
}
void DerivedXMLParser110::parseBankFromXML(const XMLElem bankElem,
     Filter::Bank& bank) const
{
    XMLElem predefinedElem = getOptional(bankElem, "Predefined");
    XMLElem customElem = getOptional(bankElem, "Custom");

    bool ok = false;
    if (predefinedElem)
    {
        if (!customElem)
        {
            ok = true;
            bank.predefined.reset(new Filter::Predefined());
            parsePredefinedFilterFromXML(predefinedElem, *bank.predefined);
        }
    }
    else if (customElem)
    {
        bank.custom.reset(new Filter::Bank::Custom());
        ok = true;

        XMLElem filterCoef = getFirstAndOnly(customElem, "FilterCoefficients");
        const XMLAttributes& attributes = filterCoef->getAttributes();
        getAttributeIfExists(attributes, "numPhasings", bank.custom->numPhasings);
        getAttributeIfExists(attributes, "numPoints", bank.custom->numPoints);

        std::vector<XMLElem> coefficients;
        filterCoef->getElementsByTagName("Coef", coefficients);
        size_t numCoefs = coefficients.size();
        bank.custom->filterCoef.resize(numCoefs);
        for (size_t ii = 0; ii < numCoefs; ++ii)
        {
            parseDouble(coefficients[ii], bank.custom->filterCoef[ii]);
        }
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Exactly one of Custom or Predefined must be set for FilterBank"));
    }
}

void DerivedXMLParser110::parseInteractiveProcessingFromXML(
            const XMLElem interactiveElem,
            InteractiveProcessing& interactive) const
{
    XMLElem geomElem = getFirstAndOnly(interactiveElem, "GeometricTransform");
    XMLElem sharpnessElem = getFirstAndOnly(interactiveElem,
            "SharpnessEnhancement");
    XMLElem colorElem = getOptional(interactiveElem, "ColorSpaceTransform");
    XMLElem dynamicElem = getFirstAndOnly(interactiveElem, "DynamicRangeAdjustment");
    XMLElem ttcElem = getOptional(interactiveElem, "TonalTransferCurve");

    interactive.geometricTransform = GeometricTransform();
    parseGeometricTransformFromXML(geomElem, interactive.geometricTransform);
    parseSharpnessEnhancementFromXML(sharpnessElem,
                                     interactive.sharpnessEnhancement);

    if (colorElem)
    {
        interactive.colorSpaceTransform.reset(new ColorSpaceTransform());
        parseColorSpaceTransformFromXML(colorElem,
                                        *interactive.colorSpaceTransform);
    }

    parseDynamicRangeAdjustmentFromXML(dynamicElem,
                                       interactive.dynamicRangeAdjustment);

    if (ttcElem)
    {
        interactive.tonalTransferCurve.reset(new LookupTable());
        parseLookupTableFromXML(ttcElem, *interactive.tonalTransferCurve);
    }
}

void DerivedXMLParser110::parseGeometricTransformFromXML(const XMLElem geomElem,
             GeometricTransform& transform) const
{
    XMLElem scalingElem = getFirstAndOnly(geomElem, "Scaling");
    parseFilterFromXML(getFirstAndOnly(scalingElem, "AntiAlias"),
        transform.scaling.antiAlias);
    parseFilterFromXML(getFirstAndOnly(scalingElem, "Interpolation"),
        transform.scaling.interpolation);

    XMLElem orientationElem = getFirstAndOnly(geomElem, "Orientation");
    parseEnum(getFirstAndOnly(orientationElem, "ShadowDirection"), transform.orientation.shadowDirection);
}

void DerivedXMLParser110::parseSharpnessEnhancementFromXML(
             const XMLElem sharpElem,
             SharpnessEnhancement& sharpness) const
{
    bool ok = false;
    XMLElem mTFCElem = getOptional(sharpElem,
                                   "ModularTransferFunctionCompensation");
    XMLElem mTFRElem = getOptional(sharpElem,
                                   "ModularTransferFunctionRestoration");
    if (mTFCElem)
    {
        if (!mTFRElem)
        {
            ok = true;
            sharpness.modularTransferFunctionCompensation.reset(new Filter());
            parseFilterFromXML(mTFCElem,
                               *sharpness.modularTransferFunctionCompensation);
        }
    }
    else if (mTFRElem)
    {
        ok = true;
        sharpness.modularTransferFunctionRestoration.reset(new Filter());
        parseFilterFromXML(mTFRElem,
                           *sharpness.modularTransferFunctionRestoration);
    }
    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of modularTransferFunctionCompensation or "
                "modularTransferFunctionRestoration must be set"));
    }
}

void DerivedXMLParser110::parseColorSpaceTransformFromXML(
            const XMLElem colorElem, ColorSpaceTransform& transform) const
{
    XMLElem manageElem = getFirstAndOnly(colorElem, "ColorManagementModule");

    parseEnum(getFirstAndOnly(manageElem, "RenderingIntent"),
            transform.colorManagementModule.renderingIntent);
    parseString(getFirstAndOnly(manageElem, "SourceProfile"),
                transform.colorManagementModule.sourceProfile);
    parseString(getFirstAndOnly(manageElem, "DisplayProfile"),
                transform.colorManagementModule.displayProfile);
    parseString(getFirstAndOnly(manageElem, "ICCProfile"),
                transform.colorManagementModule.iccProfile);
}

void DerivedXMLParser110::parseDynamicRangeAdjustmentFromXML(
            const XMLElem rangeElem,
            DynamicRangeAdjustment& rangeAdjustment) const
{
    parseEnum(getFirstAndOnly(rangeElem, "AlgorithmType"), rangeAdjustment.algorithmType);
    parseInt(getFirstAndOnly(rangeElem, "BandStatsSource"), rangeAdjustment.bandStatsSource);

    bool ok = false;
    XMLElem parameterElem = getOptional(rangeElem, "DRAParameters");
    XMLElem overrideElem = getOptional(rangeElem, "DRAOverrides");
    if (parameterElem)
    {
        if (!overrideElem)
        {
            ok = true;
            rangeAdjustment.draParameters.reset(new DynamicRangeAdjustment::DRAParameters());
            parseDouble(getFirstAndOnly(parameterElem, "Pmin"), rangeAdjustment.draParameters->pMin);
            parseDouble(getFirstAndOnly(parameterElem, "Pmax"), rangeAdjustment.draParameters->pMax);
            parseDouble(getFirstAndOnly(parameterElem, "EminModifier"), rangeAdjustment.draParameters->eMinModifier);
            parseDouble(getFirstAndOnly(parameterElem, "EmaxModifier"), rangeAdjustment.draParameters->eMaxModifier);
        }
    }
    else if (overrideElem)
    {
        ok = true;
        rangeAdjustment.draOverrides.reset(new DynamicRangeAdjustment::DRAOverrides());
        parseDouble(getFirstAndOnly(overrideElem, "Subtractor"),
            rangeAdjustment.draOverrides->subtractor);
        parseDouble(getFirstAndOnly(overrideElem, "Multiplier"),
            rangeAdjustment.draOverrides->multiplier);
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Elem should have exactly one of DRAParameters and DRAOverrides"));
    }
}

XMLElem DerivedXMLParser110::convertDerivedClassificationToXML(
        const DerivedClassification& classification,
        XMLElem parent) const
{
    XMLElem classElem = newElement("Classification", parent);

    common().addParameters("SecurityExtension",
                    classification.securityExtensions,
                           classElem);

    //! from ism:ISMRootNodeAttributeGroup
    // SIDD 1.1 is tied to IC-ISM v13
    setAttribute(classElem, "DESVersion", "13", ISM_URI);

    // So far as I can tell this should just be 1
    setAttribute(classElem, "ISMCATCESVersion", "1", ISM_URI);

    //! from ism:ResourceNodeAttributeGroup
    setAttribute(classElem, "resourceElement", "true", ISM_URI);
    setAttribute(classElem, "createDate",
                 classification.createDate.format("%Y-%m-%d"), ISM_URI);
    // required (was optional in SIDD 1.0)
    setAttributeList(classElem, "compliesWith", classification.compliesWith,
                     ISM_URI);

    // optional
    setAttributeIfNonEmpty(classElem,
                           "exemptFrom",
                           classification.exemptFrom,
                           ISM_URI);

    //! from ism:SecurityAttributesGroup
    //  -- referenced in ism::ResourceNodeAttributeGroup
    setAttribute(classElem, "classification", classification.classification,
                 ISM_URI);
    setAttributeList(classElem, "ownerProducer", classification.ownerProducer,
                     ISM_URI, true);

    // optional
    setAttributeIfNonEmpty(classElem, "joint", classification.joint, ISM_URI);

    // optional
    setAttributeList(classElem, "SCIcontrols", classification.sciControls,
                     ISM_URI);
    // optional
    setAttributeList(classElem, "SARIdentifier", classification.sarIdentifier,
                     ISM_URI);
    // optional
    setAttributeList(classElem,
                     "atomicEnergyMarkings",
                     classification.atomicEnergyMarkings,
                     ISM_URI);
    // optional
    setAttributeList(classElem,
                     "disseminationControls",
                     classification.disseminationControls,
                     ISM_URI);
    // optional
    setAttributeList(classElem,
                     "displayOnlyTo",
                     classification.displayOnlyTo,
                     ISM_URI);
    // optional
    setAttributeList(classElem, "FGIsourceOpen", classification.fgiSourceOpen,
                     ISM_URI);
    // optional
    setAttributeList(classElem,
                     "FGIsourceProtected",
                     classification.fgiSourceProtected,
                     ISM_URI);
    // optional
    setAttributeList(classElem, "releasableTo", classification.releasableTo,
                     ISM_URI);
    // optional
    setAttributeList(classElem, "nonICmarkings", classification.nonICMarkings,
                     ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "classifiedBy",
                           classification.classifiedBy,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "compilationReason",
                           classification.compilationReason,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "derivativelyClassifiedBy",
                           classification.derivativelyClassifiedBy,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "classificationReason",
                           classification.classificationReason,
                           ISM_URI);
    // optional
    setAttributeList(classElem, "nonUSControls", classification.nonUSControls,
                     ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "derivedFrom",
                           classification.derivedFrom,
                           ISM_URI);
    // optional
    if (classification.declassDate.get())
    {
        setAttributeIfNonEmpty(
                classElem, "declassDate",
                classification.declassDate->format("%Y-%m-%d"),
                ISM_URI);
    }
    // optional
    setAttributeIfNonEmpty(classElem,
                           "declassEvent",
                           classification.declassEvent,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "declassException",
                           classification.declassException,
                           ISM_URI);

    //! from ism:NoticeAttributesGroup
    // optional
    setAttributeIfNonEmpty(classElem,
                           "noticeType",
                           classification.noticeType,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "noticeReason",
                           classification.noticeReason,
                           ISM_URI);
    // optional
    if (classification.noticeDate.get())
    {
        setAttributeIfNonEmpty(
                classElem, "noticeDate",
                classification.noticeDate->format("%Y-%m-%d"),
                ISM_URI);
    }
    // optional
    setAttributeIfNonEmpty(classElem,
                           "unregisteredNoticeType",
                           classification.unregisteredNoticeType,
                           ISM_URI);
    // optional
    setAttributeIfNonEmpty(classElem,
                           "externalNotice",
                           classification.externalNotice,
                           ISM_URI);

    return classElem;
}

XMLElem DerivedXMLParser110::convertLookupTableToXML(
        const std::string& name,
        const LookupTable& table,
        XMLElem parent) const
{
    XMLElem lookupElem = newElement(name, parent);
    createString("LUTName", table.lutName, lookupElem);

    bool ok = false;
    if (table.predefined.get())
    {
        if (table.custom.get() == NULL)
        {
            ok = true;
            XMLElem predefElem = newElement("Predefined", lookupElem);

            //exactly one of databaseName or (remapFamily and remapMember) can be set
            bool innerOk = false;
            if (table.predefined->databaseName.empty())
            {
                if (six::Init::isDefined(table.predefined->remapFamily) &&
                    six::Init::isDefined(table.predefined->remapMember))
                {
                    innerOk = true;
                    createInt("RemapFamily", table.predefined->remapFamily, predefElem);
                    createInt("RemapMember", table.predefined->remapMember, predefElem);
                }
            }
            else if (six::Init::isUndefined(table.predefined->remapFamily) &&
                     six::Init::isUndefined(table.predefined->remapMember))
            {
                innerOk = true;
                createString("DatabaseName", table.predefined->databaseName, predefElem);
            }
            if (innerOk == false)
            {
                throw except::Exception(Ctxt("Exactly one of databaseName or remapFamiy+remapMember must be set"));
            }
        }
    }
    else if (table.custom.get())
    {
        ok = true;
        std::vector<LUT>& lutValues = table.custom->lutValues;

        XMLElem customElem = newElement("Custom", lookupElem);
        XMLElem lutInfoElem = newElement("LUTInfo", customElem);
        setAttribute(lutInfoElem, "numBands", str::toString(lutValues.size()));
        setAttribute(lutInfoElem, "size",
                str::toString(lutValues[0].table.size()));

        for (size_t ii = 0; ii < lutValues.size(); ++ii)
        {
            XMLElem lutElem = createLUT("LUTValues", &lutValues[ii], lutInfoElem);
            setAttribute(lutElem, "Band", str::toString(ii + 1));
        }
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Exactly one of Predefined or Custom must be defined"));
    }
    return lookupElem;
}

XMLElem DerivedXMLParser110::convertNonInteractiveProcessingToXML(
        const NonInteractiveProcessing& processing,
        XMLElem parent) const
{
    XMLElem processingElem = newElement("NonInteractiveProcessing", parent);

    // ProductGenerationOptions
    XMLElem prodGenElem = newElement("ProductGenerationOptions",
                                    processingElem);

    const ProductGenerationOptions& prodGen =
            processing.productGenerationOptions;

    if (prodGen.bandEqualization.get())
    {
        const BandEqualization& bandEq = *prodGen.bandEqualization;
        XMLElem bandEqElem = newElement("BandEqualization", prodGenElem);
        createStringFromEnum("Algorithm", bandEq.algorithm, bandEqElem);
        for (size_t ii = 0; ii < bandEq.bandLUTs.size(); ++ii)
        {
            convertLookupTableToXML("BandLUT", *bandEq.bandLUTs[ii], bandEqElem);
        }

        //add the attribute to each of the LUTs
        std::vector<XMLElem> LUTElems;
        bandEqElem->getElementsByTagName("BandLUT", LUTElems);
        for (size_t ii = 0; ii < LUTElems.size(); ++ii)
        {
            setAttribute(LUTElems[ii], "k", str::toString(ii+1));
        }
    }

    if (prodGen.modularTransferFunctionRestoration.get())
    {
        convertFilterToXML("ModularTransferFunctionRestoration",
                           *prodGen.modularTransferFunctionRestoration,
                           prodGenElem);
    }

    if (prodGen.dataRemapping.get())
    {
        convertLookupTableToXML("DataRemapping", *prodGen.dataRemapping,
                                prodGenElem);
    }

    if (prodGen.asymmetricPixelCorrection.get())
    {
        convertFilterToXML("AsymmetricPixelCorrection",
                           *prodGen.asymmetricPixelCorrection, prodGenElem);
    }

    // RRDS
    XMLElem rrdsElem = newElement("RRDS", processingElem);

    const RRDS& rrds = processing.rrds;
    createStringFromEnum("DownsamplingMethod", rrds.downsamplingMethod,
                         rrdsElem);

    if (rrds.downsamplingMethod != DownsamplingMethod::DECIMATE && rrds.downsamplingMethod != DownsamplingMethod::MAX_PIXEL)
    {
        confirmNonNull(rrds.antiAlias, "antiAlias",
                       "for DECIMATE downsampling");
        convertFilterToXML("AntiAlias", *rrds.antiAlias, rrdsElem);

        confirmNonNull(rrds.interpolation, "interpolation",
                       "for DECIMATE downsampling");
        convertFilterToXML("Interpolation", *rrds.interpolation, rrdsElem);
    }
    return processingElem;
}

XMLElem DerivedXMLParser110::convertInteractiveProcessingToXML(
        const InteractiveProcessing& processing,
        XMLElem parent) const
{
    XMLElem processingElem = newElement("InteractiveProcessing", parent);

    // GeometricTransform
    const GeometricTransform& geoTransform(processing.geometricTransform);
    XMLElem geoTransformElem = newElement("GeometricTransform", processingElem);

    XMLElem scalingElem = newElement("Scaling", geoTransformElem);
    convertFilterToXML("AntiAlias", geoTransform.scaling.antiAlias,
                       scalingElem);
    convertFilterToXML("Interpolation", geoTransform.scaling.interpolation,
                       scalingElem);

    XMLElem orientationElem = newElement("Orientation", geoTransformElem);
    createStringFromEnum("ShadowDirection",
        geoTransform.orientation.shadowDirection,
        orientationElem);

    // SharpnessEnhancement
    const SharpnessEnhancement& sharpness(processing.sharpnessEnhancement);
    XMLElem sharpElem = newElement("SharpnessEnhancement", processingElem);

    bool ok = false;
    if (sharpness.modularTransferFunctionCompensation.get())
    {
        if (sharpness.modularTransferFunctionRestoration.get() == NULL)
        {
            ok = true;
            convertFilterToXML("ModularTransferFunctionCompensation",
                               *sharpness.modularTransferFunctionCompensation,
                               sharpElem);
        }
    }
    else if (sharpness.modularTransferFunctionRestoration.get())
    {
        ok = true;
        convertFilterToXML("ModularTransferFunctionRestoration",
                           *sharpness.modularTransferFunctionRestoration,
                           sharpElem);
    }

    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of modularTransferFunctionCompensation or "
                "modularTransferFunctionRestoration must be set"));
    }

    // ColorSpaceTransform
    if (processing.colorSpaceTransform.get())
    {
        const ColorManagementModule& cmm =
                processing.colorSpaceTransform->colorManagementModule;

        XMLElem colorSpaceTransformElem =
                newElement("ColorSpaceTransform", processingElem);
        XMLElem cmmElem =
                newElement("ColorManagementModule", colorSpaceTransformElem);

        createStringFromEnum("RenderingIntent", cmm.renderingIntent, cmmElem);

        // TODO: Not sure what this'll actually look like
        createString("SourceProfile", cmm.sourceProfile, cmmElem);
        createString("DisplayProfile", cmm.displayProfile, cmmElem);

        if (!cmm.iccProfile.empty())
        {
            createString("ICCProfile", cmm.iccProfile, cmmElem);
        }
    }

    // DynamicRangeAdjustment

    const DynamicRangeAdjustment& adjust =
            processing.dynamicRangeAdjustment;

    XMLElem adjustElem =
        newElement("DynamicRangeAdjustment", processingElem);

    createStringFromEnum("AlgorithmType", adjust.algorithmType,
        adjustElem);
    createInt("BandStatsSource", adjust.bandStatsSource, adjustElem);

    ok = false;
    if (adjust.draParameters.get())
    {
        if (!adjust.draOverrides.get())
        {
            ok = true;
            XMLElem paramElem = newElement("DRAParameters", adjustElem);
            createDouble("Pmin", adjust.draParameters->pMin, paramElem);
            createDouble("Pmax", adjust.draParameters->pMax, paramElem);
            createDouble("EminModifier", adjust.draParameters->eMinModifier, paramElem);
            createDouble("EmaxModifier", adjust.draParameters->eMinModifier, paramElem);
        }
    }
    else if (adjust.draOverrides.get())
    {
        ok = true;
        XMLElem overrideElem = newElement("DRAOverrides", adjustElem);
        createDouble("Subtractor", adjust.draOverrides->subtractor, overrideElem);
        createDouble("Multiplier", adjust.draOverrides->multiplier, overrideElem);
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Data must contain exactly one of DRAParameters and DRAOverrides"));
    }

    if (processing.tonalTransferCurve.get())
    {
        convertLookupTableToXML("TonalTransferCurve", *processing.tonalTransferCurve, processingElem);
    }
    return processingElem;
}

XMLElem DerivedXMLParser110::convertPredefinedFilterToXML(
        const Filter::Predefined& predefined,
        XMLElem parent) const
{
    XMLElem predefinedElem = newElement("Predefined", parent);

    // Make sure either DBName or FilterFamily+FilterMember are defined
    bool ok = false;
    if (isDefined(predefined.databaseName))
    {
        if (six::Init::isUndefined(predefined.filterFamily) &&
            six::Init::isUndefined(predefined.filterMember))
        {
            ok = true;

            createStringFromEnum("DatabaseName", predefined.databaseName, predefinedElem);
        }
    }
    else if (six::Init::isDefined(predefined.filterFamily) &&
             six::Init::isDefined(predefined.filterMember))
    {
        ok = true;

        createInt("FilterFamily", predefined.filterFamily, predefinedElem);
        createInt("FilterMember", predefined.filterMember, predefinedElem);
    }

    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of either dbName or FilterFamily and "
                "FilterMember must be defined"));
    }

    return predefinedElem;
}

XMLElem DerivedXMLParser110::convertKernelToXML(
        const Filter::Kernel& kernel,
        XMLElem parent) const
{
    XMLElem kernelElem = newElement("FilterKernel", parent);

    bool ok = false;
    if (kernel.predefined.get())
    {
        if (kernel.custom.get() == NULL)
        {
            ok = true;
            convertPredefinedFilterToXML(*kernel.predefined, kernelElem);
        }
    }
    else if (kernel.custom.get())
    {
        ok = true;

        XMLElem customElem = newElement("Custom", kernelElem);

        if (kernel.custom->filterCoef.size() !=
            static_cast<size_t>(kernel.custom->size.row) * kernel.custom->size.col)
        {
            std::ostringstream ostr;
            ostr << "Filter size is " << kernel.custom->size.row << " rows x "
                << kernel.custom->size.col << " cols but have "
                << kernel.custom->filterCoef.size() << " coefficients";
            throw except::Exception(Ctxt(ostr.str()));
        }

        XMLElem filterCoef = newElement("FilterCoefficients", customElem);
        setAttribute(filterCoef, "numRows", str::toString(kernel.custom->size.row));
        setAttribute(filterCoef, "numCols", str::toString(kernel.custom->size.col));

        for (sys::SSize_T row = 0, idx = 0; row < kernel.custom->size.row; ++row)
        {
            for (sys::SSize_T col = 0; col < kernel.custom->size.col; ++col, ++idx)
            {
                XMLElem coefElem = createDouble("Coef", kernel.custom->filterCoef[idx],
                    filterCoef);
                setAttribute(coefElem, "row", str::toString(row));
                setAttribute(coefElem, "col", str::toString(col));
            }
        }
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Exactly one of Custom or Predefined must be set"));
    }

    return kernelElem;
}

XMLElem DerivedXMLParser110::convertBankToXML(const Filter::Bank& bank,
    XMLElem parent) const
{
    XMLElem bankElem = newElement("FilterBank", parent);

    bool ok = false;
    if (bank.predefined.get())
    {
        if (bank.custom.get() == NULL)
        {
            ok = true;
            convertPredefinedFilterToXML(*bank.predefined, bankElem);
        }
    }
    else if (bank.custom.get())
    {
        ok = true;

        XMLElem customElem = newElement("Custom", bankElem);

        if (bank.custom->filterCoef.size() !=
            static_cast<size_t>(bank.custom->numPhasings) * bank.custom->numPoints)
        {
            std::ostringstream ostr;
            ostr << "Filter size is " << bank.custom->numPhasings << " x "
                << bank.custom->numPoints << " but have "
                << bank.custom->filterCoef.size() << " coefficients";
            throw except::Exception(Ctxt(ostr.str()));
        }

        XMLElem filterCoef = newElement("FilterCoefficients", customElem);
        setAttribute(filterCoef, "numPhasings", str::toString(bank.custom->numPhasings));
        setAttribute(filterCoef, "numPoints", str::toString(bank.custom->numPoints));

        for (size_t row = 0, idx = 0; row < bank.custom->numPhasings; ++row)
        {
            for (size_t col = 0; col < bank.custom->numPoints; ++col, ++idx)
            {
                XMLElem coefElem = createDouble("Coef", bank.custom->filterCoef[idx],
                    filterCoef);
                setAttribute(coefElem, "phasing", str::toString(row));
                setAttribute(coefElem, "point", str::toString(col));
            }
        }
    }
    if (!ok)
    {
        throw except::Exception(Ctxt("Exactly one of Custom or Predefined must be set"));
    }

    return bankElem;
}

XMLElem DerivedXMLParser110::convertFilterToXML(const std::string& name,
                                                const Filter& filter,
                                                XMLElem parent) const
{
    XMLElem filterElem = newElement(name, parent);

    createString("FilterName", filter.filterName, filterElem);

    // Exactly one of Kernel or Bank should be set
    bool ok = false;
    if (filter.filterKernel.get())
    {
        if (filter.filterBank.get() == NULL)
        {
            ok = true;
            convertKernelToXML(*filter.filterKernel, filterElem);
        }
    }
    else if (filter.filterBank.get())
    {
        ok = true;
        convertBankToXML(*filter.filterBank, filterElem);
    }

    if (!ok)
    {
        throw except::Exception(Ctxt(
                "Exactly one of kernel or bank must be set"));
    }

    createStringFromEnum("Operation", filter.operation, filterElem);

    return filterElem;
}

XMLElem DerivedXMLParser110::convertCompressionToXML(
        const Compression& compression,
        XMLElem parent) const
{
    XMLElem compressionElem = newElement("Compression", parent);
    XMLElem j2kElem = newElement("J2K", compressionElem);
    XMLElem originalElem = newElement("Original", j2kElem);
    convertJ2KToXML(compression.original, originalElem);

    if (compression.parsed.get())
    {
        XMLElem parsedElem = newElement("Parsed", j2kElem);
        convertJ2KToXML(*compression.parsed, parsedElem);
    }
    return compressionElem;
}

void DerivedXMLParser110::convertJ2KToXML(const J2KCompression& j2k,
                                          XMLElem& parent) const
{
    createInt("NumWaveletLevels", j2k.numWaveletLevels, parent);
    createInt("NumBands", j2k.numBands, parent);

    size_t numLayers = j2k.layerInfo.size();
    XMLElem layerInfoElem = newElement("LayerInfo", parent);
    setAttribute(layerInfoElem, "numLayers", toString(numLayers));

    for (size_t ii = 0; ii < numLayers; ++ii)
    {
        XMLElem layerElem = newElement("Layer", layerInfoElem);
        setAttribute(layerElem, "index", toString(ii + 1));
        createDouble("Bitrate", j2k.layerInfo[ii].bitRate, layerElem);
    }
}

XMLElem DerivedXMLParser110::convertMeasurementToXML(const Measurement* measurement,
    XMLElem parent) const
{
    XMLElem measurementElem = DerivedXMLParser::convertMeasurementToXML(measurement, parent);

    if (six::Init::isDefined(measurement->arpFlag))
    {
        createStringFromEnum("ARPFlag", measurement->arpFlag, measurementElem);
    }

    common().createPolyXYZ("ARPPoly",
        measurement->arpPoly,
        measurementElem);

    //only if 3+ vertices
    const size_t numVertices = measurement->validData.size();
    if (numVertices >= 3)
    {
        XMLElem vElem = newElement("ValidData", measurementElem);
        setAttribute(vElem, "size", str::toString(numVertices));

        for (size_t ii = 0; ii < numVertices; ++ii)
        {
            XMLElem vertexElem = common().createRowCol(
                "Vertex", measurement->validData[ii], vElem);
            setAttribute(vertexElem, "index", str::toString(ii + 1));
        }
    }
    return measurementElem;
}

XMLElem DerivedXMLParser110::convertExploitationFeaturesToXML(
    const ExploitationFeatures* exploitationFeatures,
    XMLElem parent) const
{
    XMLElem exploitationFeaturesElem =
        newElement("ExploitationFeatures", parent);

    if (exploitationFeatures->collections.size() < 1)
    {
        throw except::Exception(Ctxt(FmtX(
            "ExploitationFeatures must have at least [1] Collection, " \
            "only [%d] found", exploitationFeatures->collections.size())));
    }

    // 1 to unbounded
    for (size_t i = 0; i < exploitationFeatures->collections.size(); ++i)
    {
        Collection* collection = exploitationFeatures->collections[i].get();
        XMLElem collectionElem = newElement("Collection",
            exploitationFeaturesElem);
        setAttribute(collectionElem, "identifier", collection->identifier);

        // create Information
        XMLElem informationElem = newElement("Information", collectionElem);

        createString("SensorName",
            collection->information.sensorName,
            informationElem);
        XMLElem radarModeElem = newElement("RadarMode", informationElem);
        createString("ModeType",
            common().getSICommonURI(),
            six::toString(collection->information.radarMode),
            radarModeElem);
        // optional
        if (collection->information.radarModeID
            != Init::undefined<std::string>())
            createString("ModeID",
                common().getSICommonURI(),
                collection->information.radarModeID,
                radarModeElem);
        createDateTime("CollectionDateTime",
            collection->information.collectionDateTime,
            informationElem);
        // optional
        if (collection->information.localDateTime != Init::undefined<
            six::DateTime>())
        {
            createDateTime("LocalDateTime",
                collection->information.localDateTime,
                informationElem);
        }
        createDouble("CollectionDuration",
            collection->information.collectionDuration,
            informationElem);
        // optional
        if (!Init::isUndefined(collection->information.resolution))
        {
            common().createRangeAzimuth("Resolution",
                collection->information.resolution,
                informationElem);
        }
        // optional
        if (collection->information.inputROI.get())
        {
            XMLElem roiElem = newElement("InputROI", informationElem);
            common().createRowCol("Size",
                collection->information.inputROI->size,
                roiElem);
            common().createRowCol("UpperLeft",
                collection->information.inputROI->upperLeft,
                roiElem);
        }
        // optional to unbounded
        for (size_t n = 0, nElems =
            collection->information.polarization.size(); n < nElems; ++n)
        {
            TxRcvPolarization *p = collection->information.polarization[n].get();
            XMLElem polElem = newElement("Polarization", informationElem);

            createString("TxPolarization",
                six::toString(p->txPolarization),
                polElem);
            createString("RcvPolarization",
                six::toString(p->rcvPolarization),
                polElem);
            // optional
            if (!Init::isUndefined(p->rcvPolarizationOffset))
            {
                createDouble("RcvPolarizationOffset",
                    p->rcvPolarizationOffset,
                    polElem);
            }
            // optional
            if (!Init::isUndefined(p->processed))
            {
                createString("Processed", six::toString(p->processed), polElem);
            }
        }

        // create Geometry -- optional
        Geometry* geom = collection->geometry.get();
        if (geom != NULL)
        {
            XMLElem geometryElem = newElement("Geometry", collectionElem);

            // optional
            if (geom->azimuth != Init::undefined<double>())
                createDouble("Azimuth", geom->azimuth, geometryElem);
            // optional
            if (geom->slope != Init::undefined<double>())
                createDouble("Slope", geom->slope, geometryElem);
            // optional
            if (geom->squint != Init::undefined<double>())
                createDouble("Squint", geom->squint, geometryElem);
            // optional
            if (geom->graze != Init::undefined<double>())
                createDouble("Graze", geom->graze, geometryElem);
            // optional
            if (geom->tilt != Init::undefined<double>())
                createDouble("Tilt", geom->tilt, geometryElem);
            // optional
            if (geom->dopplerConeAngle != Init::undefined<double>())
                createDouble("DopplerConeAngle", geom->dopplerConeAngle, geometryElem);
            // optional to unbounded
            common().addParameters("Extension", geom->extensions,
                geometryElem);
        }

        // create Phenomenology -- optional
        Phenomenology* phenom = collection->phenomenology.get();
        if (phenom != NULL)
        {
            XMLElem phenomenologyElem = newElement("Phenomenology",
                collectionElem);

            // optional
            if (phenom->shadow != Init::undefined<AngleMagnitude>())
            {
                XMLElem shadow = newElement("Shadow", phenomenologyElem);
                createDouble("Angle", common().getSICommonURI(),
                    phenom->shadow.angle, shadow);
                createDouble("Magnitude", common().getSICommonURI(),
                    phenom->shadow.magnitude, shadow);
            }
            // optional
            if (phenom->layover != Init::undefined<AngleMagnitude>())
            {
                XMLElem layover = newElement("Layover", phenomenologyElem);
                createDouble("Angle", common().getSICommonURI(),
                    phenom->layover.angle, layover);
                createDouble("Magnitude", common().getSICommonURI(),
                    phenom->layover.magnitude, layover);
            }
            // optional
            if (phenom->multiPath != Init::undefined<double>())
                createDouble("MultiPath", phenom->multiPath, phenomenologyElem);
            // optional
            if (phenom->groundTrack != Init::undefined<double>())
                createDouble("GroundTrack", phenom->groundTrack,
                    phenomenologyElem);
            // optional to unbounded
            common().addParameters("Extension", phenom->extensions,
                phenomenologyElem);
        }
    }

    // create Product
    XMLElem productElem = newElement("Product", exploitationFeaturesElem);

    common().createRowCol("Resolution",
        exploitationFeatures->product.resolution,
        productElem);
    // optional
    if (exploitationFeatures->product.north != Init::undefined<double>())
        createDouble("North", exploitationFeatures->product.north, productElem);
    // optional to unbounded

    common().addParameters("Extension",
        exploitationFeatures->product.extensions,
        productElem);

    return exploitationFeaturesElem;
}

XMLElem DerivedXMLParser110::convertDisplayToXML(
        const Display& display,
        XMLElem parent) const
{
    // NOTE: In several spots here, there are fields which are required in
    //       SIDD 1.1 but a pointer in the Display class since it didn't exist
    //       in SIDD 1.0, so need to confirm it's allocated
    XMLElem displayElem = newElement("Display", parent);


    createString("PixelType", six::toString(display.pixelType), displayElem);

    // BandInformation (Optional)
    if (display.bandInformation.get() != NULL)
    {
        XMLElem bandInfoElem = newElement("BandInformation", displayElem);
        createInt("NumBands", display.bandInformation->bandDescriptors.size(),
                bandInfoElem);
        for (size_t ii = 0; 
                ii < display.bandInformation->bandDescriptors.size(); ++ii)
        {
            XMLElem bandElem = createString("BandDescriptor",
                display.bandInformation->bandDescriptors[ii],
                bandInfoElem);
            setAttribute(bandElem, "band", str::toString(ii + 1));
        }

        if (six::Init::isDefined<size_t>(display.bandInformation->displayFlag))
        {
            createInt("DisplayFlag", display.bandInformation->displayFlag,
                bandInfoElem);
        }
    }

    // NonInteractiveProcessing
    
    for (size_t ii = 0; ii < display.nonInteractiveProcessing.size(); ++ii)
    {
        confirmNonNull(display.nonInteractiveProcessing[ii],
                "nonInteractiveProcessing");
        XMLElem temp = convertNonInteractiveProcessingToXML(
                *display.nonInteractiveProcessing[ii],
                displayElem);
        setAttribute(temp, "band", str::toString(ii + 1));
    }

    for (size_t ii = 0; ii < display.interactiveProcessing.size(); ++ii)
    {
        // InteractiveProcessing
        confirmNonNull(display.interactiveProcessing[ii],
                "interactiveProcessing");
        XMLElem temp = convertInteractiveProcessingToXML(
                *display.interactiveProcessing[ii],
                displayElem);
        setAttribute(temp, "band", str::toString(ii + 1));
    }

    // optional to unbounded
    common().addParameters("DisplayExtension", display.displayExtensions,
                           displayElem);
    return displayElem;
}

XMLElem DerivedXMLParser110::convertGeographicTargetToXML(
        const GeographicAndTarget& geographicAndTarget,
        XMLElem parent) const
{
    XMLElem geographicAndTargetElem = newElement("GeographicAndTarget", parent);

    common().createEarthModelType("EarthModel", geographicAndTarget.earthModel, geographicAndTargetElem);

    confirmNonNull(geographicAndTarget.imageCorners,
                   "geographicAndTarget.imageCorners");
    common().createLatLonFootprint("ImageCorners", "ICP",
                                   *geographicAndTarget.imageCorners,
                                   geographicAndTargetElem);

    //only if 3+ vertices
    const size_t numVertices = geographicAndTarget.validData.size();
    if (numVertices >= 3)
    {
        XMLElem vElem = newElement("ValidData", geographicAndTargetElem);
        setAttribute(vElem, "size", str::toString(numVertices));

        for (size_t ii = 0; ii < numVertices; ++ii)
        {
            XMLElem vertexElem =
                    common().createLatLon("Vertex",
                                          geographicAndTarget.validData[ii],
                                          vElem);
            setAttribute(vertexElem, "index", str::toString(ii + 1));
        }
    }

    for (size_t ii = 0; ii < geographicAndTarget.geoInfos.size(); ++ii)
    {
        common().convertGeoInfoToXML(*geographicAndTarget.geoInfos[ii],
                                     true,
                                     geographicAndTargetElem);
    }

    return geographicAndTargetElem;
}

XMLElem DerivedXMLParser110::convertDigitalElevationDataToXML(
        const DigitalElevationData& ded,
        XMLElem parent) const
{
    XMLElem dedElem = newElement("DigitalElevationData", parent);

    // GeographicCoordinates
    XMLElem geoCoordElem = newElement("GeographicCoordinates", dedElem);
    createDouble("LongitudeDensity",
                 ded.geographicCoordinates.longitudeDensity,
                 geoCoordElem);
    createDouble("LatitudeDensity",
                 ded.geographicCoordinates.latitudeDensity,
                 geoCoordElem);
    common().createLatLon("ReferenceOrigin",
                          ded.geographicCoordinates.referenceOrigin,
                          geoCoordElem);

    // Geopositioning
    XMLElem geoposElem = newElement("Geopositioning", dedElem);
    createStringFromEnum("CoordinateSystemType",
                         ded.geopositioning.coordinateSystemType,
                         geoposElem);
    createString("GeodeticDatum", ded.geopositioning.geodeticDatum,
                 geoposElem);
    createString("ReferenceEllipsoid", ded.geopositioning.referenceEllipsoid,
                 geoposElem);
    createString("VerticalDatum", ded.geopositioning.verticalDatum,
                 geoposElem);
    createString("SoundingDatum", ded.geopositioning.soundingDatum,
                 geoposElem);
    createInt("FalseOrigin", ded.geopositioning.falseOrigin, geoposElem);
    if (ded.geopositioning.coordinateSystemType == CoordinateSystemType::UTM)
    {
        createInt("UTMGridZoneNumber",
                  ded.geopositioning.utmGridZoneNumber,
                  geoposElem);
    }

    // PositionalAccuracy
    XMLElem posAccElem = newElement("PositionalAccuracy", dedElem);
    createInt("NumRegions", ded.positionalAccuracy.numRegions, posAccElem);

    XMLElem absAccElem = newElement("AbsoluteAccuracy", posAccElem);
    createDouble("Horizontal",
                 ded.positionalAccuracy.absoluteAccuracyHorizontal,
                 absAccElem);
    createDouble("Vertical",
                 ded.positionalAccuracy.absoluteAccuracyVertical,
                 absAccElem);

    XMLElem p2pAccElem = newElement("PointToPointAccuracy", posAccElem);
    createDouble("Horizontal",
                 ded.positionalAccuracy.pointToPointAccuracyHorizontal,
                 p2pAccElem);
    createDouble("Vertical",
                 ded.positionalAccuracy.pointToPointAccuracyVertical,
                 p2pAccElem);

    if (six::Init::isDefined(ded.nullValue))
    {
        createInt("NullValue", ded.nullValue, dedElem);
    }

    return dedElem;
}

void DerivedXMLParser110::parseGeographicTargetFromXML(
    const XMLElem geographicElem,
    GeographicAndTarget& geographicAndTarget) const
{
    std::string model;

    common().parseEarthModelType(getFirstAndOnly(geographicElem, "EarthModel"), geographicAndTarget.earthModel);
    common().parseFootprint(getFirstAndOnly(geographicElem, "ImageCorners"), "ICP",
        *geographicAndTarget.imageCorners);

    XMLElem dataElem = getOptional(geographicElem, "ValidData");
    if (dataElem)
    {
        common().parseLatLons(dataElem, "Vertex", geographicAndTarget.validData);
    }

    std::vector<XMLElem> geoInfosElem;
    geographicElem->getElementsByTagName("GeoInfo", geoInfosElem);

    //optional
    size_t idx(geographicAndTarget.geoInfos.size());
    geographicAndTarget.geoInfos.resize(idx + geoInfosElem.size());

    for (std::vector<XMLElem>::const_iterator it = geoInfosElem.begin(); it
        != geoInfosElem.end(); ++it, ++idx)
    {
        geographicAndTarget.geoInfos[idx].reset(new GeoInfo());
        common().parseGeoInfoFromXML(*it, geographicAndTarget.geoInfos[idx].get());
    }
}

void DerivedXMLParser110::parseMeasurementFromXML(
        const XMLElem measurementElem,
        Measurement* measurement) const
{
    DerivedXMLParser::parseMeasurementFromXML(measurementElem, measurement);

    XMLElem arpFlagElem = getOptional(measurementElem, "ARPFlag");
    if (arpFlagElem)
    {
        parseEnum(arpFlagElem, measurement->arpFlag);
    }

    common().parsePolyXYZ(getFirstAndOnly(measurementElem, "ARPPoly"),
        measurement->arpPoly);

    XMLElem validDataElem = getOptional(measurementElem, "ValidData");
    if (validDataElem)
    {
        common().parseRowColInts(validDataElem,
            "Vertex",
            measurement->validData);
    }
}

void DerivedXMLParser110::parseExploitationFeaturesFromXML(
    const XMLElem exploitationFeaturesElem,
    ExploitationFeatures* exploitationFeatures) const
{
    DerivedXMLParser::parseExploitationFeaturesFromXML(exploitationFeaturesElem, exploitationFeatures);

    std::vector<XMLElem> collectionsElem;
    exploitationFeaturesElem->getElementsByTagName("Collection", collectionsElem);
    for (size_t i = 0; i < collectionsElem.size(); ++i)
    {
        XMLElem collectionElem = collectionsElem[i];
        XMLElem geometryElem = getOptional(collectionElem, "Geometry");
        Collection coll = *exploitationFeatures->collections[i];

        // optional
        if (geometryElem) {
            XMLElem dopplerElem = getOptional(geometryElem, "DopplerConeAngle");
            if (dopplerElem)
                parseDouble(dopplerElem, coll.geometry->dopplerConeAngle);
        }
    }
}

void DerivedXMLParser110::parseDigitalElevationDataFromXML(
        const XMLElem elem,
        DigitalElevationData& ded) const
{
    XMLElem coordElem = getFirstAndOnly(elem, "GeographicCoordinates");
    parseDouble(getFirstAndOnly(coordElem, "LongitudeDensity"), ded.geographicCoordinates.longitudeDensity);
    parseDouble(getFirstAndOnly(coordElem, "LatitudeDensity"), ded.geographicCoordinates.latitudeDensity);
    common().parseLatLon(getFirstAndOnly(coordElem, "ReferenceOrigin"), ded.geographicCoordinates.referenceOrigin);

    XMLElem posElem = getFirstAndOnly(elem, "Geopositioning");
    std::string coordSystemType;
    parseString(getFirstAndOnly(posElem, "CoordinateSystemType"), coordSystemType);
    ded.geopositioning.coordinateSystemType = CoordinateSystemType(coordSystemType);
    parseUInt(getFirstAndOnly(posElem, "FalseOrigin"), ded.geopositioning.falseOrigin);
    parseInt(getFirstAndOnly(posElem, "UTMGridZoneNumber"), ded.geopositioning.utmGridZoneNumber);

    XMLElem posAccuracyElem = getFirstAndOnly(elem, "PositionalAccuracy");
    parseUInt(getFirstAndOnly(posAccuracyElem, "NumRegions"), ded.positionalAccuracy.numRegions);
    XMLElem absoluteElem = getFirstAndOnly(posAccuracyElem, "AbsoluteAccuracy");
    parseDouble(getFirstAndOnly(absoluteElem, "Horizontal"), ded.positionalAccuracy.absoluteAccuracyHorizontal);
    parseDouble(getFirstAndOnly(absoluteElem, "Vertical"), ded.positionalAccuracy.absoluteAccuracyVertical);
    XMLElem pointElem = getFirstAndOnly(posAccuracyElem, "PointToPointAccuracy");
    parseDouble(getFirstAndOnly(pointElem, "Horizontal"), ded.positionalAccuracy.pointToPointAccuracyHorizontal);
    parseDouble(getFirstAndOnly(pointElem, "Vertical"), ded.positionalAccuracy.pointToPointAccuracyVertical);
}

std::auto_ptr<LUT> DerivedXMLParser110::parseSingleLUT(const XMLElem elem,
        size_t size) const
{
    std::string lutStr = "";
    parseString(elem, lutStr);
    std::vector<std::string> lutVals = str::split(lutStr, " ");
    std::auto_ptr<LUT> lut(new LUT(size, sizeof(short)));

    for (size_t ii = 0; ii < lutVals.size(); ++ii)
    {
        const short lutVal = str::toType<short>(lutVals[ii]);
        ::memcpy(&(lut->table[ii * lut->elementSize]),
            &lutVal, sizeof(short));
    }
    return lut;
}

XMLElem DerivedXMLParser110::createLUT(const std::string& name, const LUT *lut,
        XMLElem parent) const
{
    XMLElem lutElement = newElement(name, parent);
    return createLUTImpl(lut, lutElement);
}
}
}
