using System.Collections.Generic;
using UnrealBuildTool;

public class EOS_OSS_Tutorial : ModuleRules
{
    public EOS_OSS_Tutorial(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "InputCore",
            "UMG",
            "OnlineSubsystem",
            "OnlineSubsystemUtils",
            "Sockets",
            "Networking",
            "EOSSDK"
        });

        DynamicallyLoadedModuleNames.AddRange(new string[] {
            "OnlineSubsystemEOS"
        });

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "OnlineSubsystem",
                "OnlineSubsystemUtils"
            }
        );

  
        PublicIncludePaths.AddRange(new string[] {
            "EOS_OSS_Tutorial"
        });
        PrivateIncludePaths.AddRange(new string[] {
            "EOS_OSS_Tutorial"
        });
    }
}
