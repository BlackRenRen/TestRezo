using UnrealBuildTool;

public class EOS_OSS_TutorialServerTarget : TargetRules
{
    public EOS_OSS_TutorialServerTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Server;
        DefaultBuildSettings = BuildSettingsVersion.V5;
        IncludeOrderVersion  = EngineIncludeOrderVersion.Unreal5_6;
        CppStandard          = CppStandardVersion.Cpp20;

        ExtraModuleNames.Add("EOS_OSS_Tutorial");
    }
}
