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

#pragma once

#include <dng_auto_ptr.h>
#include <dng_exif.h>
#include <dng_host.h>
#include <dng_memory.h>
#include <dng_stream.h>
#include <dng_xmp.h>

class Exiv2Meta
{
public:
    Exiv2Meta();
    virtual ~Exiv2Meta();

    virtual void Parse(dng_host &host, dng_stream &stream);
    virtual void PostParse(dng_host &host);

    dng_exif* GetExif()
    {
        return m_Exif.Get();
    }

    const dng_exif* GetExif() const
    {
        return m_Exif.Get();
    }

    dng_xmp* GetXMP ()
    {
        return m_XMP.Get ();
    }

    const dng_xmp* GetXMP () const
    {
        return m_XMP.Get ();
    }

    const void* MakerNoteData() const
    {
        return m_MakerNote.Get() ? m_MakerNote->Buffer() : NULL;
    }

    uint32 MakerNoteLength() const
    {
        return m_MakerNote.Get() ? m_MakerNote->LogicalSize() : 0;
    }

    uint32 MakerNoteOffset() const
    {
        return m_MakerNoteOffset;
    }

    const dng_string& MakerNoteByteOrder()
    {
        return m_MakerNoteByteOrder;
    }

private:
    AutoPtr<dng_exif> m_Exif;
    AutoPtr<dng_xmp> m_XMP;
    AutoPtr<dng_memory_block> m_MakerNote;
    uint32 m_MakerNoteOffset;
    dng_string m_MakerNoteByteOrder;
};
