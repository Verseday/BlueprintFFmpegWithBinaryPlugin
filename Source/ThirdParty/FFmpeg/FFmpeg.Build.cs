// Fill out your copyright notice in the Description page of Project Settings.

using System.IO;
using UnrealBuildTool;

public class FFmpeg : ModuleRules
{
    public FFmpeg(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.External;

        // get FFmpeg directory path
        var FFmpegDirectoryPath = Path.Combine(ModuleDirectory, "FFmpeg");

        // get FFmpeg libavcodec directory path
        var FFmpegIncludeDirectoryPath = Path.Combine(FFmpegDirectoryPath, "bin", "include");

        // get FFmpeg lib directory path
        var FFmpegLibDirectoryPath = Path.Combine(FFmpegDirectoryPath, "bin");

        // get FFmpeg bin directory path
        var FFmpegBinDirectoryPath = Path.Combine(FFmpegDirectoryPath, "bin");

        PublicSystemIncludePaths.Add(FFmpegIncludeDirectoryPath);

        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
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
            var MacArchBinDirectoryPath = Path.Combine(FFmpegBinDirectoryPath, "Mac");
            var LibAvcodecDylibPath = Path.Combine(MacArchBinDirectoryPath, "libavcodec.61.dylib");
            var LibFormatDylibPath = Path.Combine(MacArchBinDirectoryPath, "libavformat.61.dylib");
            var LibAvutilDylibPath = Path.Combine(MacArchBinDirectoryPath, "libavutil.59.dylib");
            var LibSwscaleDylibPath = Path.Combine(MacArchBinDirectoryPath, "libswscale.8.dylib");

            // Delay-load the DLL, so we can load it from the right place first
            PublicDelayLoadDLLs.Add(LibAvcodecDylibPath);
            PublicDelayLoadDLLs.Add(LibFormatDylibPath);
            PublicDelayLoadDLLs.Add(LibAvutilDylibPath);
            PublicDelayLoadDLLs.Add(LibSwscaleDylibPath);

            // Ensure that the DLL is staged along with the executable
            RuntimeDependencies.Add(LibAvcodecDylibPath);
            RuntimeDependencies.Add(LibFormatDylibPath);
            RuntimeDependencies.Add(LibAvutilDylibPath);
            RuntimeDependencies.Add(LibSwscaleDylibPath);
        }
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            // Add the import library
            PublicAdditionalLibraries.Add(Path.Combine(FFmpegBinDirectoryPath, "*.so"));
        }
        else if (Target.Platform == UnrealTargetPlatform.Linux)
        {
            string LibFFmpegSoPath = Path.Combine(FFmpegBinDirectoryPath, "*.so");

            PublicAdditionalLibraries.Add(LibFFmpegSoPath);
            PublicDelayLoadDLLs.Add(LibFFmpegSoPath);
            RuntimeDependencies.Add(LibFFmpegSoPath);
        }
    }
}
