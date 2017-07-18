/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

using UnrealBuildTool;

public class FlexSpline : ModuleRules
{
    public FlexSpline(ReadOnlyTargetRules Target) : base(Target)
    {
        
        PublicIncludePaths.AddRange(new string[] {
            "FlexSpline/Public"
        });


        PrivateIncludePaths.AddRange(new string[] {
            "FlexSpline/Private"
        });
            
        
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core"
        });
            
        
        PrivateDependencyModuleNames.AddRange(new string[] {
            "CoreUObject"
            , "Engine"
        });


        PrivateIncludePathModuleNames.AddRange(new string[] {
        });

        DynamicallyLoadedModuleNames.AddRange(new string[] {
        });
    }
}
