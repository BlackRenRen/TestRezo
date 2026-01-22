using UnrealBuildTool;
using System.Collections.Generic;

public class EOS_OSS_TutorialEditorTarget : TargetRules
{
    public EOS_OSS_TutorialEditorTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Editor;

        // UE5.7 default build settings
        DefaultBuildSettings = BuildSettingsVersion.V6;

        // Must stay SHARED for installed engine builds
        BuildEnvironment = TargetBuildEnvironment.Shared;

        // Recommended for UE5.7
        IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
    }
}
