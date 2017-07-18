/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

#pragma once

#include "ModuleManager.h"

class FFlexSplineDetailsModule : public IModuleInterface
{
public:

    /** IModuleInterface implementation */
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};


DECLARE_LOG_CATEGORY_EXTERN(FlexDetailsLog, Log, All);
