/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

#pragma once

#include "IDetailCustomization.h"

/**
 * Adds details to Flex Spline Actor when selecting one or multiple spline points
 */
class FFlexSplineDetails : public IDetailCustomization
{
public:

    /** Makes a new instance of this detail layout class for a specific detail view requesting it */
    static TSharedRef<IDetailCustomization> MakeInstance();

    /** IDetailCustomization interface */
    void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
