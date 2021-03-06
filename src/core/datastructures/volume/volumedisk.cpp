/***********************************************************************************
 *                                                                                 *
 * Voreen - The Volume Rendering Engine                                            *
 *                                                                                 *
 * Copyright (C) 2005-2013 University of Muenster, Germany.                        *
 * Visualization and Computer Graphics Group <http://viscg.uni-muenster.de>        *
 * For a list of authors please refer to the file "CREDITS.txt".                   *
 *                                                                                 *
 * This file is part of the Voreen software package. Voreen is free software:      *
 * you can redistribute it and/or modify it under the terms of the GNU General     *
 * Public License version 2 as published by the Free Software Foundation.          *
 *                                                                                 *
 * Voreen is distributed in the hope that it will be useful, but WITHOUT ANY       *
 * WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR   *
 * A PARTICULAR PURPOSE. See the GNU General Public License for more details.      *
 *                                                                                 *
 * You should have received a copy of the GNU General Public License in the file   *
 * "LICENSE.txt" along with this file. If not, see <http://www.gnu.org/licenses/>. *
 *                                                                                 *
 * For non-commercial academic use see the license exception specified in the file *
 * "LICENSE-academic.txt". To get information about commercial licensing please    *
 * contact the authors.                                                            *
 *                                                                                 *
 ***********************************************************************************/

#include "voreen/core/datastructures/volume/volumedisk.h"

#include "voreen/core/datastructures/volume/volumeatomic.h"
#include "voreen/core/datastructures/volume/volumefactory.h"
#include "voreen/core/datastructures/volume/operators/volumeoperatorswapendianness.h"

#include "voreen/core/io/volumereader.h"
#include "voreen/core/utils/hashing.h"

#include <algorithm>
#include <typeinfo>
#include <fstream>

using tgt::vec3;
using tgt::bvec3;
using tgt::ivec3;
using tgt::svec3;
using tgt::col4;
using tgt::mat4;

namespace voreen {

const std::string VolumeDisk::loggerCat_("voreen.VolumeDisk");

VolumeDisk::VolumeDisk(const std::string& format, tgt::svec3 dimensions)
    : VolumeRepresentation(dimensions)
    , format_(format)
{}

std::string VolumeDisk::getBaseType() const {
    VolumeFactory vf;
    return vf.getBaseType(getFormat());
}

size_t VolumeDisk::getNumChannels() const {
    VolumeFactory vf;
    return vf.getNumChannels(getFormat());
}

size_t VolumeDisk::getBytesPerVoxel() const {
    VolumeFactory vf;
    return vf.getBytesPerVoxel(getFormat());
}

//-------------------------------------------------------------------------------------------------

const std::string VolumeDiskRaw::loggerCat_("voreen.VolumeDiskRaw");

VolumeDiskRaw::VolumeDiskRaw(const std::string& filename, const std::string& format, tgt::svec3 dimensions, int64_t offset /*= 0*/, bool swapEndian /*= false*/)
    : VolumeDisk(format, dimensions)
    , filename_(filename)
    , offset_(offset)
    , swapEndian_(swapEndian)
{}

VolumeDiskRaw::VolumeDiskRaw(const VolumeDiskRaw* diskrep)
    : VolumeDisk(diskrep->getFormat(), diskrep->getDimensions())
    , filename_(diskrep->getFileName())
    , offset_(diskrep->getOffset())
    , swapEndian_(diskrep->getSwapEndian())
{}

VolumeDiskRaw::~VolumeDiskRaw() {
}

std::string VolumeDiskRaw::getHash() const {
    std::string configStr = getFileName() + "#";
    configStr += getFormat() + "#";
    configStr += genericToString(getDimensions()) + "#";
    configStr += itos((int)offset_) + "#";
    configStr += (swapEndian_ ? "true" : "false");

    return VoreenHash::getHash(configStr);
}

VolumeRAM* VolumeDiskRaw::loadVolume() const
    throw (tgt::Exception)
{
    VolumeRAM* volume = 0;
    LDEBUG("Creating volume from diskrepr. " << getFileName() << " format: " << getFormat());
    VolumeFactory vf;
    try {
        volume = vf.create(getFormat(), getDimensions());
    }
    catch (std::bad_alloc&) {
        throw tgt::Exception("bad allocation");
    }
    if (!volume)
        throw VoreenException("Failed to create VolumeRAM");

    FILE* fin;
    fin = fopen(getFileName().c_str(),"rb");

    if (fin == 0) {
        throw tgt::IOException("Unable to open raw file for reading", getFileName());
    }

    size_t bytesPerVoxel = static_cast<size_t>(volume->getBitsAllocated() / 8);
    size_t numVoxels = hmul(getDimensions());
    size_t numBytes = numVoxels * bytesPerVoxel;

    int64_t offset = getOffset();
    if(offset < 0) {
        //Assume data is aligned to end of file.

        // get file size:
        fseek(fin, 0, SEEK_END);
        int64_t fileSize = ftell(fin);
        rewind(fin);

        //calculate real offset:
        offset = fileSize - numBytes;
    }
#ifdef _MSC_VER
    _fseeki64(fin, offset, SEEK_SET);
#else
    fseek(fin, offset, SEEK_SET);
#endif

    if(fread(reinterpret_cast<char*>(volume->getData()), numBytes, 1, fin) != 1) {
        //LERRORC("voreen.RepresentationConverterLoadFromDisk", "fread() failed");
        fclose(fin);
        delete volume;
        throw tgt::FileException("Failed to read from file: " + getFileName());
    }

    fclose(fin);

    if (getSwapEndian()) {
        Volume* tempHandle = new Volume(volume, vec3(1.0f), vec3(0.0f));
        VolumeOperatorSwapEndianness::APPLY_OP(tempHandle);
        tempHandle->releaseAllRepresentations();
        delete tempHandle;
    }

    return volume;
}

VolumeRAM* VolumeDiskRaw::loadSlices(const size_t firstSlice, const size_t lastSlice) const
    throw (tgt::Exception)
{
    //check for wrong parameter
    if(getDimensions().z <= lastSlice)
        throw std::invalid_argument("lastSlice is out of volume dimension!!!");
    if(firstSlice > lastSlice)
        throw std::invalid_argument("firstSlice has to be less or equal lastSlice!!!");

    //create new VolumeRam
    VolumeFactory vf;
    VolumeRAM* vr = vf.create(getFormat(), tgt::svec3(getDimensions().x,getDimensions().y, lastSlice-firstSlice+1));
    if (!vr)
        throw VoreenException("Failed to create VolumeRAM");

    //open file
    std::ifstream infile(getFileName().c_str(), std::ios::in | std::ios::binary);
    if (infile.fail())
        throw tgt::FileException("Failed to open file for reading: " + getFileName());

    size_t bytesPerVoxel = static_cast<size_t>(vr->getBitsAllocated() / 8);
    size_t numVoxels = getDimensions().x*getDimensions().y*(lastSlice-firstSlice+1);
    size_t numBytes = numVoxels * bytesPerVoxel;

    int64_t offset = getOffset();

    if(offset < 0) {
        //Assume data is aligned to end of file.

        // get file size:
        infile.seekg( 0, infile.end);
        std::streampos fileSize = infile.tellg();
        infile.seekg( 0, infile.beg);

        //calculate real offset:
        offset = static_cast<std::string::size_type>(fileSize) - hmul(getDimensions())*bytesPerVoxel;
    }

    //modify offset to start at first slice
    offset += getDimensions().x*getDimensions().y*firstSlice*bytesPerVoxel;
    infile.seekg(offset);

    //read into ram
    infile.read(reinterpret_cast<char*>(vr->getData()),numBytes);

    if (infile.fail()) {
        //LERRORC("voreen.RepresentationConverterLoadFromDisk", "read() failed");
        infile.close();
        delete vr;
        throw tgt::FileException("Failed to read from file: " + getFileName());
    }
    infile.close();

    //swap endian
    if(getSwapEndian()) {
        Volume* tempHandle = new Volume(vr, vec3(1.0f), vec3(0.0f));
        VolumeOperatorSwapEndianness::APPLY_OP(tempHandle);
        tempHandle->releaseAllRepresentations();
        delete tempHandle;
    }

    return vr;
}

VolumeRAM* VolumeDiskRaw::loadBrick(const tgt::svec3& pOffset, const tgt::svec3& pDimensions) const
    throw (tgt::Exception)
{
    //check for wrong parameter
    if(tgt::hmul(pDimensions) == 0)
        throw std::invalid_argument("requested brick dimensions are zero!");
    if(!tgt::hand(tgt::lessThanEqual(pOffset+pDimensions,getDimensions())))
        throw std::invalid_argument("requested brick outside volume date!");

    //create new VolumeRam
    VolumeFactory vf;
    VolumeRAM* vr = vf.create(getFormat(), pDimensions);
    if (!vr)
        throw VoreenException("Failed to create VolumeRAM");

    //open file
    FILE* fin;
    fin = fopen(getFileName().c_str(),"rb");
    if (!fin)
        throw tgt::FileException("Failed to open file for reading: " + getFileName());

    size_t bytesPerVoxel = static_cast<size_t>(vr->getBitsAllocated() / 8);
    size_t numVoxels = pDimensions.x;
    size_t numBytes = numVoxels * bytesPerVoxel;

    int64_t offset = getOffset();
    if(offset < 0) {
        //Assume data is aligned to end of file.

        // get file size:
        fseek(fin, 0, SEEK_END);
        int64_t fileSize = ftell(fin);
        rewind(fin);

        //calculate real offset:
        offset = fileSize - hmul(getDimensions())*bytesPerVoxel;
    }

    //modify offset to start at first slice
    offset += getDimensions().x*getDimensions().y*pOffset.z*bytesPerVoxel;

    fseek(fin, static_cast<long>(offset), SEEK_SET);

    //read into ram
    size_t pointerOffset = 0;
    for(size_t z = 0; z < pDimensions.z; z++){
        for(size_t y = 0; y < pDimensions.y; y++) {
            //read into ram
            if(fread(reinterpret_cast<char*>(vr->getData())+pointerOffset, numBytes, 1, fin) != 1) {
                fclose(fin);
                delete vr;
                throw tgt::FileException("Failed to read from file: " + getFileName());
            }
            //move offset
            pointerOffset += numBytes;

            //move to next read
            if(y < pDimensions.y)
                fseek(fin, static_cast<long>(getDimensions().x-pDimensions.x),SEEK_SET);
        }
        //move to next read
        if(z < pDimensions.z)
            fseek(fin,static_cast<long>((getDimensions().x*getDimensions().y)-(pDimensions.x*pDimensions.y)),SEEK_SET);
    }

    fclose(fin);

    //swap endian
    if(getSwapEndian()) {
        Volume* tempHandle = new Volume(vr, vec3(1.0f), vec3(0.0f));
        VolumeOperatorSwapEndianness::APPLY_OP(tempHandle);
        tempHandle->releaseAllRepresentations();
        delete tempHandle;
    }

    return vr;
}


//-------------------------------------------------------------------------------------------------

bool RepresentationConverterLoadFromDisk::canConvert(const VolumeRepresentation* source) const {
    if (dynamic_cast<const VolumeDisk*>(source))
        return true;
    else
        return false;
}

VolumeRepresentation* RepresentationConverterLoadFromDisk::convert(const VolumeRepresentation* source) const {
    tgtAssert(source, "null pointer passed");
    const VolumeDisk* volumeDisk = dynamic_cast<const VolumeDisk*>(source);

    if (volumeDisk) {
        try {
            VolumeRAM* volumeRam = volumeDisk->loadVolume();
            return volumeRam;
        }
        catch (tgt::Exception& e) {
            LERRORC("voreen.RepresentationConverterLoadFromDisk", "Unable to create VolumeRAM representation: " + std::string(e.what()));
            return 0;
        }
    }
    else {
        LERRORC("voreen.RepresentationConverterLoadFromDisk", "No VolumeDisk passed");
        return 0;
    }

}

//-------------------------------------------------------------------------------------------------

bool RepresentationConverterLoadFromDiskToGL::canConvert(const VolumeRepresentation* source) const {
    if (dynamic_cast<const VolumeDisk*>(source))
        return true;
    else
        return false;
}

VolumeRepresentation* RepresentationConverterLoadFromDiskToGL::convert(const VolumeRepresentation* source) const {
    // convert from disk to ram
    RepresentationConverterLoadFromDisk diskToRam;
    VolumeRepresentation* volumeRam = 0;
    if (diskToRam.canConvert(source))
        volumeRam = diskToRam.convert(source);
    if (!volumeRam) {
        LERRORC("voreen.RepresentationConverterLoadFromDiskToGL", "Failed to create RAM volume from disk volume");
        return 0;
    }

    // convert from ram to disk
    RepresentationConverterUploadGL ramToGL;
    VolumeRepresentation* volumeGL = 0;
    if (ramToGL.canConvert(volumeRam))
        volumeGL = ramToGL.convert(volumeRam);
    if (!volumeGL) {
        LERRORC("voreen.RepresentationConverterLoadFromDiskToGL", "Failed to create GL volume from RAM volume");
    }

    // free volumeRam
    delete volumeRam;
    volumeRam = 0;

    return volumeGL;
}

} // namespace voreen
