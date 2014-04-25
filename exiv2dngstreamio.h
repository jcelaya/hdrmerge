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

#include <stdlib.h>
#include <exiv2/basicio.hpp>

#include "dng_stream.h"
#include "dng_memory.h"

class Exiv2DngStreamIO : public Exiv2::BasicIo
{
public:
    Exiv2DngStreamIO(dng_stream& stream, dng_memory_allocator &allocator = gDefaultDNGMemoryAllocator);
    ~Exiv2DngStreamIO(void);

    virtual int open();
    virtual int close();
    virtual long write(const Exiv2::byte* data, long wcount);
    virtual long write(BasicIo& src);
    virtual int putb(Exiv2::byte data);
    virtual Exiv2::DataBuf read(long rcount);
    virtual long read(Exiv2::byte* buf, long rcount);
    virtual int getb();
    virtual void transfer(BasicIo& src);
    virtual int seek(long offset, Position pos);
    virtual Exiv2::byte* mmap(bool isWriteable =false);
    virtual int munmap();
    virtual long tell() const;
    virtual long size() const;
    virtual bool isopen() const;
    virtual int error() const;
    virtual bool eof() const;
    virtual std::string path() const;
#ifdef EXV_UNICODE_PATH
    virtual std::wstring wpath() const;
#endif
    virtual Exiv2::BasicIo::AutoPtr temporary() const;

protected:
    dng_memory_allocator &m_Allocator;
    std::auto_ptr<dng_memory_block> m_MemBlock;
    dng_stream& m_Stream;
};
