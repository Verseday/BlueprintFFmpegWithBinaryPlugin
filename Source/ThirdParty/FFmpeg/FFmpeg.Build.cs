// Fill out your copyright notice in the Description page of Project Settings.

using System.Diagnostics;
using System.IO;
using UnrealBuildTool;

public class FFmpeg : ModuleRules
{
    public FFmpeg(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            // get FFmpeg directory path
            var FFmpegDirectoryPath = Path.Combine(ModuleDirectory, "FFmpegBinary");

            // get FFmpeg libavcodec directory path
            var FFmpegIncludeDirectoryPath = Path.Combine(FFmpegDirectoryPath, "bin", "include");

            // get FFmpeg lib directory path
            var FFmpegLibDirectoryPath = Path.Combine(FFmpegDirectoryPath, "bin");

            // get FFmpeg bin directory path
            var FFmpegBinDirectoryPath = Path.Combine(FFmpegDirectoryPath, "bin");

            PublicSystemIncludePaths.Add(FFmpegIncludeDirectoryPath);

            var FFmpegDllFilePath = Path.Combine(FFmpegBinDirectoryPath, "Windows", "*.dll");

            // Add the import library
            PublicAdditionalLibraries.Add(Path.Combine(FFmpegLibDirectoryPath, "Windows", "avcodec.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(FFmpegLibDirectoryPath, "Windows", "avformat.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(FFmpegLibDirectoryPath, "Windows", "swscale.lib"));
            PublicAdditionalLibraries.Add(Path.Combine(FFmpegLibDirectoryPath, "Windows", "avutil.lib"));

            // Delay-load the DLL, so we can load it from the right place first
            PublicDelayLoadDLLs.Add(Path.Combine(FFmpegDllFilePath));

            // Ensure that the DLLs are staged along with the executable
            RuntimeDependencies.Add("$(BinaryOutputDir)", FFmpegDllFilePath);
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            // get FFmpeg directory path
            var FFmpegDirectoryPath = Path.Combine(ModuleDirectory, "ffmpeg");

            // get FFmpeg include directory path
            var FFmpegIncludeDirectoryPath = Path.Combine(FFmpegDirectoryPath, "include");

            // get FFmpeg lib directory path
            var FFmpegLibDirectoryPath = Path.Combine(FFmpegDirectoryPath, "lib");

            // if at least one of the "include" or "lib" directory doesn't exist
            if (!Directory.Exists(FFmpegIncludeDirectoryPath) || !Directory.Exists(FFmpegLibDirectoryPath)) {
                // execute PreBuild.sh
                Process.Start(new ProcessStartInfo("bash") {
                    Arguments = Path.Combine(PluginDirectory, "PreBuild.sh")
                }).WaitForExit();
            }

           PublicSystemIncludePaths.Add(FFmpegIncludeDirectoryPath);

            var MacArchBinDirectoryPath = Path.Combine(FFmpegLibDirectoryPath);
            var LibAvcodecDylibPath = Path.Combine(MacArchBinDirectoryPath, "libavcodec.dylib");
            var LibFormatDylibPath = Path.Combine(MacArchBinDirectoryPath, "libavformat.dylib");
            var LibAvutilDylibPath = Path.Combine(MacArchBinDirectoryPath, "libavutil.dylib");
            var LibSwresampleDylibPath = Path.Combine(MacArchBinDirectoryPath, "libswresample.dylib");
            var LibSwscaleDylibPath = Path.Combine(MacArchBinDirectoryPath, "libswscale.dylib");

            // Delay-load the DLL, so we can load it from the right place first
            PublicDelayLoadDLLs.Add(LibAvcodecDylibPath);
            PublicDelayLoadDLLs.Add(LibFormatDylibPath);
            PublicDelayLoadDLLs.Add(LibAvutilDylibPath);
            PublicDelayLoadDLLs.Add(LibSwresampleDylibPath);
            PublicDelayLoadDLLs.Add(LibSwscaleDylibPath);

            // Ensure that the DLL is staged along with the executable
            RuntimeDependencies.Add(LibAvcodecDylibPath);
            RuntimeDependencies.Add(LibFormatDylibPath);
            RuntimeDependencies.Add(LibAvutilDylibPath);
            RuntimeDependencies.Add(LibSwresampleDylibPath);
            RuntimeDependencies.Add(LibSwscaleDylibPath);
        }
        // else if (Target.Platform == UnrealTargetPlatform.Android)
        // {
        //     // Add the import library
        //     PublicAdditionalLibraries.Add(Path.Combine(FFmpegBinDirectoryPath, "*.so"));
        // }
        // else if (Target.Platform == UnrealTargetPlatform.Linux)
        // {
        //     string LibFFmpegSoPath = Path.Combine(FFmpegBinDirectoryPath, "*.so");

        //     PublicAdditionalLibraries.Add(LibFFmpegSoPath);
        //     PublicDelayLoadDLLs.Add(LibFFmpegSoPath);
        //     RuntimeDependencies.Add(LibFFmpegSoPath);
        // }
    }
}
