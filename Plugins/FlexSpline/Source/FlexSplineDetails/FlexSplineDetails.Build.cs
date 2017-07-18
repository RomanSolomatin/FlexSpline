/*****************************************************************************
* Copyright (C) 2017 Oliver Hawk - All Rights Reserved
*
* @Author       Oliver Hawk
* @EMail        *************************
* @Package      Flex Spline
******************************************************************************/

using UnrealBuildTool;

public class FlexSplineDetails : ModuleRules
{
    public FlexSplineDetails(ReadOnlyTargetRules Target) : base(Target)
    {
        
        PublicIncludePaths.AddRange(new string[] {
            "FlexSplineDetails/Public"
        });


        PrivateIncludePaths.AddRange( new string[] {
            "FlexSplineDetails/Private"
            , "FlexSplineDetails/Private/DetailsCustomization"
        });
            
        
        PublicDependencyModuleNames.AddRange( new string[] {
            "Core"
        });
            
        
        PrivateDependencyModuleNames.AddRange(new string[] {
            "CoreUObject"
            , "Engine"
            , "InputCore"
            , "UnrealEd"

            , "FlexSpline"

            , "DesktopWidgets"
            , "Slate"
            , "SlateCore"
            , "EditorWidgets"
            , "PropertyEditor"
            , "ComponentVisualizers"
            , "EditorStyle"
        });


        PrivateIncludePathModuleNames.AddRange(new string[] {
        });

        DynamicallyLoadedModuleNames.AddRange(new string[] {
        });
    }
}
