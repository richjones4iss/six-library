/* =========================================================================
 * This file is part of six-c++ 
 * =========================================================================
 * 
 * (C) Copyright 2004 - 2009, General Dynamics - Advanced Information Systems
 *
 * six-c++ is free software; you can redistribute it and/or modify
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
#ifndef __SIX_XML_CONTROL_H__
#define __SIX_XML_CONTROL_H__

#include "six/Data.h"
#include "six/Parameter.h"
#include "six/ErrorStatistics.h"
#include "six/Radiometric.h"
#include <import/xml/lite.h>
#include <import/logging.h>

namespace six
{

/*!
 *  \class XMLControl
 *  \brief Base class for reading and writing a Data object
 *
 *  This interfaces' implementors are not typically called directly.
 *  Instead, they are used to convert back and forth from XML during
 *  file format reads and writes.
 *
 *  The XMLControl is derived for each Data type that is supported
 *  in this library.  This class does not actually convert data into
 *  a bytes, although the helper functions in the XMLControlFactory do.
 *
 *  Instead, this class defines the interface for going between a data
 *  model, represented by the Data object, to an XML Document Object Model
 *  (DOM), and back.
 *
 *  This interface's implementors are used by the ReadControl and the
 *  WriteControl to bundle the XML content into a file container.
 *
 *  They can also be used to interact with an XML model or a stub XML
 *  file as well.
 */
class XMLControl
{

public:
    //!  Constructor
    XMLControl(logging::Logger* log = NULL, bool ownLog = false);

    //!  Destructor
    virtual ~XMLControl();

    void setLogger(logging::Logger* log, bool ownLog = false);

    /*!
     *  Convert the Data model into an XML DOM.
     *  \param data the Data model
     *  \return An XML DOM
     */
    virtual xml::lite::Document* toXML(const Data* data) = 0;

    /*!
     *  Convert a document from a DOM into a Data model
     *  \param doc
     *  \return a Data model
     */
    virtual Data* fromXML(const xml::lite::Document* doc) = 0;

protected:
    typedef xml::lite::Element* XMLElem;

    logging::Logger *mLog;
    bool mOwnLog;

    //! Returns the default URI
    virtual std::string getDefaultURI() const = 0;

    //! Returns the URI to use with SI Common types
    virtual std::string getSICommonURI() const = 0;

    XMLElem newElement(const std::string& name, XMLElem prnt = NULL);

    XMLElem newElement(const std::string& name, const std::string& uri,
            XMLElem prnt = NULL);

    XMLElem newElement(const std::string& name, const std::string& uri,
            const std::string& characterData, XMLElem parent = NULL);

    // generic element creation methods, w/URI
    virtual XMLElem createString(const std::string& name,
            const std::string& uri, const std::string& p = "",
            XMLElem parent = NULL);
    virtual XMLElem createInt(const std::string& name, const std::string& uri,
            int p = 0, XMLElem parent = NULL);
    virtual XMLElem createDouble(const std::string& name,
            const std::string& uri, double p = 0, XMLElem parent = NULL);
    virtual XMLElem createBooleanType(const std::string& name,
            const std::string& uri, BooleanType b, XMLElem parent = NULL);
    virtual XMLElem createDateTime(const std::string& name,
            const std::string& uri, DateTime p, XMLElem parent = NULL);
    virtual XMLElem createDateTime(const std::string& name,
            const std::string& uri, const std::string& s,
            XMLElem parent = NULL);
    virtual XMLElem createDate(const std::string& name,
            const std::string& uri, DateTime p, XMLElem parent = NULL);

    // generic element creation methods, using default URI
    virtual XMLElem createString(const std::string& name,
            const std::string& p = "", XMLElem parent = NULL);
    virtual XMLElem createInt(const std::string& name, int p = 0,
            XMLElem parent = NULL);
    virtual XMLElem createDouble(const std::string& name, double p = 0,
            XMLElem parent = NULL);
    virtual XMLElem createBooleanType(const std::string& name, BooleanType b,
            XMLElem parent = NULL);
    virtual XMLElem createDateTime(const std::string& name, DateTime p,
            XMLElem parent = NULL);
    virtual XMLElem createDateTime(const std::string& name,
            const std::string& s, XMLElem parent = NULL);
    virtual XMLElem createDate(const std::string& name, DateTime p,
            XMLElem parent = NULL);

    XMLElem createComplex(const std::string& name, std::complex<double> c,
            XMLElem parent = NULL);
    XMLElem createVector3D(const std::string& name, Vector3 p = 0.0,
            XMLElem parent = NULL);
    XMLElem createRowCol(const std::string& name, const std::string& rowName,
            const std::string& colName, const RowColInt& value,
            XMLElem parent = NULL);
    XMLElem createRowCol(const std::string& name, const std::string& rowName,
            const std::string& colName, const RowColDouble& value,
            XMLElem parent = NULL);
    XMLElem createRowCol(const std::string& name, const RowColInt& value,
            XMLElem parent = NULL);
    XMLElem createRowCol(const std::string& name, const RowColDouble& value,
            XMLElem parent = NULL);
    XMLElem createRowCol(const std::string&, const RowColLatLon& value,
            XMLElem parent = NULL);
    XMLElem createRangeAzimuth(const std::string& name,
            const RangeAzimuth<double>& value, XMLElem parent = NULL);
    XMLElem createLatLon(const std::string& name, const LatLon& value,
            XMLElem parent = NULL);
    XMLElem createLatLonAlt(const std::string& name, const LatLonAlt& value,
            XMLElem parent = NULL);

    virtual XMLElem createFootprint(const std::string& name,
            const std::string& cornerName, const std::vector<LatLon>& c,
            XMLElem parent = NULL);
    virtual XMLElem createFootprint(const std::string& name,
            const std::string& cornerName, const std::vector<LatLonAlt>& c,
            XMLElem parent = NULL);
    XMLElem createPoly1D(const std::string& name, const std::string& uri,
            const Poly1D& poly1D, XMLElem parent = NULL);
    XMLElem createPoly2D(const std::string& name, const std::string& uri,
            const Poly2D& poly2D, XMLElem parent = NULL);
    XMLElem createPoly1D(const std::string& name, const Poly1D& poly1D,
            XMLElem parent = NULL);
    XMLElem createPoly2D(const std::string& name, const Poly2D& poly2D,
            XMLElem parent = NULL);
    XMLElem createPolyXYZ(const std::string& name, const PolyXYZ& polyXYZ,
            XMLElem parent = NULL);
    XMLElem createParameter(const std::string& name, const std::string& uri,
            const Parameter& value, XMLElem parent = NULL);
    void addParameters(const std::string& name, const std::string& uri,
            const std::vector<Parameter>& props, XMLElem parent = NULL);
    XMLElem createParameter(const std::string& name, const Parameter& value,
            XMLElem parent = NULL);
    void addParameters(const std::string& name,
            const std::vector<Parameter>& props, XMLElem parent = NULL);
    void addDecorrType(const std::string& name, const std::string& uri,
            DecorrType& dt, XMLElem p);

    void parseInt(XMLElem element, int& value);
    void parseInt(XMLElem element, long& value);
    void parseUInt(XMLElem element, unsigned int& value);
    void parseUInt(XMLElem element, unsigned long& value);
    void parseDouble(XMLElem element, double& value);
    void parseComplex(XMLElem element, std::complex<double>& value);
    void parseString(XMLElem element, std::string& value);
    void parseBooleanType(XMLElem element, BooleanType& value);

    void parsePoly1D(XMLElem polyXML, Poly1D& poly1D);
    void parsePoly2D(XMLElem polyXML, Poly2D& poly2D);
    void parsePolyXYZ(XMLElem polyXML, PolyXYZ& polyXYZ);

    void parseVector3D(XMLElem vecXML, Vector3& vec);
    void parseLatLonAlt(XMLElem llaXML, LatLonAlt& lla);
    void parseLatLon(XMLElem parent, LatLon& ll);
    void parseLatLons(XMLElem pointsXML, const std::string& pointName,
            std::vector<LatLon>& llVec);
    void parseRangeAzimuth(XMLElem parent, RangeAzimuth<double>& value);
    virtual void parseFootprint(XMLElem footprint,
            const std::string& cornerName, std::vector<LatLon>& value);
    virtual void parseFootprint(XMLElem footprint,
            const std::string& cornerName, std::vector<LatLonAlt>& value);

    void parseDateTime(XMLElem element, DateTime& value);
    void parseRowColDouble(XMLElem parent, const std::string& rowName,
            const std::string& colName, RowColDouble& rc);
    void parseRowColDouble(XMLElem parent, RowColDouble& rc);

    void parseRowColInt(XMLElem parent, const std::string& rowName,
            const std::string& colName, RowColInt& rc);
    void parseRowColInt(XMLElem parent, RowColInt& rc);
    void parseParameter(XMLElem element, Parameter& param);

    void parseRowColLatLon(XMLElem parent, RowColLatLon& rc);

    void parseParameters(XMLElem paramXML, const std::string& paramName,
            std::vector<Parameter>& props);

    static
    void setAttribute(XMLElem e, const std::string& name,
            const std::string& v);

    static XMLElem getOptional(XMLElem parent, const std::string& tag);
    static XMLElem getFirstAndOnly(XMLElem parent, const std::string& tag);

    void parseDecorrType(XMLElem decorrXML, DecorrType& decorrType);

    XMLElem toXML(const ErrorStatistics* errorStatistics,
            XMLElem parent = NULL);

    void fromXML(const XMLElem errorStatsXML,
            ErrorStatistics* errorStatistics);

    XMLElem toXML(const Radiometric *obj, XMLElem parent = NULL);
    void fromXML(const XMLElem radiometricXML, Radiometric *obj);

    /*!
     * Require an element to be not NULL
     * @throw throws an Exception if the element is NULL
     * @return returns the input Element
     */
    static XMLElem require(XMLElem element, const std::string& name);

    // Takes in a set of points in any order in 'in'
    // Produces the same points in clockwise order, starting with the upper
    // left corner, in 'out'
    static void toClockwise(const std::vector<LatLon>& in,
                            std::vector<LatLon>& out);

    static void toClockwise(const std::vector<LatLonAlt>& in,
                            std::vector<LatLonAlt>& out);
};

}

#endif
