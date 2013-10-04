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

#include "spatialtransfunc1dkeyseditor.h"

#include "voreen/qt/widgets/transfunc/colorpicker.h"
#include "voreen/qt/widgets/transfunc/colorluminancepicker.h"
#include "voreen/qt/widgets/transfunc/doubleslider.h"
#include "voreen/qt/widgets/transfunc/transfunc1dkeyspainter.h"
#include "spatialtransfuncmappingcanvas.h"

#include "voreen/core/datastructures/transfunc/transfunc1dkeys.h"
#include "voreen/core/datastructures/volume/volumeram.h"
#include "voreen/core/datastructures/volume/volumeminmax.h"
#include "voreen/core/datastructures/meta/realworldmappingmetadata.h"

#include "tgt/logmanager.h"
#include "tgt/qt/qtcanvas.h"

#include <QApplication>
#include <QPushButton>
#include <QCheckBox>
#include <QFileDialog>
#include <QLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSplitter>
#include <QToolButton>

namespace voreen {

const std::string SpatialTransFunc1DKeysEditor::loggerCat_("voreen.qt.SpatialTransFunc1DKeysEditor");


//----------------------------------------------------------------------------------------------
//      Constructor and Qt Stuff
//----------------------------------------------------------------------------------------------
SpatialTransFunc1DKeysEditor::SpatialTransFunc1DKeysEditor(SpatialTransFuncProperty* prop, QWidget* parent,
                                                   Qt::Orientation orientation)
    : SpatialTransFuncEditor(prop, parent)
    , transCanvas_(0)
    , transferFuncIntensity_(0)
    , textureCanvas_(0)
    , texturePainter_(0)
    , doubleSlider_(0)
    , orientation_(orientation)
    , maxDigits_(7)
    , setTFValues_(true)
{
    title_ = QString("Intensity");
    transferFuncIntensity_ = dynamic_cast<TransFunc1DKeys*>(property_->get());
}

SpatialTransFunc1DKeysEditor::~SpatialTransFunc1DKeysEditor() {
}

QLayout* SpatialTransFunc1DKeysEditor::createMappingLayout() {
    transCanvas_ = new SpatialTransFuncMappingCanvas(0, transferFuncIntensity_);
    transCanvas_->setMinimumWidth(200);

    QWidget* additionalSpace = new QWidget();
    additionalSpace->setMinimumHeight(2);

    // threshold slider
    QHBoxLayout* hboxSlider = new QHBoxLayout();
    doubleSlider_ = new DoubleSlider();
    doubleSlider_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    doubleSlider_->setOffsets(12, 27);
    hboxSlider->addWidget(doubleSlider_);

    //spinboxes for threshold values
    lowerThresholdSpin_ = new QDoubleSpinBox();
    lowerThresholdSpin_->setRange(-9999999.0, 9999999.0);
    lowerThresholdSpin_->setValue(0.0);
    lowerThresholdSpin_->setDecimals(maxDigits_-1);
    lowerThresholdSpin_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lowerThresholdSpin_->setFixedWidth(6*maxDigits_+25);
    lowerThresholdSpin_->setKeyboardTracking(false);
    upperThresholdSpin_ = new QDoubleSpinBox();
    upperThresholdSpin_->setRange(-9999999.0, 9999999.0);
    upperThresholdSpin_->setValue(1.0);
    upperThresholdSpin_->setDecimals(maxDigits_-1);
    upperThresholdSpin_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    upperThresholdSpin_->setFixedWidth(6*maxDigits_+25);
    upperThresholdSpin_->setKeyboardTracking(false);
    QHBoxLayout* hboxSpin = new QHBoxLayout();
    QLabel* thresLabel = new QLabel("Threshold");
    //the spacing is added so that spinboxes and doubleslider are aligned vertically
    hboxSpin->addSpacing(6);
    hboxSpin->addWidget(lowerThresholdSpin_);
    hboxSpin->addStretch();
    hboxSpin->addWidget(thresLabel);
    hboxSpin->addStretch();
    hboxSpin->addWidget(upperThresholdSpin_);
    hboxSpin->addSpacing(21);

    //mapping settings:
    QHBoxLayout* hboxMapping = new QHBoxLayout();
    lowerMappingSpin_ = new QDoubleSpinBox();
    upperMappingSpin_ = new QDoubleSpinBox();
    upperMappingSpin_->setRange(-9999999.0, 9999999.0);
    lowerMappingSpin_->setRange(-9999999.0, 9999999.0);
    upperMappingSpin_->setValue(1.0);
    lowerMappingSpin_->setValue(0.0);
    upperMappingSpin_->setKeyboardTracking(false);
    lowerMappingSpin_->setKeyboardTracking(false);
    upperMappingSpin_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    lowerMappingSpin_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    upperMappingSpin_->setDecimals(maxDigits_-1);
    lowerMappingSpin_->setDecimals(maxDigits_-1);
    upperMappingSpin_->setFixedWidth(6*maxDigits_+25);
    lowerMappingSpin_->setFixedWidth(6*maxDigits_+25);


    QLabel* mappingLabel = new QLabel();
    mappingLabel->setText("TF Domain Bounds");

    hboxMapping->addSpacing(6);
    hboxMapping->addWidget(lowerMappingSpin_);
    hboxMapping->addStretch();
    hboxMapping->addWidget(mappingLabel);
    hboxMapping->addStretch();
    hboxMapping->addWidget(upperMappingSpin_);
    hboxMapping->addSpacing(21);

    QHBoxLayout* hboxAutoFit = new QHBoxLayout();

    fitDomainToData_ = new QPushButton();
    fitDomainToData_->setText("Fit to Data");

    alwaysFit_ = new QCheckBox();
    alwaysFit_->setText("Auto fit");
    //data bounds
    lowerData_ = new QLabel();
    //lowerData_->setReadOnly(true);
    upperData_ = new QLabel();
    //upperData_->setReadOnly(true);


    hboxAutoFit->addSpacing(6);
    hboxAutoFit->addWidget(lowerData_);
    hboxAutoFit->addStretch();
    hboxAutoFit->addWidget(fitDomainToData_);
    hboxAutoFit->addWidget(alwaysFit_);
    hboxAutoFit->addStretch();
    hboxAutoFit->addWidget(upperData_);
    hboxAutoFit->addSpacing(21);


    //add gradient that displays the transferfunction as image
    textureCanvas_ = new tgt::QtCanvas("", tgt::ivec2(1, 1), tgt::GLCanvas::RGBADD, 0, true);
    texturePainter_ = new TransFunc1DKeysPainter(textureCanvas_);
    texturePainter_->initialize();
    texturePainter_->setTransFunc(transferFuncIntensity_);
    textureCanvas_->setPainter(texturePainter_, false);
    textureCanvas_->setFixedHeight(12);
    textureCanvas_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    QHBoxLayout* hboxTexture = new QHBoxLayout();
    hboxTexture->addSpacing(12);
    hboxTexture->addWidget(textureCanvas_);
    hboxTexture->addSpacing(25);

    // put widgets in layout
    QVBoxLayout* vBox = new QVBoxLayout();
    vBox->setMargin(0);
    vBox->setSpacing(1);
    vBox->addStretch();
    vBox->addWidget(transCanvas_, 1);
    //vBox->addLayout(hboxTexture);//Widget(textureCanvas_);
    //vBox->addWidget(additionalSpace);
    //vBox->addLayout(hboxSlider);
    //vBox->addLayout(hboxSpin);
    //vBox->addLayout(hboxMapping);
    //vBox->addLayout(hboxAutoFit);
    //vBox->addSpacing(1);

    return vBox;
}

QLayout* SpatialTransFunc1DKeysEditor::createButtonLayout() {
    QBoxLayout* buttonLayout;
    if (orientation_ == Qt::Vertical)
        buttonLayout = new QHBoxLayout();
    else
        buttonLayout = new QVBoxLayout();

    clearButton_ = new QToolButton();
    clearButton_->setIcon(QIcon(":/qt/icons/clear.png"));
    clearButton_->setToolTip(tr("Reset to default transfer function"));

    loadButton_ = new QToolButton();
    loadButton_->setIcon(QIcon(":/qt/icons/open.png"));
    loadButton_->setToolTip(tr("Load transfer function"));

    saveButton_ = new QToolButton();
    saveButton_->setIcon(QIcon(":/qt/icons/save.png"));
    saveButton_->setToolTip(tr("Save transfer function"));

    //if (property_->getManualRepaint()) {
        //repaintButton_ = new QToolButton();
        //repaintButton_->setIcon(QIcon(":/qt/icons/view-refresh.png"));
        //repaintButton_->setToolTip(tr("Repaint the volume rendering"));
    //}

    buttonLayout->setSpacing(0);
    buttonLayout->setMargin(0);
    buttonLayout->addWidget(clearButton_);
    buttonLayout->addWidget(loadButton_);
    buttonLayout->addWidget(saveButton_);
    //if (property_->getManualRepaint())
        //buttonLayout->addWidget(repaintButton_);

    buttonLayout->addStretch();

    return buttonLayout;
}

QLayout* SpatialTransFunc1DKeysEditor::createColorLayout() {
    // ColorPicker
    colorPicker_ = new ColorPicker();
    colorPicker_->setFrameStyle(QFrame::Panel | QFrame::Sunken);
    colorPicker_->setMinimumWidth(100);
    colorPicker_->setMaximumHeight(150);

    // ColorLuminacePicker
    colorLumPicker_ = new ColorLuminancePicker();
    colorLumPicker_->setFixedWidth(20);
    colorLumPicker_->setMaximumHeight(150);


    QHBoxLayout* hBoxColor = new QHBoxLayout();
    hBoxColor->setMargin(0);
    hBoxColor->addWidget(colorPicker_);
    hBoxColor->addWidget(colorLumPicker_);

    if (orientation_ == Qt::Vertical)
        return hBoxColor;
    else {
        QVBoxLayout* vbox = new QVBoxLayout();
        vbox->addLayout(hBoxColor, 1);
        vbox->addStretch();
        return vbox;
    }
}

void SpatialTransFunc1DKeysEditor::createWidgets() {
    QWidget* mapping = new QWidget();
    QWidget* color = new QWidget();

    QLayout* mappingLayout = createMappingLayout();
    QLayout* colorLayout = createColorLayout();
    QLayout* buttonLayout = createButtonLayout();

    QSplitter* splitter = new QSplitter(orientation_);
    QLayout* buttonColor;
    if (orientation_ == Qt::Vertical) {
        buttonColor = new QVBoxLayout();
        //buttonColor->addItem(buttonLayout);
        buttonColor->addItem(mappingLayout);
        mapping->setLayout(buttonColor);
        color->setLayout(colorLayout);
    }
    else {
        buttonColor = new QHBoxLayout();
        buttonColor->addItem(buttonLayout);
        buttonColor->addItem(colorLayout);
        mapping->setLayout(mappingLayout);
        color->setLayout(buttonColor);
    }
    splitter->setChildrenCollapsible(true);
    splitter->addWidget(mapping);
    //splitter->addWidget(color);

    splitter->setStretchFactor(0, QSizePolicy::Expanding); // mapping should be stretched
    splitter->setStretchFactor(1, QSizePolicy::Preferred); // color should not be stretched

    QHBoxLayout* mainLayout = new QHBoxLayout();
    mainLayout->setMargin(4);
    mainLayout->addWidget(splitter);

    setLayout(mainLayout);
}

void SpatialTransFunc1DKeysEditor::createConnections() {
    // Buttons
    connect(clearButton_, SIGNAL(clicked()), this, SLOT(clearButtonClicked()));
    connect(loadButton_, SIGNAL(clicked()), this, SLOT(loadTransferFunction()));
    connect(saveButton_, SIGNAL(clicked()), this, SLOT(saveTransferFunction()));

    // signals from transferMappingCanvas
    connect(transCanvas_, SIGNAL(changed()), this, SLOT(updateTransferFunction()));
    connect(transCanvas_, SIGNAL(loadTransferFunction()), this, SLOT(loadTransferFunction()));
    connect(transCanvas_, SIGNAL(saveTransferFunction()), this, SLOT(saveTransferFunction()));
    connect(transCanvas_, SIGNAL(resetTransferFunction()), this, SLOT(clearButtonClicked()));
    connect(transCanvas_, SIGNAL(toggleInteractionMode(bool)), this, SLOT(toggleInteractionMode(bool)));
    connect(transCanvas_, SIGNAL(propertiesChanged()), this, SLOT(updateProperty()));

    // signals for colorPicker
    connect(transCanvas_, SIGNAL(colorChanged(const QColor&)),
            colorPicker_, SLOT(setCol(const QColor)));
    connect(transCanvas_, SIGNAL(colorChanged(const QColor&)),
            colorLumPicker_, SLOT(setCol(const QColor)));
    connect(colorPicker_, SIGNAL(newCol(int,int)),
            colorLumPicker_, SLOT(setCol(int,int)));
    connect(colorLumPicker_, SIGNAL(newHsv(int,int,int)),
            this, SLOT(markerColorChanged(int,int,int)));
    connect(colorPicker_, SIGNAL(toggleInteractionMode(bool)), this, SLOT(toggleInteractionMode(bool)));
    connect(colorLumPicker_, SIGNAL(toggleInteractionMode(bool)), this, SLOT(toggleInteractionMode(bool)));

    // doubleslider
    connect(doubleSlider_, SIGNAL(valuesChanged(float, float)), this, SLOT(thresholdChanged(float, float)));
    connect(doubleSlider_, SIGNAL(toggleInteractionMode(bool)), this, SLOT(toggleInteractionMode(bool)));

    // threshold spinboxes
    connect(lowerThresholdSpin_, SIGNAL(valueChanged(double)), this, SLOT(lowerThresholdSpinChanged(double)));
    connect(upperThresholdSpin_, SIGNAL(valueChanged(double)), this, SLOT(upperThresholdSpinChanged(double)));

    connect(lowerMappingSpin_, SIGNAL(valueChanged(double)), this, SLOT(lowerMappingChanged(double)));
    connect(upperMappingSpin_, SIGNAL(valueChanged(double)), this, SLOT(upperMappingChanged(double)));
    connect(fitDomainToData_, SIGNAL(clicked()), this, SLOT(fitDomainToData()));
    connect(alwaysFit_, SIGNAL(stateChanged(int)), this, SLOT(alwaysFitChanged(int)));
}

//----------------------------------------------------------------------------------------------
//      small functions
//----------------------------------------------------------------------------------------------
void SpatialTransFunc1DKeysEditor::causeVolumeRenderingRepaint() {
    // this informs the owner about change in transfer function texture
    property_->notifyChange();
    repaintAll();
    emit transferFunctionChanged();
}

void SpatialTransFunc1DKeysEditor::repaintAll() {
    transCanvas_->update();
    doubleSlider_->update();
    textureCanvas_->update();
}

void SpatialTransFunc1DKeysEditor::clearButtonClicked() {
    resetTransferFunction();
    causeVolumeRenderingRepaint();
}

void SpatialTransFunc1DKeysEditor::resetTransferFunction() {
    if (!transferFuncIntensity_) {
        LWARNING("No valid transfer function assigned");
        return;
    }

    //transferFuncIntensity_->setToStandardFunc();
    updateProperty();
    //fitDomainToData();
    causeVolumeRenderingRepaint();
}

void SpatialTransFunc1DKeysEditor::fitDomainToData() {
    property_->fitDomainToData();

    if(volume_) {
        updateMappingSpin(true);
        updateThresholdSpin(false);
        updateTransferFunction();
    }
}

void SpatialTransFunc1DKeysEditor::alwaysFitChanged(int state) {
    if(state == Qt::Checked) {
        property_->setAlwaysFitToDomain(true);
        fitDomainToData();
    }
    else
        property_->setAlwaysFitToDomain(false);
}

void SpatialTransFunc1DKeysEditor::setTransFuncProp(SpatialTransFuncProperty* prop) {

    SpatialTransFuncEditor::setTransFuncProp(prop);

    // update widgets
    transferFuncIntensity_ = dynamic_cast<TransFunc1DKeys*>(prop->get());
    texturePainter_->setTransFunc(transferFuncIntensity_);
    transCanvas_->setTransFunc(transferFuncIntensity_);
    updateFromProperty();
}

const SpatialTransFuncProperty* SpatialTransFunc1DKeysEditor::getTransFuncProp() const {
    return property_;
}

void SpatialTransFunc1DKeysEditor::updateTransferFunction() {

    if (!transferFuncIntensity_)
        return;

    if (property_ && transCanvas_) {
        property_->setParamMagnitude(transCanvas_->getParamMagnitude());
    }

    transferFuncIntensity_->invalidateTexture();
    property_->notifyChange();
    emit transferFunctionChanged();
}

void SpatialTransFunc1DKeysEditor::updateProperty() {
    property_->setParamMagnitude(transCanvas_->getParamMagnitude());
    property_->setWindowMode((int)transCanvas_->getWindowMode());
    property_->setKeysLocked(transCanvas_->getKeysLocked());
}

void SpatialTransFunc1DKeysEditor::markerColorChanged(int h, int s, int v) {
    transCanvas_->changeCurrentColor(QColor::fromHsv(h, s, v));
}

//----------------------------------------------------------------------------------------------
//      load and save
//----------------------------------------------------------------------------------------------
void SpatialTransFunc1DKeysEditor::saveTransferFunction() {

    if (!transferFuncIntensity_) {
        LWARNING("No valid transfer function assigned");
        return;
    }

    QStringList filter;
    for (size_t i = 0; i < transferFuncIntensity_->getSaveFileFormats().size(); ++i) {
        std::string temp = "transfer function (*." + transferFuncIntensity_->getSaveFileFormats()[i] + ")";
        filter << temp.c_str();
    }

    QString fileName = getSaveFileName(filter);
    if (!fileName.isEmpty()) {
        //save transfer function to disk
        if (!transferFuncIntensity_->save(fileName.toStdString())) {
            QMessageBox::critical(this, tr("Error"),
                                  tr("The transfer function could not be saved."));
            LERROR("The transfer function could not be saved. Maybe the disk is full?");
        }
    }
}

void SpatialTransFunc1DKeysEditor::loadTransferFunction() {

    if (!transferFuncIntensity_) {
        LWARNING("No valid transfer function assigned");
        return;
    }

    //create filter with supported file formats
    QString filter = "transfer function (";
    for (size_t i = 0; i < transferFuncIntensity_->getLoadFileFormats().size(); ++i) {
        std::string temp = "*." + transferFuncIntensity_->getLoadFileFormats()[i] + " ";
        filter.append(temp.c_str());
    }
    filter.replace(filter.length()-1, 1, ")");

    QString fileName = getOpenFileName(filter);
    if (!fileName.isEmpty()) {
        if (transferFuncIntensity_->load(fileName.toStdString())) {
            updateMappingSpin(true);
            updateThresholdSpin(true);
            updateTransferFunction();
        }
        else {
            QMessageBox::critical(this, tr("Error"),
                "The selected transfer function could not be loaded.");
            LERROR("The selected transfer function could not be loaded. Maybe the file is corrupt.");
        }
    }
}

//----------------------------------------------------------------------------------------------
//      update mapping
//----------------------------------------------------------------------------------------------
void SpatialTransFunc1DKeysEditor::updateMappingSpin(bool fromTF){
   if (!transferFuncIntensity_)
       return;

   lowerMappingSpin_->blockSignals(true);
   upperMappingSpin_->blockSignals(true);

   if(fromTF){
        tgt::vec2 domain = transferFuncIntensity_->getDomain();
        lowerMappingSpin_->setValue(domain.x);
        upperMappingSpin_->setValue(domain.y);
   } else
        transferFuncIntensity_->setDomain(static_cast<float>(lowerMappingSpin_->value()),static_cast<float>(upperMappingSpin_->value()),0);

   transCanvas_->domainChanged();

   double min = lowerMappingSpin_->value();
   double max = upperMappingSpin_->value();
   double diff = max - min;

   //set decimals
   if(abs(min) < 1.0)
       lowerMappingSpin_->setDecimals(maxDigits_-1);
   else
       lowerMappingSpin_->setDecimals(maxDigits_-(static_cast<int>( log10( abs( min ) ) ) + 1));
   if(abs(max) < 1.0)
       upperMappingSpin_->setDecimals(maxDigits_-1);
   else
       upperMappingSpin_->setDecimals(maxDigits_-(static_cast<int>( log10( abs( max ) ) ) + 1));

   //set stepsize
   lowerMappingSpin_->setSingleStep(diff/1000.0);
   upperMappingSpin_->setSingleStep(diff/1000.0);

   lowerMappingSpin_->blockSignals(false);
   upperMappingSpin_->blockSignals(false);
}

void SpatialTransFunc1DKeysEditor::mappingChanged() {
    if (!transferFuncIntensity_)
        return;

    updateMappingSpin(false);
    updateThresholdSpin(false);

    updateTransferFunction();
    checkDomainVersusData();
}

void SpatialTransFunc1DKeysEditor::lowerMappingChanged(double value) {
    if (!transferFuncIntensity_)
        return;

    //increment value of lower mapping spin when it equals value of upper mapping spin
    if (value >= upperMappingSpin_->value()) {
        upperMappingSpin_->blockSignals(true);
        upperMappingSpin_->setValue(value+0.01);
        upperMappingSpin_->blockSignals(false);
    }
    mappingChanged();
}

 void SpatialTransFunc1DKeysEditor::upperMappingChanged(double value) {
    if (!transferFuncIntensity_)
        return;

    //increment value of upper mapping spin when it equals value of lower mapping spin
    if (value <= lowerMappingSpin_->value()) {
        lowerMappingSpin_->blockSignals(true);
        lowerMappingSpin_->setValue(value-0.01);
        lowerMappingSpin_->blockSignals(false);
    }
    mappingChanged();
}

//----------------------------------------------------------------------------------------------
//      update threshold
//----------------------------------------------------------------------------------------------
void SpatialTransFunc1DKeysEditor::updateThresholdSpin(bool fromTF){
    if (!transferFuncIntensity_)
        return;

   lowerThresholdSpin_->blockSignals(true);
   upperThresholdSpin_->blockSignals(true);

    //set Range
   lowerThresholdSpin_->setRange(lowerMappingSpin_->value(), upperMappingSpin_->value());
   upperThresholdSpin_->setRange(lowerMappingSpin_->value(), upperMappingSpin_->value());

   double min, max;

   if(fromTF){
       min = transferFuncIntensity_->normalizedToRealWorld(transferFuncIntensity_->getThresholds().x);
       max = transferFuncIntensity_->normalizedToRealWorld(transferFuncIntensity_->getThresholds().y);
   }
   else{
       min = lowerMappingSpin_->value();
       max = upperMappingSpin_->value();
       transferFuncIntensity_->setThresholds(tgt::vec2(transferFuncIntensity_->realWorldToNormalized(min),transferFuncIntensity_->realWorldToNormalized(max)));
   }

   double diff = max - min;
   lowerThresholdSpin_->setValue(min);
   upperThresholdSpin_->setValue(max);
   transCanvas_->setThreshold(transferFuncIntensity_->realWorldToNormalized(min),transferFuncIntensity_->realWorldToNormalized(max));

   //set decimals
   if(abs(min) < 1.0)
       lowerThresholdSpin_->setDecimals(maxDigits_-1);
   else
       lowerThresholdSpin_->setDecimals(maxDigits_-(static_cast<int>( log10( abs( min ) ) ) + 1));
   if(abs(max) < 1.0)
       upperThresholdSpin_->setDecimals(maxDigits_-1);
   else
       upperThresholdSpin_->setDecimals(maxDigits_-(static_cast<int>( log10( abs( max ) ) ) + 1));

   //set stepsize
   lowerThresholdSpin_->setSingleStep(diff/1000.0);
   upperThresholdSpin_->setSingleStep(diff/1000.0);

   lowerThresholdSpin_->blockSignals(false);
   upperThresholdSpin_->blockSignals(false);

   doubleSlider_->blockSignals(true);
   doubleSlider_->setValues(transferFuncIntensity_->realWorldToNormalized(min),transferFuncIntensity_->realWorldToNormalized(max));
   doubleSlider_->blockSignals(false);
}


void SpatialTransFunc1DKeysEditor::thresholdChanged(float min, float max) {
    //sync with spinboxes
    if(transferFuncIntensity_) {
        upperThresholdSpin_->setValue(transferFuncIntensity_->normalizedToRealWorld(max));
        lowerThresholdSpin_->setValue(transferFuncIntensity_->normalizedToRealWorld(min));
    }

    //apply threshold to transfer function
    applyThreshold();
}

void SpatialTransFunc1DKeysEditor::lowerThresholdSpinChanged(double value) {
    //increment value of upper spin when it equals value of lower spin
    if (value > upperThresholdSpin_->value()) {
        upperThresholdSpin_->blockSignals(true);
        upperThresholdSpin_->setValue(value+0.01);//TODO: calculate diff
        upperThresholdSpin_->blockSignals(false);
    }

    //update doubleSlider to new minValue
    doubleSlider_->blockSignals(true);
    if(transferFuncIntensity_)
        doubleSlider_->setMinValue(transferFuncIntensity_->realWorldToNormalized(value));
    doubleSlider_->blockSignals(false);

    //apply threshold to transfer function
    applyThreshold();
}

void SpatialTransFunc1DKeysEditor::upperThresholdSpinChanged(double value) {
    if (!transferFuncIntensity_)
        return;

    //increment value of lower spin when it equals value of upper spin
    if (value < lowerThresholdSpin_->value()) {
        lowerThresholdSpin_->blockSignals(true);
        lowerThresholdSpin_->setValue(value-0.01);//TODO: calculate diff
        lowerThresholdSpin_->blockSignals(false);
    }

    //update doubleSlider to new maxValue
    doubleSlider_->blockSignals(true);
    if(transferFuncIntensity_)
        doubleSlider_->setMaxValue(transferFuncIntensity_->realWorldToNormalized(value));
    doubleSlider_->blockSignals(false);

    //apply threshold to transfer function
    applyThreshold();
}

void SpatialTransFunc1DKeysEditor::applyThreshold() {
    if (!transferFuncIntensity_)
        return;

    float min = doubleSlider_->getMinValue();
    float max = doubleSlider_->getMaxValue();
    transCanvas_->setThreshold(min, max);
    transferFuncIntensity_->setThresholds(min, max);

    updateTransferFunction();
}

//----------------------------------------------------------------------------------------------
//      other functions
//----------------------------------------------------------------------------------------------
void SpatialTransFunc1DKeysEditor::checkDomainVersusData() {
    bool warnLower = false;
    bool warnUpper = false;
    if (transferFuncIntensity_ && volume_ && volume_->getRepresentation<VolumeRAM>()) {
        //calculate Min/Max values:
        float min = volume_->getDerivedData<VolumeMinMax>()->getMinNormalized();
        float max = volume_->getDerivedData<VolumeMinMax>()->getMaxNormalized();

        RealWorldMapping rwm = volume_->getRealWorldMapping();
        min = rwm.normalizedToRealWorld(min);
        max = rwm.normalizedToRealWorld(max);

        tgt::vec2 domain = transferFuncIntensity_->getDomain();
        float avg = (domain.x + domain.y) / 2.0f;

        if(domain.x > min)
            warnLower = true;
        if(domain.y < max)
            warnUpper = true;

        if(min > avg)
            warnLower = true;
        if(max < avg)
            warnUpper = true;
    }


    QPalette lowerPal(lowerMappingSpin_->palette());
    if(warnLower)
        lowerPal.setColor(QPalette::Base, Qt::yellow);
    else
        lowerPal.setColor(QPalette::Base, QApplication::palette().color(QPalette::Base));
    lowerMappingSpin_->setPalette(lowerPal);


    QPalette upperPal(upperMappingSpin_->palette());
    if(warnUpper)
        upperPal.setColor(QPalette::Base, Qt::yellow);
    else
        upperPal.setColor(QPalette::Base, QApplication::palette().color(QPalette::Base));
    upperMappingSpin_->setPalette(upperPal);

}


void SpatialTransFunc1DKeysEditor::updateFromProperty() {
    tgtAssert(property_, "No property");

    // check whether new transfer function object has been assigned
    if (property_->get() != transferFuncIntensity_) {
        transferFuncIntensity_ = dynamic_cast<TransFunc1DKeys*>(property_->get());
        // propagate transfer function to mapping canvas and texture painter
        texturePainter_->setTransFunc(transferFuncIntensity_);
        transCanvas_->setTransFunc(transferFuncIntensity_);

        updateMappingSpin(true);
        updateThresholdSpin(true);

        if (property_->get() && !transferFuncIntensity_) {
            if (isEnabled()) {
                LWARNING("Current transfer function not supported by this editor. Disabling.");
                setEnabled(false);
            }
        }
    }

    // check whether the volume associated with the SpatialTransFuncProperty has changed
    const VolumeBase* newHandle = property_->getVolumeHandle();
    if (newHandle != volume_) {
        volume_ = newHandle;
        volumeChanged();
    }

    newHandle = property_->getParamHandle();
    if (newHandle != paramHandle_) {
        paramHandle_ = newHandle;
        paramChanged();
    }

    alwaysFit_->blockSignals(true);
    if(property_->getAlwaysFitToDomain())
        alwaysFit_->setCheckState(Qt::Checked);
    else
        alwaysFit_->setCheckState(Qt::Unchecked);
    alwaysFit_->blockSignals(false);

    if (transferFuncIntensity_) {
        setEnabled(true);

        if(setTFValues_){
        // update treshold widgets from tf
            updateMappingSpin(true);
            updateThresholdSpin(true);
            setTFValues_ = false;
        }
        // repaint control elements
        repaintAll();
    }
    else {
        setEnabled(false);
    }

    if ((int)transCanvas_->getWindowMode() != property_->getWindowMode())
        transCanvas_->setWindowMode(SpatialTransFuncMappingCanvas::WindowMode(property_->getWindowMode()));

    if (transCanvas_->getParamMagnitude() != property_->getParamMagnitude())
        transCanvas_->setParamMagnitude(property_->getParamMagnitude());

    if (transCanvas_->getKeysLocked() != property_->getKeysLocked())
        transCanvas_->setKeysLocked(property_->getKeysLocked());
}

void SpatialTransFunc1DKeysEditor::volumeChanged() {
    if (volume_ && volume_->getRepresentation<VolumeRAM>()) {

        //calculate Min/Max values:
        float min = volume_->getDerivedData<VolumeMinMax>()->getMinNormalized();
        float max = volume_->getDerivedData<VolumeMinMax>()->getMaxNormalized();

        RealWorldMapping rwm = volume_->getRealWorldMapping();
        min = rwm.normalizedToRealWorld(min);
        max = rwm.normalizedToRealWorld(max);
        //std::string unit = rwm.getUnit();

        lowerData_->setText(QString::number(min));
        upperData_->setText(QString::number(max));

        /*if(unit == "")
            dataLabel_->setText("Data Bounds");
        else
            dataLabel_->setText(QString("Data Bounds [") + QString::fromStdString(unit) + "]");*/

        if(property_->getAlwaysFitToDomain() || (((rwm.getOffset() != 0.0f) || (rwm.getScale() != 1.0f) ||
          (rwm.getUnit() != "")) && *transferFuncIntensity_ == TransFunc1DKeys())) {
            fitDomainToData();
        }
        setTFValues_ = true;
        // propagate new volume to transfuncMappingCanvas
        transCanvas_->volumeChanged(volume_);
    } else {
        transCanvas_->volumeChanged(0);
    }

    checkDomainVersusData();
}

void SpatialTransFunc1DKeysEditor::paramChanged() {
    transCanvas_->paramChanged(paramHandle_);
}

void SpatialTransFunc1DKeysEditor::resetEditor() {
    if (property_->get() != transferFuncIntensity_) {
        LDEBUG("The pointers of property and transfer function do not match."
                << "Creating new transfer function object.....");
        transferFuncIntensity_ = new TransFunc1DKeys(1024);//TODO
        property_->set(transferFuncIntensity_);

        // propagate transfer function to mapping canvas and texture painter
        texturePainter_->setTransFunc(transferFuncIntensity_);
        transCanvas_->setTransFunc(transferFuncIntensity_);
    }

    // reset transfer function and thresholds
    resetTransferFunction();

    causeVolumeRenderingRepaint();
}











} // namespace voreen
