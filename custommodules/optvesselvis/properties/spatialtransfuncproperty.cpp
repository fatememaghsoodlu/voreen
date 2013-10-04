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

#include "spatialtransfuncproperty.h"
#include "voreen/core/properties/condition.h"
#include "voreen/core/datastructures/transfunc/transfunc1dkeys.h"
#include "voreen/core/datastructures/transfunc/transfunc2dprimitives.h"
#include "voreen/core/datastructures/transfunc/transfuncmappingkey.h"

#include "voreen/core/datastructures/volume/volume.h"
#include "voreen/core/datastructures/volume/volumeminmax.h"

namespace voreen {

const std::string SpatialTransFuncProperty::loggerCat_("voreen.SpatialTransFuncProperty");

SpatialTransFuncProperty::SpatialTransFuncProperty(const std::string& ident, const std::string& guiText, int invalidationLevel,
                             SpatialTransFuncProperty::Editors editors, bool lazyEditorInstantiation)
    : TemplateProperty<TransFunc*>(ident, guiText, 0, invalidationLevel)
    , volume_(0)
    , paramHandle_(0)
    , editors_(editors)
    , lazyEditorInstantiation_(lazyEditorInstantiation)
    , alwaysFitDomain_(false)
{
    paramMagnitude_ = 1.f;
    windowMode_ = 0;
    lockKeys_ = false;
}

SpatialTransFuncProperty::SpatialTransFuncProperty()
    : TemplateProperty<TransFunc*>("", "", 0, Processor::INVALID_RESULT)
    , volume_(0)
{}

SpatialTransFuncProperty::~SpatialTransFuncProperty() {
/*    if (value_) {
        LWARNING(getFullyQualifiedGuiName() << " has not been deinitialized before destruction.");
    } */

    delete value_;
    value_ = 0;
}

Property* SpatialTransFuncProperty::create() const {
    return new SpatialTransFuncProperty();
}

void SpatialTransFuncProperty::reset() {
    if(value_)
        value_->reset();
}

void SpatialTransFuncProperty::enableEditor(SpatialTransFuncProperty::Editors editor) {
    editors_ |= editor;
}

void SpatialTransFuncProperty::disableEditor(SpatialTransFuncProperty::Editors editor) {
    editors_ &= ~editor;
}

bool SpatialTransFuncProperty::isEditorEnabled(SpatialTransFuncProperty::Editors editor) {
    return (editors_ & editor);
}

void SpatialTransFuncProperty::set(TransFunc* tf) {

    //Normally this method is only called when the type of transfer function
    //changes. The plugins are working with a pointer and thus they only call
    //notifyChange() when the transfer function changes.

    // tf object already assigned
    if (tf == value_) {
        return;
    }

    // new tf object assigned -> check if contents are equal
    TransFunc1DKeys* tf_int = dynamic_cast<TransFunc1DKeys*>(tf);
    TransFunc1DKeys* value_int = dynamic_cast<TransFunc1DKeys*>(value_);
    TransFunc2DPrimitives* tf_grad = dynamic_cast<TransFunc2DPrimitives*>(tf);
    TransFunc2DPrimitives* value_grad = dynamic_cast<TransFunc2DPrimitives*>(value_);

    // assign new object, but store previous one for deletion
    TransFunc* oldValue = value_;
    value_ = tf;

    // both transfer functions are of type TransFuncIntensity
    if (tf_int && value_int) {
        if (*tf_int == *value_int) {
            // keys are equal -> only inform widgets about new object
            updateWidgets();
        }
        else {
            // the pointer is the same but keys are different -> notify change
            notifyChange();
        }
    }
    // both transfer functions are of type TransFuncIntensityGradient
    else if (tf_grad && value_grad) {
        if (*tf_grad == *value_grad) {
            // primitives are equal ->only inform widgets about new object
            updateWidgets();
        }
        else {
            // primitives are different -> notify change
            notifyChange();
        }
    }
    // last case: tf type has changed -> additionally shaders may have to be recompiled
    else {
        invalidateOwner(Processor::INVALID_PROGRAM);
        notifyChange();
    }

    // finally delete previously assigned object
    delete oldValue;
}

void SpatialTransFuncProperty::setVolumeHandle(const VolumeBase* handle) {

    if (volume_ != handle) {

        volume_ = handle;
        if (volume_) {
            handle->addObserver(this);

            // Resize texture of tf according to bitdepth of volume
            int bits = volume_->getRepresentation<VolumeRAM>()->getBitsAllocated() / volume_->getRepresentation<VolumeRAM>()->getNumChannels();
            if (bits > 16)
                bits = 16; // handle float data as if it was 16 bit to prevent overflow

            int max = static_cast<int>(pow(2.f, bits));

            if (TransFunc1DKeys* tfi = dynamic_cast<TransFunc1DKeys*>(value_)) {
                value_->resize(max);
                RealWorldMapping rwm = volume_->getRealWorldMapping();
                if((((rwm.getOffset() != 0.0f) || (rwm.getScale() != 1.0f) || (rwm.getUnit() != "")) && *tfi == TransFunc1DKeys()) || alwaysFitDomain_)
                    fitDomainToData();
            } else if (dynamic_cast<TransFunc2DPrimitives*>(value_)) {
                // limit 2D tfs to 10 bit for reducing memory consumption
                max = std::min(max, 1024);
                value_->resize(max, max);
            }

        }

        updateWidgets();
    }
}

void SpatialTransFuncProperty::setParamHandle(const VolumeBase* handle) {
    if (paramHandle_ != handle) {
        paramHandle_ = handle;
        updateWidgets();
    }
}

void SpatialTransFuncProperty::fitDomainToData() {
    if(!volume_)
        return;

    if (TransFunc1DKeys* tfi = dynamic_cast<TransFunc1DKeys*>(value_)) {
        RealWorldMapping rwm = volume_->getRealWorldMapping();
        float min = rwm.normalizedToRealWorld(volume_->getDerivedData<VolumeMinMax>()->getMinNormalized());
        float max = rwm.normalizedToRealWorld(volume_->getDerivedData<VolumeMinMax>()->getMaxNormalized());
        tfi->setDomain(tgt::vec2(min, max));
        //notifyChange();
        invalidateOwner();
    }
}

const VolumeBase* SpatialTransFuncProperty::getVolumeHandle() const {
    return volume_;
}

const VolumeBase* SpatialTransFuncProperty::getParamHandle() const {
    return paramHandle_;
}

void SpatialTransFuncProperty::notifyChange() {
    executeLinks();

    // check if conditions are met and exec actions
    for (size_t j = 0; j < conditions_.size(); ++j)
        conditions_[j]->exec();

    updateWidgets();

    // invalidate owner:
    invalidateOwner();
}

void SpatialTransFuncProperty::volumeDelete(const VolumeBase* source) {
    if (volume_ == source) {
        volume_ = 0;
        updateWidgets();
    }
}

void SpatialTransFuncProperty::volumeChange(const VolumeBase* source) {
    if (volume_ == source)
        updateWidgets();
}

void SpatialTransFuncProperty::serialize(XmlSerializer& s) const {
    Property::serialize(s);

    s.serialize("TransferFunction", value_);
    s.serialize("AlwaysFitDomain", alwaysFitDomain_);
    s.serialize("WindowMode", windowMode_);
    s.serialize("LockKeys", lockKeys_);
    s.serialize("ParamMagnitude", paramMagnitude_);
}

void SpatialTransFuncProperty::deserialize(XmlDeserializer& s) {
    Property::deserialize(s);

    TransFunc* tf = 0;
    s.deserialize("TransferFunction", tf);
    set(tf);
    s.deserialize("WindowMode", windowMode_);
    s.deserialize("LockKeys", lockKeys_);
    s.deserialize("ParamMagnitude", paramMagnitude_);

    try {
        s.deserialize("AlwaysFitDomain", alwaysFitDomain_);
    } catch (XmlSerializationNoSuchDataException&) {
        s.removeLastError();
    }
}

void SpatialTransFuncProperty::initialize() throw (tgt::Exception) {

    TemplateProperty<TransFunc*>::initialize();

    // create initial transfer function, if it has not been created during deserialization
    if (!value_) {
        set(new TransFunc1DKeys());
        LGL_ERROR;
    }
}

void SpatialTransFuncProperty::deinitialize() throw (tgt::Exception) {
    if (value_) {
        value_->deleteTexture();
        LGL_ERROR;
    }

    TemplateProperty<TransFunc*>::deinitialize();
}

} // namespace voreen
