using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Text;
using System.Xml.Linq;

namespace MsiGenerator
{
    class Program
    {
        // Updated path to reflect WiX Toolset v3.14
        private static readonly string WiX_TOOLSET_PATH = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFilesX86), "WiX Toolset v3.14", "bin");
        private static readonly string OUTPUT_DIR = "Output";

        static void Main(string[] args)
        {
            Console.WriteLine("---------------------------------------------");
            Console.WriteLine("  MSI Installer Generator for Windows");
            Console.WriteLine("---------------------------------------------");
            Console.WriteLine();

            // Create output directory
            Directory.CreateDirectory(OUTPUT_DIR);

            // Get user input for files to be installed
            var filesToInstall = new List<string>();
            Console.WriteLine("Enter full paths of files to be installed (one per line).");
            Console.WriteLine("Press Enter on an empty line to finish.");
            string filePath;
            while (!string.IsNullOrWhiteSpace(filePath = Console.ReadLine()))
            {
                if (File.Exists(filePath))
                {
                    filesToInstall.Add(filePath);
                }
                else
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"Error: File not found at '{filePath}'. Please enter a valid path.");
                    Console.ResetColor();
                }
            }

            Console.WriteLine();

            // Get user input for .reg files
            var regFiles = new List<string>();
            Console.WriteLine("Enter full paths of registry (.reg) files to be installed (one per line).");
            Console.WriteLine("Press Enter on an empty line to finish.");
            string regPath;
            while (!string.IsNullOrWhiteSpace(regPath = Console.ReadLine()))
            {
                if (File.Exists(regPath))
                {
                    regFiles.Add(regPath);
                }
                else
                {
                    Console.ForegroundColor = ConsoleColor.Red;
                    Console.WriteLine($"Error: Registry file not found at '{regPath}'. Please enter a valid path.");
                    Console.ResetColor();
                }
            }

            Console.WriteLine();

            // Get user input for the registry key to check
            Console.WriteLine("Enter a registry key to check before installation (e.g., SOFTWARE\\MyApp).");
            Console.WriteLine("The installer will only proceed if this key exists. Leave blank to skip.");
            string registryCheckKey = Console.ReadLine();

            Console.WriteLine();

            // Generate the WiX XML
            Console.WriteLine("Generating WiX XML...");
            try
            {
                string wixSourcePath = Path.Combine(OUTPUT_DIR, "Product.wxs");
                string customActionDllPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "RegistryCheckCustomAction.dll");

                GenerateWixXml(wixSourcePath, filesToInstall, regFiles, registryCheckKey, customActionDllPath);

                Console.ForegroundColor = ConsoleColor.Green;
                Console.WriteLine($"Successfully generated WiX source file: {wixSourcePath}");
                Console.ResetColor();

                // Build the MSI
                Console.WriteLine();
                Console.WriteLine("Building the MSI...");
                BuildMsi(wixSourcePath, customActionDllPath);
            }
            catch (Exception ex)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                Console.WriteLine($"An error occurred: {ex.Message}");
                Console.ResetColor();
            }

            Console.WriteLine();
            Console.WriteLine("Press any key to exit.");
            Console.ReadKey();
        }

        /// <summary>
        /// Generates the WiX XML source file based on user input.
        /// </summary>
        /// <param name="outputPath">The path to save the generated .wxs file.</param>
        /// <param name="files">List of full file paths to be installed.</param>
        /// <param name="regFiles">List of full registry file paths to be installed.</param>
        /// <param name="registryCheckKey">The registry key to check as a prerequisite.</param>
        /// <param name="customActionDllPath">Path to the custom action DLL.</param>
        private static void GenerateWixXml(string outputPath, List<string> files, List<string> regFiles, string registryCheckKey, string customActionDllPath)
        {
            XNamespace wixNs = "http://schemas.microsoft.com/wix/2006/wi";

            // Generate the components and component group inside a single fragment
            var componentFragment = GenerateComponentsFragment(files, regFiles, wixNs);

            var document = new XDocument(
                new XElement(wixNs + "Wix",
                    new XElement(wixNs + "Product",
                        new XAttribute("Id", Guid.NewGuid().ToString("D")),
                        new XAttribute("Name", "Custom Application"),
                        new XAttribute("Language", "1033"),
                        new XAttribute("Version", "1.0.0.0"),
                        new XAttribute("Manufacturer", "My Company"),
                        new XAttribute("UpgradeCode", Guid.NewGuid().ToString("D")),
                        new XElement(wixNs + "Package",
                            new XAttribute("Id", "*"),
                            new XAttribute("InstallerVersion", "200"),
                            new XAttribute("Compressed", "yes"),
                            new XAttribute("InstallScope", "perMachine")
                        ),
                        new XElement(wixNs + "Media",
                            new XAttribute("Id", "1"),
                            new XAttribute("Cabinet", "media1.cab"),
                            new XAttribute("EmbedCab", "yes")
                        ),
                        new XElement(wixNs + "Directory",
                            new XAttribute("Id", "TARGETDIR"),
                            new XAttribute("Name", "SourceDir"),
                            new XElement(wixNs + "Directory",
                                new XAttribute("Id", "ProgramFilesFolder"),
                                new XElement(wixNs + "Directory",
                                    new XAttribute("Id", "INSTALLFOLDER"),
                                    new XAttribute("Name", "Custom App")
                                )
                            )
                        ),
                        new XElement(wixNs + "Feature",
                            new XAttribute("Id", "ProductFeature"),
                            new XAttribute("Title", "Custom App"),
                            new XAttribute("Level", "1"),
                            // This ComponentGroupRef is now inside the Feature element.
                            new XElement(wixNs + "ComponentGroupRef",
                                new XAttribute("Id", "ProductComponents")
                            )
                        )
                    ),
                    // These fragments are now direct children of the Wix element
                    componentFragment,
                    GenerateRegistryCheck(registryCheckKey, wixNs)
                )
            );

            // Save the XML to a file
            document.Save(outputPath);
        }

        /// <summary>
        /// Generates a WiX Fragment containing the ComponentGroup and DirectoryRef elements.
        /// </summary>
        private static XElement GenerateComponentsFragment(List<string> files, List<string> regFiles, XNamespace wixNs)
        {
            var componentGroup = new XElement(wixNs + "ComponentGroup", new XAttribute("Id", "ProductComponents"));
            var directoryRef = new XElement(wixNs + "DirectoryRef", new XAttribute("Id", "INSTALLFOLDER"));

            foreach (var file in files)
            {
                string fileId = $"File_{Path.GetFileName(file).Replace('.', '_')}";
                var component = new XElement(wixNs + "Component",
                    new XAttribute("Id", fileId),
                    new XAttribute("Guid", Guid.NewGuid().ToString("D")),
                    new XElement(wixNs + "File",
                        new XAttribute("Id", fileId),
                        new XAttribute("Source", file),
                        new XAttribute("KeyPath", "yes")
                    )
                );
                directoryRef.Add(component);
                componentGroup.Add(new XElement(wixNs + "ComponentRef", new XAttribute("Id", fileId)));
            }

            foreach (var regFile in regFiles)
            {
                string regFileId = $"RegFile_{Path.GetFileName(regFile).Replace('.', '_')}";
                var component = new XElement(wixNs + "Component",
                    new XAttribute("Id", regFileId),
                    new XAttribute("Guid", Guid.NewGuid().ToString("D")),
                    new XElement(wixNs + "RegistryFile",
                        new XAttribute("Id", regFileId),
                        new XAttribute("Source", regFile)
                    )
                );
                directoryRef.Add(component);
                componentGroup.Add(new XElement(wixNs + "ComponentRef", new XAttribute("Id", regFileId)));
            }

            return new XElement(wixNs + "Fragment",
                directoryRef,
                componentGroup
            );
        }

        /// <summary>
        /// Generates the custom action fragment for registry key check.
        /// </summary>
        private static XElement GenerateRegistryCheck(string registryCheckKey, XNamespace wixNs)
        {
            if (string.IsNullOrEmpty(registryCheckKey))
            {
                return null;
            }

            var customAction = new XElement(wixNs + "CustomAction",
                new XAttribute("Id", "CheckRegistryKey"),
                new XAttribute("BinaryKey", "RegistryCheckCustomAction"),
                new XAttribute("DllEntry", "CheckRegistryKey"),
                new XAttribute("Execute", "immediate"),
                new XAttribute("Return", "check")
            );

            var customActionData = new XElement(wixNs + "CustomAction",
                new XAttribute("Id", "CheckRegistryKeyData"),
                new XAttribute("Property", "CheckRegistryKey"),
                new XAttribute("Value", $"REGISTRYKEY={registryCheckKey}")
            );

            var customActionRef = new XElement(wixNs + "CustomActionRef", new XAttribute("Id", "CheckRegistryKey"));

            var installExecuteSequence = new XElement(wixNs + "InstallExecuteSequence",
                new XElement(wixNs + "Custom",
                    new XAttribute("Action", "CheckRegistryKey"),
                    new XAttribute("Before", "InstallInitialize"),
                    "NOT Installed"
                )
            );

            var binary = new XElement(wixNs + "Binary",
                new XAttribute("Id", "RegistryCheckCustomAction"),
                new XAttribute("SourceFile", "RegistryCheckCustomAction.dll")
            );

            // This is a simplified example. In a real-world scenario, you would
            // also need to handle the custom action's return code and display
            // a message if the check fails.

            return new XElement(
                wixNs + "Fragment",
                new XElement(wixNs + "Property",
                    new XAttribute("Id", "MYREGISTRYKEY"),
                    new XElement(wixNs + "RegistrySearch",
                        new XAttribute("Id", "MyRegistryKeySearch"),
                        new XAttribute("Root", "HKLM"),
                        new XAttribute("Key", registryCheckKey),
                        new XAttribute("Name", "ValueName"),
                        new XAttribute("Type", "raw")
                    )
                ),
                installExecuteSequence,
                customAction,
                customActionData,
                binary
            );
        }

        /// <summary>
        /// Executes the WiX command-line tools to build the MSI.
        /// </summary>
        private static void BuildMsi(string wixSourcePath, string customActionDllPath)
        {
            // Path to candle.exe and light.exe
            string candlePath = Path.Combine(WiX_TOOLSET_PATH, "candle.exe");
            string lightPath = Path.Combine(WiX_TOOLSET_PATH, "light.exe");

            // Extract the filename and directory for correct working directory setting
            string wixSourceFileName = Path.GetFileName(wixSourcePath);
            string wixObjectFileName = Path.ChangeExtension(wixSourceFileName, ".wixobj");
            string msiOutputFileName = Path.GetFileName(Path.Combine(OUTPUT_DIR, "MyInstaller.msi"));
            string workingDirectory = Path.GetDirectoryName(wixSourcePath);

            // Step 1: Compile the WiX source
            Console.WriteLine($"Compiling with: {candlePath} {wixSourceFileName}");
            var candleProcess = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = candlePath,
                    Arguments = wixSourceFileName,
                    WorkingDirectory = workingDirectory, // Corrected working directory
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true
                }
            };

            candleProcess.Start();
            candleProcess.WaitForExit();
            Console.WriteLine(candleProcess.StandardOutput.ReadToEnd());

            if (candleProcess.ExitCode != 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                throw new Exception("Candle failed to compile the WiX source.");
            }

            // Step 2: Link the WiX object to create the MSI
            Console.WriteLine($"Linking with: {lightPath} {wixObjectFileName} -out {msiOutputFileName}");
            var lightProcess = new Process
            {
                StartInfo = new ProcessStartInfo
                {
                    FileName = lightPath,
                    Arguments = $"{wixObjectFileName} -out {msiOutputFileName} -ext WixUIExtension",
                    WorkingDirectory = workingDirectory, // Corrected working directory
                    UseShellExecute = false,
                    RedirectStandardOutput = true,
                    CreateNoWindow = true
                }
            };

            lightProcess.Start();
            lightProcess.WaitForExit();
            Console.WriteLine(lightProcess.StandardOutput.ReadToEnd());

            if (lightProcess.ExitCode != 0)
            {
                Console.ForegroundColor = ConsoleColor.Red;
                throw new Exception("Light failed to create the MSI.");
            }

            Console.ForegroundColor = ConsoleColor.Green;
            Console.WriteLine($"Successfully created MSI: {Path.Combine(workingDirectory, msiOutputFileName)}");
            Console.ResetColor();
        }
    }
}
