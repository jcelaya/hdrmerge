/* This file is part of the dngconvert project
   Copyright (C) 2011 Jens Mueller <tschensensinger at gmx dot de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "exiv2meta.h"
#include "exiv2dngstreamio.h"

#include <dng_rational.h>
#include <dng_orientation.h>

#include <exiv2/exif.hpp>
#include <exiv2/image.hpp>

#ifdef WIN32
#define snprintf _snprintf
#include <windows.h>
#endif

void printExiv2ExceptionError(const char* msg, Exiv2::Error& e)
{
    std::string s(e.what());
    fprintf(stderr, "Exiv2: %s  (Error #%i: %s)\n", msg, e.code(), s.c_str());
}

bool getExifTag(const Exiv2::ExifData& exifData, const char* exifTagName, dng_string* value)
{
    try
    {
        Exiv2::ExifKey exifKey(exifTagName);
        Exiv2::ExifData::const_iterator it = exifData.findKey(exifKey);
        if (it != exifData.end())
        {
            value->Set_ASCII((it->print(&exifData)).c_str());
            value->TrimLeadingBlanks();
            value->TrimTrailingBlanks();
            return true;
        }
    }
    catch(Exiv2::Error& e)
    {
        std::string err = std::string("Cannot find Exif String value from key '") + std::string(exifTagName);
        printExiv2ExceptionError(err.c_str(), e);
    }

    return false;
}

bool getExifTag(const Exiv2::ExifData& exifData, const char* exifTagName, dng_date_time_info* value)
{
    try
    {
        Exiv2::ExifKey exifKey(exifTagName);
        Exiv2::ExifData::const_iterator it = exifData.findKey(exifKey);
        if (it != exifData.end())
        {
            dng_date_time dt;
            dt.Parse((it->print(&exifData)).c_str());
            value->SetDateTime(dt);
            return true;
        }
    }
    catch(Exiv2::Error& e)
    {
        std::string err = std::string("Cannot find Exif String value from key '") + std::string(exifTagName);
        printExiv2ExceptionError(err.c_str(), e);
    }

    return false;
}

bool getExifTag(const Exiv2::ExifData& exifData, const char* exifTagName, int32 component, dng_srational* rational)
{
    try
    {
        Exiv2::ExifData::const_iterator it = exifData.findKey(Exiv2::ExifKey(exifTagName));
        if (it != exifData.end())
        {
            Exiv2::Rational exiv2Rational = (*it).toRational(component);
            *rational = dng_srational(exiv2Rational.first, exiv2Rational.second);
            return true;
        }
    }
    catch(Exiv2::Error& e)
    {
        std::string err = std::string("Cannot find Exif Rational value from key '") + std::string(exifTagName);
        printExiv2ExceptionError(err.c_str(), e);
    }

    return false;
}

bool getExifTag(const Exiv2::ExifData& exifData, const char* exifTagName, int32 component, dng_urational* rational)
{
    try
    {
        Exiv2::ExifData::const_iterator it = exifData.findKey(Exiv2::ExifKey(exifTagName));
        if (it != exifData.end())
        {
            Exiv2::URational exiv2Rational = (*it).toRational(component);
            *rational = dng_urational(exiv2Rational.first, exiv2Rational.second);
            return true;
        }
    }
    catch(Exiv2::Error& e)
    {
        std::string err = std::string("Cannot find Exif Rational value from key '") + std::string(exifTagName);
        printExiv2ExceptionError(err.c_str(), e);
    }

    return false;
}

bool getExifTag(const Exiv2::ExifData& exifData, const char* exifTagName, int32 component, uint32* value)
{
    try
    {
        Exiv2::ExifData::const_iterator it = exifData.findKey(Exiv2::ExifKey(exifTagName));
        if (it != exifData.end() && it->count() > 0)
        {
            *value = static_cast<uint32>(it->toLong(component));
            return true;
        }
    }
    catch(Exiv2::Error& e)
    {
        std::string err = std::string("Cannot find Exif Long value from key '") + std::string(exifTagName);
        printExiv2ExceptionError(err.c_str(), e);
    }

    return false;
}

bool getExifTagData(const Exiv2::ExifData& exifData, const char* exifTagName, long* size, unsigned char** data)
{
    try
    {
        Exiv2::ExifData::const_iterator it = exifData.findKey(Exiv2::ExifKey(exifTagName));
        if (it != exifData.end())
        {
            *data = new unsigned char[(*it).size()];
            *size = (*it).size();
            (*it).copy((Exiv2::byte*)*data, Exiv2::bigEndian);
            return true;
        }
    }
    catch(Exiv2::Error& e)
    {
        std::string err = std::string("Cannot find Exif value from key '") + std::string(exifTagName);
        printExiv2ExceptionError(err.c_str(), e);
    }

    return false;
}

Exiv2Meta::Exiv2Meta() :
    m_Exif(),
    m_XMP(),
    m_MakerNote(),
    m_MakerNoteOffset(0)
{

}

Exiv2Meta::~Exiv2Meta()
{

}

void Exiv2Meta::Parse(dng_host &host, dng_stream &stream)
{
    m_Exif.Reset(new dng_exif());

    try
    {
        Exiv2::BasicIo::AutoPtr exiv2Stream(new Exiv2DngStreamIO(stream));
        Exiv2::Image::AutoPtr image = Exiv2::ImageFactory::open(exiv2Stream);

        if (!image.get())
        {
            fprintf(stderr, "Exiv2Meta: Stream is not readable\n");
            return;
        }

        image->readMetadata();

        // Image comments ---------------------------------
        std::string imageComments = image->comment();

        // Exif metadata ----------------------------------
        Exiv2::ExifData exifData = image->exifData();

        // Iptc metadata ----------------------------------
        Exiv2::IptcData iptcData = image->iptcData();

        // Xmp metadata -----------------------------------
        Exiv2::XmpData xmpData = image->xmpData();

        dng_string str;
        dng_urational urat = dng_urational(0, 0);
        uint32 val = 0;

        // Standard Exif tags

        getExifTag(exifData, "Exif.Image.Artist", &m_Exif->fArtist);
        getExifTag(exifData, "Exif.Image.BatteryLevel", 0, &m_Exif->fBatteryLevelR);
        getExifTag(exifData, "Exif.Image.CameraSerialNumber", &m_Exif->fCameraSerialNumber);
        getExifTag(exifData, "Exif.Image.Copyright", &m_Exif->fCopyright);
        getExifTag(exifData, "Exif.Image.DateTime", &m_Exif->fDateTime);
        getExifTag(exifData, "Exif.Image.Make", &m_Exif->fMake);
        getExifTag(exifData, "Exif.Image.Model", &m_Exif->fModel);
        getExifTag(exifData, "Exif.Image.ImageDescription", &m_Exif->fImageDescription);
        getExifTag(exifData, "Exif.Image.Software", &m_Exif->fSoftware);
        getExifTag(exifData, "Exif.Photo.ApertureValue", 0, &m_Exif->fApertureValue);
#if (EXIV2_TEST_VERSION(0,22,0))
        getExifTag(exifData, "Exif.Photo.BodySerialNumber", &m_Exif->fCameraSerialNumber);
#endif
        getExifTag(exifData, "Exif.Photo.BrightnessValue", 0, &m_Exif->fBrightnessValue);
#if (EXIV2_TEST_VERSION(0,22,0))
        getExifTag(exifData, "Exif.Photo.CameraOwnerName", &m_Exif->fOwnerName);
#endif
        getExifTag(exifData, "Exif.Photo.ComponentsConfiguration", 0, &m_Exif->fComponentsConfiguration);
        getExifTag(exifData, "Exif.Photo.Contrast", 0, &m_Exif->fContrast);
        getExifTag(exifData, "Exif.Photo.CustomRendered", 0, &m_Exif->fCustomRendered);
        getExifTag(exifData, "Exif.Photo.DateTimeDigitized", &m_Exif->fDateTimeDigitized);
        getExifTag(exifData, "Exif.Photo.DateTimeOriginal", &m_Exif->fDateTimeOriginal);
        getExifTag(exifData, "Exif.Photo.DigitalZoomRatio", 0, &m_Exif->fDigitalZoomRatio);
        getExifTag(exifData, "Exif.Photo.ExposureBiasValue", 0, &m_Exif->fExposureBiasValue);
        getExifTag(exifData, "Exif.Photo.ExposureMode", 0, &m_Exif->fExposureMode);
        getExifTag(exifData, "Exif.Photo.ExposureProgram", 0, &m_Exif->fExposureProgram);
        getExifTag(exifData, "Exif.Photo.ExposureTime", 0, &m_Exif->fExposureTime);
        getExifTag(exifData, "Exif.Photo.FileSource", 0, &m_Exif->fFileSource);
        getExifTag(exifData, "Exif.Photo.Flash", 0, &m_Exif->fFlash);
        getExifTag(exifData, "Exif.Photo.FNumber", 0, &m_Exif->fFNumber);
        getExifTag(exifData, "Exif.Photo.FocalPlaneResolutionUnit", 0, &m_Exif->fFocalPlaneResolutionUnit);
        getExifTag(exifData, "Exif.Photo.FocalPlaneXResolution", 0, &m_Exif->fFocalPlaneXResolution);
        getExifTag(exifData, "Exif.Photo.FocalPlaneYResolution", 0, &m_Exif->fFocalPlaneYResolution);
        getExifTag(exifData, "Exif.Photo.FocalLength", 0, &m_Exif->fFocalLength);
        getExifTag(exifData, "Exif.Photo.FocalLengthIn35mmFilm", 0, &m_Exif->fFocalLengthIn35mmFilm);
        getExifTag(exifData, "Exif.Photo.GainControl", 0, &m_Exif->fGainControl);
        getExifTag(exifData, "Exif.Photo.ISOSpeedRatings", 0, &m_Exif->fISOSpeedRatings[0]);
        getExifTag(exifData, "Exif.Photo.LightSource", 0, &m_Exif->fLightSource);
#if (EXIV2_TEST_VERSION(0,22,0))
        getExifTag(exifData, "Exif.Photo.LensModel", &m_Exif->fLensName);
        getExifTag(exifData, "Exif.Photo.LensSpecification", 0, &m_Exif->fLensInfo[0]);
        getExifTag(exifData, "Exif.Photo.LensSpecification", 1, &m_Exif->fLensInfo[1]);
        getExifTag(exifData, "Exif.Photo.LensSpecification", 2, &m_Exif->fLensInfo[2]);
        getExifTag(exifData, "Exif.Photo.LensSpecification", 3, &m_Exif->fLensInfo[3]);
        getExifTag(exifData, "Exif.Photo.LensSerialNumber", &m_Exif->fLensSerialNumber);
#endif
        getExifTag(exifData, "Exif.Photo.MaxApertureValue", 0, &m_Exif->fMaxApertureValue);
        getExifTag(exifData, "Exif.Photo.MeteringMode", 0, &m_Exif->fMeteringMode);
        getExifTag(exifData, "Exif.Photo.PixelXDimension", 0, &m_Exif->fPixelXDimension);
        getExifTag(exifData, "Exif.Photo.PixelYDimension", 0, &m_Exif->fPixelYDimension);
        getExifTag(exifData, "Exif.Photo.Saturation", 0, &m_Exif->fSaturation);
        getExifTag(exifData, "Exif.Photo.SceneCaptureType", 0, &m_Exif->fSceneCaptureType);
        getExifTag(exifData, "Exif.Photo.SceneType", 0, &m_Exif->fSceneType);
        getExifTag(exifData, "Exif.Photo.SensingMethod", 0, &m_Exif->fSensingMethod);
        getExifTag(exifData, "Exif.Photo.Sharpness", 0, &m_Exif->fSharpness);
        getExifTag(exifData, "Exif.Photo.ShutterSpeedValue", 0, &m_Exif->fShutterSpeedValue);
        getExifTag(exifData, "Exif.Photo.SubjectDistance", 0, &m_Exif->fSubjectDistance);
        getExifTag(exifData, "Exif.Photo.SubjectDistanceRange", 0, &m_Exif->fSubjectDistanceRange);
        getExifTag(exifData, "Exif.Photo.UserComment", &m_Exif->fUserComment);
        getExifTag(exifData, "Exif.Photo.WhiteBalance", 0, &m_Exif->fWhiteBalance);
        getExifTag(exifData, "Exif.GPSInfo.GPSAltitude", 0, &m_Exif->fGPSAltitude);
        getExifTag(exifData, "Exif.GPSInfo.GPSAltitudeRef", 0, &m_Exif->fGPSAltitudeRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSAreaInformation", &m_Exif->fGPSAreaInformation);
        getExifTag(exifData, "Exif.GPSInfo.GPSDateStamp", &m_Exif->fGPSDateStamp);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestBearing", 0, &m_Exif->fGPSDestBearing);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestBearingRef", &m_Exif->fGPSDestBearingRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestDistance", 0, &m_Exif->fGPSDestDistance);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestDistanceRef", &m_Exif->fGPSDestDistanceRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLatitude", 0, &m_Exif->fGPSDestLatitude[0]);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLatitude", 1, &m_Exif->fGPSDestLatitude[1]);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLatitude", 2, &m_Exif->fGPSDestLatitude[2]);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLatitudeRef", &m_Exif->fGPSDestLatitudeRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLongitude", 0, &m_Exif->fGPSDestLongitude[0]);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLongitude", 1, &m_Exif->fGPSDestLongitude[1]);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLongitude", 2, &m_Exif->fGPSDestLongitude[2]);
        getExifTag(exifData, "Exif.GPSInfo.GPSDestLongitudeRef", &m_Exif->fGPSDestLongitudeRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSDifferential", 0, &m_Exif->fGPSDifferential);
        getExifTag(exifData, "Exif.GPSInfo.GPSDOP", 0, &m_Exif->fGPSDOP);
        getExifTag(exifData, "Exif.GPSInfo.GPSImgDirection", 0, &m_Exif->fGPSImgDirection);
        getExifTag(exifData, "Exif.GPSInfo.GPSImgDirectionRef", &m_Exif->fGPSImgDirectionRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSLatitude", 0, &m_Exif->fGPSLatitude[0]);
        getExifTag(exifData, "Exif.GPSInfo.GPSLatitude", 1, &m_Exif->fGPSLatitude[1]);
        getExifTag(exifData, "Exif.GPSInfo.GPSLatitude", 2, &m_Exif->fGPSLatitude[2]);
        getExifTag(exifData, "Exif.GPSInfo.GPSLatitudeRef", &m_Exif->fGPSLatitudeRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSLongitude", 0, &m_Exif->fGPSLongitude[0]);
        getExifTag(exifData, "Exif.GPSInfo.GPSLongitude", 1, &m_Exif->fGPSLongitude[1]);
        getExifTag(exifData, "Exif.GPSInfo.GPSLongitude", 2, &m_Exif->fGPSLongitude[2]);
        getExifTag(exifData, "Exif.GPSInfo.GPSLongitudeRef", &m_Exif->fGPSLongitudeRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSMapDatum", &m_Exif->fGPSMapDatum);
        getExifTag(exifData, "Exif.GPSInfo.GPSMeasureMode", &m_Exif->fGPSMeasureMode);
        getExifTag(exifData, "Exif.GPSInfo.GPSProcessingMethod", &m_Exif->fGPSProcessingMethod);
        getExifTag(exifData, "Exif.GPSInfo.GPSSatellites", &m_Exif->fGPSSatellites);
        getExifTag(exifData, "Exif.GPSInfo.GPSSpeed", 0, &m_Exif->fGPSSpeed);
        getExifTag(exifData, "Exif.GPSInfo.GPSSpeedRef", &m_Exif->fGPSSpeedRef);
        getExifTag(exifData, "Exif.GPSInfo.GPSStatus", &m_Exif->fGPSStatus);
        getExifTag(exifData, "Exif.GPSInfo.GPSTimeStamp", 0, &m_Exif->fGPSTimeStamp[0]);
        getExifTag(exifData, "Exif.GPSInfo.GPSTimeStamp", 1, &m_Exif->fGPSTimeStamp[1]);
        getExifTag(exifData, "Exif.GPSInfo.GPSTimeStamp", 2, &m_Exif->fGPSTimeStamp[2]);
        getExifTag(exifData, "Exif.GPSInfo.GPSTrack", 0, &m_Exif->fGPSTrack);
        getExifTag(exifData, "Exif.GPSInfo.GPSTrackRef", &m_Exif->fGPSTrackRef);
        uint32 gpsVer1 = 0;
        uint32 gpsVer2 = 0;
        uint32 gpsVer3 = 0;
        uint32 gpsVer4 = 0;
        getExifTag(exifData, "Exif.GPSInfo.GPSVersionID", 0, &gpsVer1);
        getExifTag(exifData, "Exif.GPSInfo.GPSVersionID", 1, &gpsVer2);
        getExifTag(exifData, "Exif.GPSInfo.GPSVersionID", 2, &gpsVer3);
        getExifTag(exifData, "Exif.GPSInfo.GPSVersionID", 3, &gpsVer4);
        uint32 gpsVer = 0;
        gpsVer += gpsVer1 << 24;
        gpsVer += gpsVer2 << 16;
        gpsVer += gpsVer3 <<  8;
        gpsVer += gpsVer4;
        m_Exif->fGPSVersionID = gpsVer;

        // Nikon Makernotes

        getExifTag(exifData, "Exif.Nikon3.Lens", 0, &m_Exif->fLensInfo[0]);
        getExifTag(exifData, "Exif.Nikon3.Lens", 1, &m_Exif->fLensInfo[1]);
        getExifTag(exifData, "Exif.Nikon3.Lens", 2, &m_Exif->fLensInfo[2]);
        getExifTag(exifData, "Exif.Nikon3.Lens", 3, &m_Exif->fLensInfo[3]);
        if (getExifTag(exifData, "Exif.Nikon3.ISOSpeed", 1, &urat))
            m_Exif->fISOSpeedRatings[0] = urat.n;

        if (getExifTag(exifData, "Exif.Nikon3.SerialNO", &m_Exif->fCameraSerialNumber))
        {
            m_Exif->fCameraSerialNumber.Replace("NO=", "", false);
            m_Exif->fCameraSerialNumber.TrimLeadingBlanks();
            m_Exif->fCameraSerialNumber.TrimTrailingBlanks();
        }

        getExifTag(exifData, "Exif.Nikon3.SerialNumber", &m_Exif->fCameraSerialNumber);

        getExifTag(exifData, "Exif.Nikon3.ShutterCount", 0, &m_Exif->fImageNumber);
        if (getExifTag(exifData, "Exif.NikonLd1.LensIDNumber", 0, &val))
        {
            char lensType[256];
            snprintf(lensType, sizeof(lensType), "%i", val);
            m_Exif->fLensID.Set_ASCII(lensType);
        }
        if (getExifTag(exifData, "Exif.NikonLd2.LensIDNumber", 0, &val))
        {
            char lensType[256];
            snprintf(lensType, sizeof(lensType), "%i", val);
            m_Exif->fLensID.Set_ASCII(lensType);
        }
        if (getExifTag(exifData, "Exif.NikonLd3.LensIDNumber", 0, &val))
        {
            char lensType[256];
            snprintf(lensType, sizeof(lensType), "%i", val);
            m_Exif->fLensID.Set_ASCII(lensType);
        }
        if (getExifTag(exifData, "Exif.NikonLd2.FocusDistance", 0, &val))
            m_Exif->fSubjectDistance = dng_urational(static_cast<uint32>(pow(10.0, val/40.0)), 100);
        if (getExifTag(exifData, "Exif.NikonLd3.FocusDistance", 0, &val))
            m_Exif->fSubjectDistance = dng_urational(static_cast<uint32>(pow(10.0, val/40.0)), 100);
        getExifTag(exifData, "Exif.NikonLd1.LensIDNumber", &m_Exif->fLensName);
        getExifTag(exifData, "Exif.NikonLd2.LensIDNumber", &m_Exif->fLensName);
        getExifTag(exifData, "Exif.NikonLd3.LensIDNumber", &m_Exif->fLensName);

        // Canon Makernotes

        if (getExifTag(exifData, "Exif.Canon.SerialNumber", 0, &val))
        {
            char serialNumber[256];
            snprintf(serialNumber, sizeof(serialNumber), "%i", val);
            m_Exif->fCameraSerialNumber.Set_ASCII(serialNumber);
            m_Exif->fCameraSerialNumber.TrimLeadingBlanks();
            m_Exif->fCameraSerialNumber.TrimTrailingBlanks();
        }

        if (getExifTag(exifData, "Exif.CanonCs.ExposureProgram", 0, &val))
        {
            switch (val)
            {
            case 1:
                m_Exif->fExposureProgram = 2;
                break;
            case 2:
                m_Exif->fExposureProgram = 4;
                break;
            case 3:
                m_Exif->fExposureProgram = 3;
                break;
            case 4:
                m_Exif->fExposureProgram = 1;
                break;
            default:
                break;
            }
        }
        if (getExifTag(exifData, "Exif.CanonCs.MeteringMode", 0, &val))
        {
            switch (val)
            {
            case 1:
                m_Exif->fMeteringMode = 3;
                break;
            case 2:
                m_Exif->fMeteringMode = 1;
                break;
            case 3:
                m_Exif->fMeteringMode = 5;
                break;
            case 4:
                m_Exif->fMeteringMode = 6;
                break;
            case 5:
                m_Exif->fMeteringMode = 2;
                break;
            default:
                break;
            }
        }

        if (getExifTag(exifData, "Exif.CanonCs.MaxAperture", 0, &val))
            m_Exif->fMaxApertureValue = dng_urational(val, 32);

        if (getExifTag(exifData, "Exif.CanonSi.SubjectDistance", 0, &val) && (val > 0))
            m_Exif->fSubjectDistance = dng_urational(val, 100);

        uint32 canonLensUnits = 1;
        if (getExifTag(exifData, "Exif.CanonCs.Lens", 2, &urat))
            canonLensUnits = urat.n;
        if (getExifTag(exifData, "Exif.CanonCs.Lens", 0, &urat))
            m_Exif->fLensInfo[1] = dng_urational(urat.n, canonLensUnits);
        if (getExifTag(exifData, "Exif.CanonCs.Lens", 1, &urat))
            m_Exif->fLensInfo[0] = dng_urational(urat.n, canonLensUnits);
        if (getExifTag(exifData, "Exif.Canon.FocalLength", 1, &urat))
            m_Exif->fFocalLength = dng_urational(urat.n, canonLensUnits);
        uint32 canonLensType = 65535;

        if ((getExifTag(exifData, "Exif.CanonCs.LensType", 0, &canonLensType)) &&
                (canonLensType != 65535))
        {
            char lensType[256];
            snprintf(lensType, sizeof(lensType), "%i", canonLensType);
            m_Exif->fLensID.Set_ASCII(lensType);
        }
        if (getExifTag(exifData, "Exif.Canon.LensModel", &m_Exif->fLensName))
        {

        }
        else if (canonLensType != 65535)
        {
            getExifTag(exifData, "Exif.CanonCs.LensType", &m_Exif->fLensName);
        }

        getExifTag(exifData, "Exif.Canon.OwnerName", &m_Exif->fOwnerName);
        getExifTag(exifData, "Exif.Canon.OwnerName", &m_Exif->fArtist);

        if (getExifTag(exifData, "Exif.Canon.FirmwareVersion", &m_Exif->fFirmware))
        {
            m_Exif->fFirmware.Replace("Firmware", "");
            m_Exif->fFirmware.Replace("Version", "");
            m_Exif->fFirmware.TrimLeadingBlanks();
            m_Exif->fFirmware.TrimTrailingBlanks();
        }

        if (getExifTag(exifData, "Exif.CanonSi.ISOSpeed", &str))
        {
            sscanf(str.Get(), "%i", &val);
            m_Exif->fISOSpeedRatings[0] = val;
        }

        getExifTag(exifData, "Exif.Canon.FileNumber", 0, &m_Exif->fImageNumber);
        getExifTag(exifData, "Exif.CanonFi.FileNumber", 0, &m_Exif->fImageNumber);

        // Pentax Makernotes

        getExifTag(exifData, "Exif.Pentax.LensType", &m_Exif->fLensName);

        uint32 pentaxLensId1 = 0;
        uint32 pentaxLensId2 = 0;
        if ((getExifTag(exifData, "Exif.Pentax.LensType", 0, &pentaxLensId1)) &&
                (getExifTag(exifData, "Exif.Pentax.LensType", 1, &pentaxLensId2)))
        {
            char lensType[256];
            snprintf(lensType, sizeof(lensType), "%i %i", pentaxLensId1, pentaxLensId2);
            m_Exif->fLensID.Set_ASCII(lensType);
        }

        // Olympus Makernotes

        getExifTag(exifData, "Exif.OlympusEq.SerialNumber", &m_Exif->fCameraSerialNumber);
        getExifTag(exifData, "Exif.OlympusEq.LensSerialNumber", &m_Exif->fLensSerialNumber);
        getExifTag(exifData, "Exif.OlympusEq.LensModel", &m_Exif->fLensName);

        if (getExifTag(exifData, "Exif.OlympusEq.MinFocalLength", 0, &val))
            m_Exif->fLensInfo[0] = dng_urational(val, 1);
        if (getExifTag(exifData, "Exif.OlympusEq.MaxFocalLength", 0, &val))
            m_Exif->fLensInfo[1] = dng_urational(val, 1);

        // Panasonic Makernotes

        getExifTag(exifData, "Exif.Panasonic.LensType", &m_Exif->fLensName);
        getExifTag(exifData, "Exif.Panasonic.LensSerialNumber", &m_Exif->fLensSerialNumber);

        // Sony Makernotes

        if (getExifTag(exifData, "Exif.Sony2.LensID", 0, &val))
        {
            char lensType[256];
            snprintf(lensType, sizeof(lensType), "%i", val);
            m_Exif->fLensID.Set_ASCII(lensType);
        }

        getExifTag(exifData, "Exif.Sony2.LensID", &m_Exif->fLensName);

        //

        if ((m_Exif->fLensName.IsEmpty()) &&
                (m_Exif->fLensInfo[0].As_real64() > 0) &&
                (m_Exif->fLensInfo[1].As_real64() > 0))
        {
            char lensName1[256];
            if (m_Exif->fLensInfo[0].As_real64() == m_Exif->fLensInfo[1].As_real64())
            {
                snprintf(lensName1, sizeof(lensName1), "%.1f mm", m_Exif->fLensInfo[0].As_real64());
            }
            else
            {
                snprintf(lensName1, sizeof(lensName1), "%.1f-%.1f mm", m_Exif->fLensInfo[0].As_real64(), m_Exif->fLensInfo[1].As_real64());
            }

            std::string lensName(lensName1);

            if ((m_Exif->fLensInfo[2].As_real64() > 0) &&
                    (m_Exif->fLensInfo[3].As_real64() > 0))
            {
                char lensName2[256];
                if (m_Exif->fLensInfo[2].As_real64() == m_Exif->fLensInfo[3].As_real64())
                {
                    snprintf(lensName2, sizeof(lensName2), " f/%.1f", m_Exif->fLensInfo[2].As_real64());
                }
                else
                {
                    snprintf(lensName2, sizeof(lensName2), " f/%.1f-%.1f", m_Exif->fLensInfo[2].As_real64(), m_Exif->fLensInfo[3].As_real64());
                }
                lensName.append(lensName2);
            }

            m_Exif->fLensName.Set_ASCII(lensName.c_str());
        }

        // XMP

        std::string xmpPacket;
        if (0 != Exiv2::XmpParser::encode(xmpPacket, xmpData))
        {
            throw Exiv2::Error(1, "Failed to serialize XMP data");
        }
        Exiv2::XmpParser::terminate();

        m_XMP.Reset(new dng_xmp(host.Allocator()));
        m_XMP->Parse(host, xmpPacket.c_str(), xmpPacket.length());
        if (getExifTag(exifData, "Exif.Image.Orientation", 0, &val))
        {
            dng_orientation orientation;
            orientation.SetTIFF(val);
            m_XMP->SetOrientation(orientation);
        }

        // Makernotes

        m_MakerNoteOffset = 0;
        m_MakerNoteByteOrder.Set_ASCII("");
        if (getExifTag(exifData, "Exif.MakerNote.Offset", 0, &m_MakerNoteOffset) &&
                getExifTag(exifData, "Exif.MakerNote.ByteOrder", &m_MakerNoteByteOrder))
        {
            long bufsize = 0;
            unsigned char* buffer = 0;
            if (getExifTagData(exifData, "Exif.Photo.MakerNote", &bufsize, &buffer))
            {
                m_MakerNote.Reset(host.Allocate(bufsize));
                memcpy(m_MakerNote->Buffer(), buffer, bufsize);
                delete[] buffer;
            }
        }
    }
    catch( Exiv2::Error& e )
    {
        printExiv2ExceptionError("Cannot load metadata", e);
    }
}

void Exiv2Meta::PostParse (dng_host& /*host*/)
{

}
