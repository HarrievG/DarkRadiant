/*
Copyright (C) 2001-2006, William Joseph.
All Rights Reserved.

This file is part of GtkRadiant.

GtkRadiant is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

GtkRadiant is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with GtkRadiant; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "ImageLoaderWx.h"

#include "itextstream.h"
#include "ifilesystem.h"

#include "RGBAImage.h"
#include "stream/ScopedArchiveBuffer.h"

#include <wx/image.h>
#include <wx/mstream.h>

namespace
{
    void copyWxImageToRGBAImage(const wxImage& src, RGBAImage& dest)
    {
        wxASSERT(src.GetWidth() == int(dest.width));
        wxASSERT(src.GetHeight() == int(dest.height));

        for (int r = 0; r < src.GetHeight(); ++r)
        for (int c = 0; c < src.GetWidth(); ++c)
        {
            RGBAPixel& destPix = dest.pixels[r * dest.width + c];

            destPix.red = src.GetRed(c, r);
            destPix.green = src.GetGreen(c, r);
            destPix.blue = src.GetBlue(c, r);

            destPix.alpha = src.HasAlpha() ? src.GetAlpha(c, r) : 255;
        }
    }
}

ImagePtr ImageLoaderWx::load(ArchiveFile& file) const
{
    ScopedArchiveBuffer buffer(file);

    // Construct a wxImage from the memory buffer
    wxMemoryInputStream inputStream(buffer.buffer, buffer.length);
    wxImage image(inputStream);

    // Copy into the required RGBAImage structure
    RGBAImagePtr rgbaImage(new RGBAImage(image.GetWidth(), image.GetHeight()));
    copyWxImageToRGBAImage(image, *rgbaImage);

    return rgbaImage;
}

std::string ImageLoaderWx::getExtension() const
{
    return "Wx";
}

void ImageLoaderWx::initialiseModule(const ApplicationContext& ctx)
{
    rMessage() << "ImageLoaderWx::initialiseModule called.\n";
}