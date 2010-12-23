////////////////////////////////////////////////////////////////////////////////
//
// photivo
//
// Copyright (C) 2008 Jos De Laender <jos.de_laender@telenet.be>
// Copyright (C) 2009,2010 Michael Munzert <mail@mm-log.com>
//
// This file is part of photivo.
//
// photivo is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 3
// as published by the Free Software Foundation.
//
// photivo is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with photivo.  If not, see <http://www.gnu.org/licenses/>.
//
////////////////////////////////////////////////////////////////////////////////

#include <QtCore>

#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#include "ptCurve.h"
#include "ptConstants.h"

double DeltaGammaBT709(double r, double, double) {
  return InverseGammaSRGB(GammaBT709(r,0,0),0,0);
};

int main () {
  // Delta between sRGB and BT709
  ptCurve* DeltaGammaBT709Curve = new ptCurve();
  DeltaGammaBT709Curve->SetCurveFromFunction(DeltaGammaBT709,0,0);
  DeltaGammaBT709Curve->m_Type = ptCurveType_Full;
  DeltaGammaBT709Curve->m_IntendedChannel = ptCurveChannel_RGB;
  char* Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This curve applies Inverted(Gamma sRGB) * Gamma BT709\n"
    "; In effect it replaces thus the correct, standard sRGB curve by\n"
    "; the BT709 Gamma, which is sometimes visually more attractive.\n"
    ";\n"
    "; (C)2008 - Jos De Laender\n"
    ";\n";
  DeltaGammaBT709Curve->WriteCurve("Curves/DeltaGammaBT709.ptc",Header);

  // Normal Astia.
  ptCurve* AstiaCurve = new ptCurve();
  AstiaCurve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/photivoAnchors/Digital_Fuji_Astia.anchors");
  AstiaCurve->m_Type = ptCurveType_Anchor;
  AstiaCurve->m_IntendedChannel = ptCurveChannel_RGB;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is a portrait type curve. Aims to produce low contrast images\n"
    "; and give accurate smooth looking skins tones, eliminating the\n"
    "; interference of harsh shadows.\n"
    ";\n";
  AstiaCurve->WriteCurve("Curves/Astia.ptc",Header);

  // Reala
  ptCurve* RealaCurve = new ptCurve();
  RealaCurve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/photivoAnchors/Digital_Fuji_Superia_Reala.anchors");
  RealaCurve->m_Type = ptCurveType_Anchor;
  RealaCurve->m_IntendedChannel = ptCurveChannel_RGB;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; Popular custom curve which improves underexposure and improves\n"
    "; highlight detail. Colour rendition is brighter and more prominent\n"
    "; whilst maintaining the default level of contrast.\n"
    ";\n";
  RealaCurve->WriteCurve("Curves/Reala.ptc",Header);

  // Optima
  ptCurve* OptimaCurve = new ptCurve();
  OptimaCurve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/photivoAnchors/Digital_Agfa_Optima.anchors");
  OptimaCurve->m_Type = ptCurveType_Anchor;
  OptimaCurve->m_IntendedChannel = ptCurveChannel_RGB;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; Contrasty curve by nature, more geared for ptatter lighting types\n"
    "; and will enhance colour rendition, contrast whilst optimising \n"
    "; dynamic range. Good to use if you want bold images straight out \n"
    "; of the camera.\n"
    ";\n";
  OptimaCurve->WriteCurve("Curves/Optima.ptc",Header);

  // PointAndShootV41
  ptCurve* PointAndShootV41Curve = new ptCurve();
  PointAndShootV41Curve->ReadAnchors(
  "ReferenceMaterial/SourceOfCurves/photivoAnchors/fotogenetic_point_and_shoot_v41.anchors");
  PointAndShootV41Curve->m_Type = ptCurveType_Anchor;
  PointAndShootV41Curve->m_IntendedChannel = ptCurveChannel_RGB;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is a good replacement curve for photographers who constantly\n"
    "; find themselves having to use EV compensation to correct for dark \n"
    "; images. It is calibrated to add the equivalent of +0.5 EV to images \n"
    "; while holding more highlight information than if you were to actually\n"
    "; set the camera to +0.5 EV. This curve is recommended for most users \n"
    "; as a general use curve."
    ";\n";
  PointAndShootV41Curve->WriteCurve("Curves/PointAndShootV41.ptc",Header);

  // EV3V42
  ptCurve* EV3V42Curve = new ptCurve();
  EV3V42Curve->ReadAnchors(
  "ReferenceMaterial/SourceOfCurves/photivoAnchors/fotogenetic_ev3_v42.anchors");
  EV3V42Curve->m_Type = ptCurveType_Anchor;
  EV3V42Curve->m_IntendedChannel = ptCurveChannel_RGB;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is a good replacement curve for photographers who constantly\n"
    "; find themselves having to use EV compensation to correct for dark \n"
    "; images. It is calibrated to add the equivalent of +0.33 EV to images \n"
    "; while holding more highlight information than if you were to actually\n"
    "; set the camera to +0.33 EV. This curve is recommended for most users \n"
    "; as a general use curve when point and shoot is too bright."
    ";\n";
  EV3V42Curve->WriteCurve("Curves/EV3V42.ptc",Header);

  // Some Sigmoidals.
  for (double Contrast=0.5; Contrast<10.0; Contrast+=0.5) {
    ptCurve* SigmoidalCurve = new ptCurve();
    SigmoidalCurve->SetCurveFromFunction(Sigmoidal,0.5,Contrast);
    SigmoidalCurve->m_Type = ptCurveType_Full;
    SigmoidalCurve->m_IntendedChannel = ptCurveChannel_RGB;
    char* Header = NULL;
    QString FileName = "Curves/Sigmoidal_";
    QString Tmp;
    Tmp.setNum((int)(10*Contrast));
    FileName += Tmp;
    FileName += ".ptc";
    SigmoidalCurve->WriteCurve(FileName.toAscii().data(),Header);
  }

  // GammaTool
  ptCurve* GammaToolCurve = new ptCurve();
  GammaToolCurve->SetCurveFromFunction(GammaTool,0.45,0.1);
  GammaToolCurve->m_Type = ptCurveType_Full;
  GammaToolCurve->m_IntendedChannel = ptCurveChannel_RGB;
  GammaToolCurve->WriteCurve("Curves/GammaTool.ptc",NULL);

  // DeltaGammaTool
  ptCurve* DeltaGammaToolCurve = new ptCurve();
  DeltaGammaToolCurve->SetCurveFromFunction(DeltaGammaTool,0.33,0.06);
  DeltaGammaToolCurve->m_Type = ptCurveType_Full;
  DeltaGammaToolCurve->m_IntendedChannel = ptCurveChannel_RGB;
  DeltaGammaToolCurve->WriteCurve("Curves/DeltaGamma(0.33,0.06).ptc",NULL);
  DeltaGammaToolCurve->SetCurveFromFunction(DeltaGammaTool,0.45,0.1);
  DeltaGammaToolCurve->m_Type = ptCurveType_Full;
  DeltaGammaToolCurve->m_IntendedChannel = ptCurveChannel_RGB;
  DeltaGammaToolCurve->WriteCurve("Curves/DeltaGamma(0.45,0.1).ptc",NULL);

  // Base Curves from darktable, see ReferenceFiles for CopyRight by darktable project
  ptCurve* Curve = new ptCurve();
  // dark contrast
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/DarkContrast.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve (dark contrast).\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/DarkContrast.ptc",Header);

  // Fotogenic V41
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/FotogenicV41.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve (Fotogenetic - Point and shoot v4.1).\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/FotogenicV41.ptc",Header);

  // Fotogenic V42
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/FotogenicV42.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve (Fotogenetic - EV3 v4.2).\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/FotogenicV42.ptc",Header);

  // Canon
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/CanonBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Canon cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Canon.ptc",Header);

  // Kodak
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/KodakBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Kodak Easyshare cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Kodak.ptc",Header);

  // Leica
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/PanasonicBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Leica cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Leica.ptc",Header);

  // Minolta
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/MinoltaBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Minolta cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Minolta.ptc",Header);

  // Nikon
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/NikonBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Nikon cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Nikon.ptc",Header);

  // Olympus
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/OlympusBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Olympus cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Olympus.ptc",Header);

  // Panasonic
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/PanasonicBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Panasonic cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Panasonic.ptc",Header);

  // Pentax
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/PentaxBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Pentax cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Pentax.ptc",Header);

  // Sony
  Curve->ReadAnchors(
    "ReferenceMaterial/SourceOfCurves/darktableAnchors/SonyBase.anchors");
  Curve->m_Type = ptCurveType_Anchor;
  Curve->m_IntendedChannel = ptCurveChannel_Base;
  Header = (char*)
    ";\n"
    "; photivo Curve File\n"
    ";\n"
    "; This is an experimental base curve for Sony cameras.\n"
    "; Original values are from the darktable project.\n"
    ";\n";
  Curve->WriteCurve("Curves/Base/Sony.ptc",Header);


}
