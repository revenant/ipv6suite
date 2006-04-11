// $Header: /home/cvs/IPv6Suite/IPv6Suite/Etc/CMake/cmOPPWrapNedcCommand.cc,v 1.6 2004/11/01 01:10:31 jlai Exp $
// Copyright (C) 2003 Johnny Lai
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   cmOPPWrapNedcCommand.cc
 * @author Johnny Lai
 * @date   26 Jun 2003
 * 

 * @brief Conversion from my corresponding cmake patch into a seperate CMake
 * plugin.
 *
 * Should save lots of time from having to rebuild the whole CMake.
 * 
 */


#include "cmCPluginAPI.h"

#include <vector>
#include <string>
#include <iostream>

typedef struct
{
  /**
   * List of produced files.
   */
  std::vector<void*> m_GeneratedSourcesClasses;
//  std::vector<cmSourceFile> m_GeneratedHeadersClasses;
  /**
   * List of Ned files that provide the source 
   * generating _n.cc 
   */
  std::vector<std::string> m_WrapNedModule;
  std::string m_Target;
  std::string m_NedIncludeVar;
  //Parsed definition list into standard includes 
  std::string m_includes;
  std::string m_MacroName;
  const char* m_NewNameSuffix;
  std::string m_ProgramName;
  ///When true this plugin will do nothing.
  bool dynamicNed;
} cmOPPWrapData;

extern "C" {

  static  const char* GetTerseDocumentation() 
  {
    return "Create OMNeT++ Nedc Wrapper.";
  }
  
  /**
   * More documentation.
   */
  static  const char* GetFullDocumentation()
  {
    return
      "OPP_WRAP_NEDC(resultingLibraryName NedIncludeDirsList SourceList)\n"
      "Produce .h and .cc files for all the .ned files listed "
      "in the SourceList.\n"
      "The .h files will be added to the library using the base name in\n"
      "source list.\n"
      "The .cc files will be added to the library using the base name in \n"
      "source list.";
  }

  static int Init(void *inf, void *mf, int argc, char* argv[])
  {
    cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
    cmOPPWrapData* cdata = new cmOPPWrapData;
    cdata->m_MacroName = "OPP_WRAP_NEDC";
    cdata->m_NewNameSuffix = "_n";
    cdata->m_ProgramName = "${OPP_NEDC}";
    cdata->dynamicNed = false;

    info->CAPI->SetClientData(info,cdata);

    if (argc < 3)
    {
      std::string help(cdata->m_MacroName + " called with incorrect number of arguments");
      info->CAPI->SetError(info, help.c_str());
      return false;
    }
  
    // Now check and see if the value has been stored in the cache
    // already, if so use that value and don't look for the program
    if (!info->CAPI->IsOn(mf, cdata->m_MacroName.c_str()))
    {
      std::string help(cdata->m_MacroName + " option is off check your omnetpp installation");
      info->CAPI->SetError(info, help.c_str());
      return false;
    }

    if (info->CAPI->IsOn(mf, "DYNAMIC_NED"))
    {
      cdata->dynamicNed = true;
      return true;
      //If we've run before with DYNAMIC_NED false will get unable to find _n.cc
      //files because there is no equivalent of mf->removeSource so we can only
      //add. This means dynamic ned on only for new installations (otherwise
      //leave it off) and if its on it'll stay on
    }
  
    // what is the current source dir
    std::string cdir = info->CAPI->GetCurrentDirectory(mf);

    // get parameter for the command
    cdata->m_Target              = argv[0];  // Target that will use the generated files
    cdata->m_NedIncludeVar       = argv[1];  // Name of list containing include dirs


    const char* NedcIncludeVar_value = info->CAPI->GetDefinition(mf, cdata->m_NedIncludeVar.c_str());
    if (NedcIncludeVar_value != 0)
      cdata->m_includes = NedcIncludeVar_value;
  
  
    if (cdata->m_includes.empty())
    {
      std::string message = cdata->m_MacroName + " called with empty NedIncludeVar list " + cdata->m_NedIncludeVar;
      info->CAPI->SetError(info, message.c_str());
      //return false;
    }
    else
    {
      for (size_t j = 0;;)
      {

        if ((j = cdata->m_includes.find(";")) < cdata->m_includes.size())
          cdata->m_includes.replace(j, 1, " -I");
        else
          break;
      }
  
      cdata->m_includes.insert(0, "-I", 2);

    }

    //std::vector<std::string> newArgs;
//    const unsigned int ARGSIZE = 1024;
    char** newArgv = 0;
//TODO should we delete newArgv or does system clean up after (most likely us)?
//    char** newArgv = new char*[argc];
//     for ( int i = 0; i < argc; i++)
//     {
//       newArgv[i] = new char[ARGSIZE];
//     }

    int newArgc = 0;

    // m_Makefile->ExpandSourceListArguments(args,newArgs, 2);
    info->CAPI->ExpandSourceListArguments(mf, argc, const_cast<const char**>(argv), &newArgc, &newArgv, 2);

    // get the list of NED files from which .cxx will be generated 
    std::string outputDirectory = info->CAPI->GetCurrentOutputDirectory(mf);

  
    const char * GENERATED_NEDC_FILES_value=
      info->CAPI->GetDefinition(mf, "GENERATED_NEDC_FILES"); 
    std::string nedsrcs("");
    if (GENERATED_NEDC_FILES_value!=0)
    {
      nedsrcs=nedsrcs+GENERATED_NEDC_FILES_value;
    }
    // for(std::vector<std::string>::iterator i = (newArgs.begin() + 2); 
//       i != newArgs.end(); i++)
    for ( int i = 2; i < newArgc; i++)
    {
      //curr is sf in cmCPluginAPI notation
      void * curr = info->CAPI->GetSource(mf, newArgv[i]);
      // if we should use the source NED to generate .cc file
      if (!curr)
      {
        void* source_file = info->CAPI->CreateSourceFile();
        std::string srcName = info->CAPI->GetFilenameWithoutExtension(newArgv[i]);

        const bool headerFileOnly = true;

//       source_file.SetName((srcName+m_NewNameSuffix).c_str(), 
//                   outputDirectory.c_str(), "cc",!headerFileOnly);
        info->CAPI->SourceFileSetName2(source_file,
                                       (srcName + cdata->m_NewNameSuffix).c_str(), 
                                       outputDirectory.c_str(), "cc",!headerFileOnly);
        std::string origname = cdir + "/" + newArgv[i];
        std::string cxxname = info->CAPI->SourceFileGetFullPath(source_file);
        cdata->m_WrapNedModule.push_back(origname);
        // add starting depends
        //source_file.GetDepends().push_back(origname);
        info->CAPI->SourceFileAddDepend(source_file, origname.c_str());
        cdata->m_GeneratedSourcesClasses.push_back(source_file);
        nedsrcs=nedsrcs + ";" + std::string(info->CAPI->SourceFileGetSourceName(source_file)) + ".cc";     
      }
      else if (cdata->dynamicNed)
      {
        //This only destroys the curr pointer and does not remove the file
        info->CAPI->DestroySourceFile(curr);
        nedsrcs="";
      }
    }
  
    info->CAPI->AddDefinition(mf, "GENERATED_NEDC_FILES", nedsrcs.c_str());
  
    return true;



  }

  static void Fin(void *inf, void *mf)
  {
    cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
    cmOPPWrapData* cdata = reinterpret_cast<cmOPPWrapData*> (info->CAPI->GetClientData(info));

    if (cdata->dynamicNed)
      return;

    // first we add the rules for all the .ned to .h and .cc files
    size_t lastHeadersClass = cdata->m_GeneratedSourcesClasses.size();


    std::string outputDirectory = info->CAPI->GetCurrentOutputDirectory(mf);

    // Generate code for all the .ned files
    for(size_t classNum = 0; classNum < lastHeadersClass; classNum++)
    {

      std::string cxxres = outputDirectory;
      cxxres += "/";
      cxxres += info->CAPI->SourceFileGetSourceName(cdata->m_GeneratedSourcesClasses[classNum]) + 
        std::string(".cc");
//    "." + cdata->m_GeneratedSourcesClasses[classNum].GetSourceExtension();

      std::vector<std::string> cxxargs;

      //Need to add -Inedcindcluds
      if (!cdata->m_includes.empty())
      {
     
        for (size_t j = 0, k = 0;;)       
          if ((k = cdata->m_includes.find_first_of(' ', j)) < cdata->m_includes.size())
          {
            //Don't want to include the actual ' ' character
            cxxargs.push_back(cdata->m_includes.substr(j, k-j));
            j = k+1;
          }
          else
          {
            cxxargs.push_back(cdata->m_includes.substr(j, cdata->m_includes.size()-j));
            break;
          }
      }
    
      cxxargs.push_back("-h"); //Generate output cc file in current dir
      cxxargs.push_back(cdata->m_WrapNedModule[classNum]);// name of the Ned module file

      std::vector<std::string> depends;
      depends.push_back(cdata->m_ProgramName);
    
      std::vector<std::string> outputs;
      outputs.push_back( cxxres );

      std::string nedcFlags = info->CAPI->GetDefinition(mf, "OPP_NEDC_FLAGS")?
        info->CAPI->GetDefinition(mf, "OPP_NEDC_FLAGS"):
        "-Wno-unused";

      // Add command for generating the .cc files
//     m_Makefile->AddCustomCommand(m_WrapNedModule[classNum].c_str(),
//                                  m_ProgramName.c_str(), cxxargs, depends, 
//                                  outputs, m_Target.c_str() );
      char** cxxargsv = new char*[cxxargs.size()];  
      for (unsigned int i = 0; i < cxxargs.size(); i++)
      {
        cxxargsv[i] = new char[cxxargs[i].size()+1];
        memcpy(cxxargsv[i], cxxargs[i].c_str(), cxxargs[i].size());
        cxxargsv[i][cxxargs[i].size()] = 0;
      }
      char** dependsv = new char*[depends.size()];
      for (unsigned int i = 0; i < depends.size(); i++)
      {
        dependsv[i] = new char[depends[i].size()+1];
        memcpy(dependsv[i], depends[i].c_str(), depends[i].size());
        dependsv[i][depends[i].size()] = 0;
      }
      char** outputsv = new char*[outputs.size()];
      for (unsigned int i = 0; i < outputs.size(); i++)
      {
        outputsv[i] = new char[outputs[i].size()+1];
        memcpy(outputsv[i], outputs[i].c_str(), outputs[i].size());
        outputsv[i][outputs[i].size()] = 0;
      }
      
      info->CAPI->AddCustomCommand(mf, cdata->m_WrapNedModule[classNum].c_str(), 
//      info->CAPI->AddCustomCommandToTarget(mf, cdata->m_Target.c_str(), 
                                   cdata->m_ProgramName.c_str(), cxxargs.size(), 
                                   const_cast<const char**>(cxxargsv), depends.size(),
                                   const_cast<const char**>(dependsv), outputs.size(),
                                   const_cast<const char**>(outputsv), cdata->m_Target.c_str());
//                                   const_cast<const char**>(cxxargsv), CM_PRE_BUILD);
    
      for (unsigned int i = 0; i < cxxargs.size(); i++)
        delete [] cxxargsv[i];
      delete [] cxxargsv;
      for (unsigned int i = 0; i < depends.size(); i++)
        delete [] dependsv[i];
      delete  [] dependsv;
      for (unsigned int i = 0; i < outputs.size(); i++)
        delete [] outputsv[i];
      delete [] outputsv;

      //Works without adding to target now because we modified the cmakelists
      //file to include NEDC_GEN_SOURCES in ADD_LIBRARY

      //TODO what's with these target bits not equiv Plugin API calls perhaps
      //these were incorrect in 1st place
      //cmTarget* target = &m_Makefile->GetTargets()[m_Target];
      // if (target->GetType() == cmTarget::EXECUTABLE || target->GetType() == cmTarget::WIN32_EXECUTABLE)
//       continue;

      //Add to library targets only otherwise will get multiply defined symbols
      // cmSourceFile* sf = m_Makefile->AddSource(m_GeneratedSourcesClasses[classNum]);
      void* sf = info->CAPI->AddSource(mf, cdata->m_GeneratedSourcesClasses[classNum]);
//     target->GetSourceFiles().push_back( sf );
     
    
    }

  }

  static void Dtor(void *inf)
  {
    cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
    cmOPPWrapData* cdata = static_cast<cmOPPWrapData*> (info->CAPI->GetClientData(info));
    delete cdata;
  }

  void CM_PLUGIN_EXPORT OPP_WRAP_NEDCInit(cmLoadedCommandInfo* info)
  {
    info->InitialPass = Init;
    info->FinalPass = Fin;
    info->Destructor = Dtor;
    info->GetTerseDocumentation = GetTerseDocumentation;
    info->GetFullDocumentation = GetFullDocumentation;
    info->m_Inherited = 1;
    info->Name = "OPP_WRAP_NEDC";
  
  }
}
