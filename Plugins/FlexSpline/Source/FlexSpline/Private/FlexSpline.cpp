/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

#include "FlexSplinePrivatePCH.h"

#define LOCTEXT_NAMESPACE "FFlexSplineModule"

void FFlexSplineModule::StartupModule()
{
}

void FFlexSplineModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE
    
DEFINE_LOG_CATEGORY(FlexLog);
IMPLEMENT_MODULE(FFlexSplineModule, FlexSpline)