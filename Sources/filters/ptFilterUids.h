/*******************************************************************************
**
** Photivo
**
** Copyright (C) 2012 Bernd Schoeler <brjohn@brother-john.net>
**
** This file is part of Photivo.
**
** Photivo is free software: you can redistribute it and/or modify
** it under the terms of the GNU General Public License version 3
** as published by the Free Software Foundation.
**
** Photivo is distributed in the hope that it will be useful,
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
** GNU General Public License for more details.
**
** You should have received a copy of the GNU General Public License
** along with Photivo.  If not, see <http://www.gnu.org/licenses/>.
**
*******************************************************************************/

#ifndef PTFILTERUIDS_H
#define PTFILTERUIDS_H

#include <QString>

// Fuid is short for “Filter Unique Id” and is a version 4 random UUID.
// Generate new ones from http://www.famkruithof.net/uuid/uuidgen
// Choose "version 4: random".
// Variable names should be "FilterName_ProcessingTabName".
namespace Fuid {
  const QString Input_Camera                    = "f2235922-9003-48d9-ae6d-5fdedd3d56c1"; // not yet ported
  const QString ColorSpace_Camera               = "fa250868-e988-4497-b5a2-de2fce17f606"; // not yet ported
  const QString GenericCorr_Camera              = "ce1ef13a-0d3c-42af-974f-78afdecb6d4f"; // not yet ported
  const QString WhieBalance_Camera              = "fc0e86eb-2df9-4ae7-96a4-2044fc5bd075"; // not yet ported
  const QString Demosaic_Camera                 = "2b1a43e7-dc4d-4bf9-ab51-7f7370af1cc0"; // not yet ported
  const QString HighlightRecovery_Camera        = "6395bbe5-7341-41fe-9f2d-465ba145daea"; // not yet ported

  const QString LensParametersLF_Geometry       = "bc69aec7-9ca2-46ec-a966-4193f2cdb1ac"; // not yet ported
  const QString ChromaAberrLF_Geometry          = "5daa9abe-5bed-4cdf-9550-4bfde7214b1b"; // not yet ported
  const QString VignettingLF_Geometry           = "3b44dc8a-978f-49b8-bf7f-712bfcff6471"; // not yet ported
  const QString LensDistortionLF_Geometry       = "cf3e85c0-48af-4cf8-838c-7dcd7ed4150f"; // not yet ported
  const QString GeomConversionLF_Geometry       = "d8b2941e-e6e7-4d9f-a527-bb6772a5d205"; // not yet ported
  const QString Defish_Geometry                 = "511011d1-44de-4768-978f-aae3a433473c"; // not yet ported
  const QString RotatePerspective_Geometry      = "397a72e5-eae7-4dc8-86f3-e797c99f3728"; // not yet ported
  const QString Crop_Geometry                   = "e487f445-d664-4bcc-812c-70d63659255b"; // not yet ported
  const QString LiquidRescale_Geometry          = "35ab24f9-c887-4ef2-a47b-356d20848b39"; // not yet ported
  const QString Resize_Geometry                 = "bebfe4b2-dab4-4ad3-9097-172d200c1e69"; // not yet ported
  const QString Flip_Geometry                   = "00aafc5b-4320-4e24-92a3-e275267f5408"; // not yet ported
  const QString Block_Geometry                  = "9b0f46d7-1b6f-4d95-adde-8b1a7dffde4f"; // not yet ported

  const QString ChannelMixer_RGB                = "ad7b76f2-7ddb-48b6-8978-e08f86d5537f"; // not yet ported
  const QString Highlights_RGB                  = "8daccb81-5dd3-4664-bf3c-1fefe7ad084e";
  const QString ColorIntensity_RGB              = "e36d034c-5acf-4c22-b7f3-a74180710bc6";
  const QString Brightness_RGB                  = "4b3e046f-2525-4b2d-86f2-63345669c82a";
  const QString Exposure_RGB                    = "096829a9-1793-4572-8d99-600b0b6bccfb"; // not yet ported
  const QString ReinhardBrighten_RGB            = "29fa3d1d-da2a-45f5-bb32-6003e745c334";
  const QString GammaTool_RGB                   = "a63b08df-36ee-4678-9e25-9c53467c502b";
  const QString Normalization_RGB               = "c27628bb-57ac-45e4-98fb-f120a072e85e";
  const QString ColorEnhancement_RGB            = "32040001-8373-40fe-a821-d3260a78d14a";
  const QString LMHRecovery_RGB                 = "c6f239bb-155d-4f28-b08c-2eb1363f25dd";
  const QString TextureContrast_RGB             = "aa7502ea-dba1-4800-a17e-e2d0d950f5c1"; // not yet ported
  const QString LocalContrast1_RGB              = "8f047599-9ca4-4d97-b919-420e20d03330"; // not yet ported
  const QString LocalContrast2_RGB              = "e67567c5-67c3-4044-ae2e-44c62fb98cdb"; // not yet ported
  const QString SigContrastRgb_RGB              = "25b0402c-1668-4800-aab8-028160a08087";
  const QString Levels_RGB                      = "f0f5f38c-4367-4550-bb3c-6dcb962a01a7";
  const QString RgbCurve_RGB                    = "786384c5-ded7-4669-b748-e40ebabcdf2c";

  const QString LabTransform_LabCC              = "ecb7f8f1-ed5a-4b4c-8da9-37e127818635";
  const QString ShadowsHighlights_LabCC         = "946311d6-64fb-46b8-a70e-bc9e8dcba56d";
  const QString LMHRecovery_LabCC               = "b025be47-6393-4557-936b-5d13d9c8edae";
  const QString Drc_LabCC                       = "df10c486-e22e-4825-b205-a1d2a8a08ee3";
  const QString TextureCurve_LabCC              = "02ffc2a8-1fc6-4f50-940a-924da85efa1e";
  const QString TextureContrast1_LabCC          = "2ddc61fb-8cc2-4388-93f1-81bf6727c4a1"; // not yet ported
  const QString TextureContrast2_LabCC          = "b2da3444-90f1-4cf2-95a7-a79a8a2c91e8"; // not yet ported
  const QString LocalContrast1_LabCC            = "049ec21a-0127-4978-ab64-5c343a1043c2"; // not yet ported
  const QString LocalContrast2_LabCC            = "35613734-22b1-4849-a9a9-864c6de39b68"; // not yet ported
  const QString LocalContrastStretch1_LabCC     = "d470b2aa-e475-4524-a695-09e8aaf4e588"; // not yet ported
  const QString LocalContrastStretch2_LabCC     = "31667a6f-8b3d-40a4-b4f4-8df0313fa211"; // not yet ported
  const QString SigContrastLab_LabCC            = "2f4b0f7a-d720-41e0-b2fc-20dff3a02fcf";
  const QString Saturation_LabCC                = "9db332ed-d421-41c9-9074-af291f992af2";
  const QString ColorBoost_LabCC                = "aae92669-4159-4e2c-8006-f92c9a7e710b";
  const QString Levels_LabCC                    = "fdc6ec47-610c-4c0e-bff1-d8f2469c7bc9";

  const QString ImpulseNR_LabSN                 = "bdc1b8bc-33f2-45fa-ae55-75d019113e6f"; // not yet ported
  const QString EAWavelets_LabSN                = "588bea41-c4d9-4fbe-8a3d-90dae3dab4dc"; // not yet ported
  const QString GreyCStoration_LabSN            = "fb32308b-b515-4e08-843f-c3cac80362c2"; // not yet ported
  const QString Defringe_LabSN                  = "3f798f38-991e-4164-96f3-bf9b5759e7ec"; // not yet ported
  const QString WaveletDenoise_LabSN            = "60ddc4aa-d02d-4e6a-9930-45f57f2ab6bb"; // not yet ported
  const QString LumaDenoise_LabSN               = "14e7b580-4e1e-4ab5-ba42-b2e37291f6fc"; // not yet ported
  const QString LumaDenoiseCurve_LabSN          = "e0310479-993f-4018-a80a-ac2d8c6c1ee4";
  const QString LumaDenoiseCurve2_LabSN         = "530aab8e-65e4-4bb8-b391-46f6673ac53d";
  const QString PyramidDenoise_LabSN            = "82bc20c5-3fac-4f54-a7f0-fc0a705b9dcf"; // not yet ported
  const QString ColorDenoise_LabSN              = "eccbe7c7-a702-498d-aa84-4312ad1ec6ab"; // not yet ported
  const QString DetailCurve_LabSN               = "bcc7f2e6-9db7-427f-a3d9-61db0cd73b29";
  const QString GradientSharpen_LabSN           = "b2a5a99d-83dd-49a1-97aa-6601c2ea2a6b"; // not yet ported
  const QString Wiener_LabSN                    = "f2d800db-1d7a-42bc-970c-9a89597f34a4";
  const QString InvDiffSharpen_LabSN            = "9f892b58-fa4e-428d-be91-bf9f0ef3e623"; // not yet ported
  const QString Usm_LabSN                       = "1d4f6f00-e2d8-47e6-9400-1986b3590ef3"; // not yet ported
  const QString HighpassSharpen_LabSN           = "38285120-2e8c-432f-80e3-3dfa14a36630"; // not yet ported
  const QString FilmGrain_LabSN                 = "6e54affb-3c1b-4080-b514-d9b190e36fa6"; // not yet ported
  const QString ViewLab_LabSN                   = "c5d5f54d-e22f-45d4-9a4a-6426f4a9a428"; // not yet ported

  const QString Outline_LabEyeCandy             = "1d3e2630-de72-46b8-b1f9-7169cd1b3832";
  const QString LumaByHueCurve_LabEyeCandy      = "1f5f1b76-71b7-4dda-9db3-89ffce1c7981";
  const QString SatCurve_LabEyeCandy            = "0a462e7c-5764-4ed5-bb28-e583aabbf292";
  const QString HueCurve_LabEyeCandy            = "960ada58-c073-4603-acad-f4894bb3fe91";
  const QString LCurve_LabEyeCandy              = "2fea9112-1a10-4466-8ef9-0918a09ea3d6";
  const QString ABCurves_LabEyeCandy            = "ee8167f2-1c78-4ae3-843d-4a3a0b807db8";
  const QString ColorContrast_LabEyeCandy       = "6dc120c5-e8ec-430e-9c5e-40ba217559d5";
  const QString ToneAdjust1_LabEyeCandy         = "f532185f-281a-4503-b96b-37c05bbcabf3";
  const QString ToneAdjust2_LabEyeCandy         = "25489dc0-8d77-4f39-bc42-1b91a0d1054d";
  const QString LumaAdjust_LabEyeCandy          = "61b61fd7-c891-4d78-92c2-3290c190be65";
  const QString SatAdjust_LabEyeCandy           = "fb3f0bbc-6255-43ec-947c-4bd43e115379";
  const QString Tone_LabEyeCandy                = "57477687-7954-4dd6-8442-e4fe52539e61";
  const QString Vignette_LabEyeCandy            = "eb2830e7-3782-4b1a-9e14-4fb3b8ad102d"; // not yet ported

  const QString BlackWhite_EyeCandy             = "fa677364-4c8c-41bd-9397-94ea42bec595"; // not yet ported
  const QString SimpleTone_EyeCandy             = "3ca8f518-398d-44f4-8e17-6303a0abcdd0"; // not yet ported
  const QString Tone1_EyeCandy                  = "d7e8ce45-5ccd-433f-aefd-00d226b2513e"; // not yet ported
  const QString Tone2_EyeCandy                  = "9ec16c48-5852-4791-ac6d-66ef268e01b7"; // not yet ported
  const QString CrossProcessing_EyeCandy        = "49258677-09d8-4035-9e01-15bdfef4cde5"; // not yet ported
  const QString SigContrastRgb_EyeCandy         = "6e7db630-310f-4f04-82e0-b668babfe8c9";
  const QString TextureOverlay1_EyeCandy        = "370dca54-85fd-4bf7-a3ff-3e5657e736c2"; // not yet ported
  const QString TextureOverlay2_EyeCandy        = "695cd103-9969-4331-b312-02fdeb361c9b"; // not yet ported
  const QString GradualOverlay1_EyeCandy        = "2dac85aa-7961-4fed-aa5d-6cde71535749"; // not yet ported
  const QString GradualOverlay2_EyeCandy        = "9b69181e-80ca-44bd-8ccf-a0bf57b0c11d"; // not yet ported
  const QString Vignette_EyeCandy               = "98128af2-9c76-4483-870c-89dce831daf1"; // not yet ported
  const QString GradualBlur1_EyeCandy           = "0a254db6-6300-403e-b4be-8ee6ca55bd7f"; // not yet ported
  const QString GradualBlur2_EyeCandy           = "2232911d-bac9-4eaa-a73f-d49594b07952"; // not yet ported
  const QString SoftglowOrten_EyeCandy          = "77ac5ce7-e1ab-4677-b6da-907c9f70e001"; // not yet ported
  const QString ColorIntensity_EyeCandy         = "2e756d6c-6a5d-45dd-873a-3690f8f3cbed";
  const QString RTone_EyeCandy                  = "6e3971ac-f640-4628-8aa0-ae07620f8ca4";
  const QString GTone_EyeCandy                  = "9e99b7ee-1e5f-4f4d-821f-efbfc0d6bffa";
  const QString BTone_EyeCandy                  = "44112e0a-81df-4ae5-aee8-18a17d0ac810";

  const QString RgbCurve_Out                    = "221fa750-226c-4a48-a94b-e8b516865aca";
  const QString GammaComp_Out                   = "86d02c06-5511-4653-be4a-aa45c47b57c3"; // not yet ported
  const QString ColorSpace_Out                  = "c18b7e50-3042-4554-b872-60f96eb26dd1"; // not yet ported
  const QString AfterGammaCurve_Out             = "88b24988-7fcf-47b6-b9a2-8600c992f3f3";
  const QString SigContrastRgb_Out              = "71999980-5530-4a91-996e-43e752099870";
  const QString Resize_Out                      = "e1736db8-50df-43f4-aefb-5903460b333d"; // not yet ported
  const QString Wiener_Out                      = "7c2a63e3-50be-476c-9364-7c442c2662fa";
  const QString OutputParams_Out                = "a3e4ce40-161d-4e13-91ae-9118ea31028d"; // not yet ported
}

#endif // PTFILTERUIDS_H
