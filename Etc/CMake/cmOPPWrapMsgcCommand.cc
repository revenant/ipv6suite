// $Header: /home/cvs/IPv6Suite/IPv6SuiteWithINET/Etc/CMake/Attic/cmOPPWrapMsgcCommand.cc,v 1.2 2005/02/10 05:59:32 andras Exp $
// Copyright (C) 2002 Johnny Lai
//
// This file is part of IPv6Suite
//
// IPv6Suite is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// IPv6Suite is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.


/**
 * @file   cmOPPWrapMsgcCommand.cc
 * @author Johnny Lai
 * @date   01 Jul 2003
 *
 * @brief  Custom command to convert .msg files to .cc and .h
 *
 * @note Assumes that opp_msgc outputs to the same directory as the source msgc
 * file.  This is different in behaviour from OPP_WRAP_NEDC (nedc -h) which
 * outputs to current directory.
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
  /**
   * List of Ned files that provide the source
   * generating _m.cc and _m.h
   */
  std::vector<std::string> m_WrapNedModule;
  std::string m_Target;
  std::string m_NedIncludeVar;
  const char* m_GenSourcesVar;
  //Parsed definition list into standard includes
  std::string m_includes;
  std::string m_MacroName;
  const char* m_NewNameSuffix;
  std::string m_ProgramName;
  const char* m_SourceExtn;
  const char* m_HeaderExtn;
  const char* m_DirSep;

} cmOPPWrapData;

extern "C" {

  static  const char* GetTerseDocumentation()
  {
    return "Create OMNeT++ Msgc Wrapper.";
  }

  /**
   * More documentation.
   */
  static  const char* GetFullDocumentation()
  {
     return
      "OPP_WRAP_MSGC(resultingLibraryName MsgIncludesDirDummyArg MsgSourceList)\n"
      "Produce .h and .cc files for all the .msg files listed "
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
    cdata->m_MacroName = "OPP_WRAP_MSGC";
    cdata->m_NewNameSuffix = "_m";
    cdata->m_ProgramName = "${OPP_MSGC}";
    cdata->m_SourceExtn = "cc";
    cdata->m_HeaderExtn = "h";
    cdata->m_DirSep = "/";
    cdata->m_GenSourcesVar = "GENERATED_MSGC_FILES";
    info->CAPI->SetClientData(info, cdata);

    if (argc < 3)
    {
      std::string help(cdata->m_MacroName + " called with incorrect number of arguments");
      info->CAPI->SetError(info, help.c_str());
      return false;
    }

    // Now check and see if the value has been stored in the cache
    // already, if so use that value and don't look for the program
    const char* OPP_WRAP_NEDC_value = info->CAPI->GetDefinition(mf, cdata->m_MacroName.c_str());
    if (OPP_WRAP_NEDC_value==0)
    {
      std::string help(cdata->m_MacroName + " Not defined anywhere : ");
      info->CAPI->SetError(info, help.c_str());
      return false;
    }

//TODO Don't know why this fails ?
//  if(cmSystemTools::IsOff(OPP_WRAP_NEDC_value))
//   if (!info->CAPI->IsOn(mf, OPP_WRAP_NEDC_value))
//   {
//     std::string help(cdata->m_MacroName + " called when off : ");
//     info->CAPI->SetError(info, help.c_str());
//     return false;
//   }

    // what is the current source dir
    std::string cdir = info->CAPI->GetCurrentDirectory(mf);

    // get parameter for the command
    cdata->m_Target              = argv[0];  // Target that will use the generated files
    cdata->m_NedIncludeVar       = argv[1];  // Name of list containing include dirs
//    cdata->m_NedSourceList       = argv[2];  // Source List of the NED source files


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

    char** newArgv = 0;
    //TODO should we delete newArgv or does system clean up after (most likely us)?

    int newArgc = 0;

    info->CAPI->ExpandSourceListArguments(mf, argc, const_cast<const char**>(argv), &newArgc, &newArgv, 2);

    // get the list of msg files from which sources and headers will be generated
    //msgc can only generate to same dir as where source msg file exists
    //std::string outputDirectory = cdir;

    std::string outputDirectory = info->CAPI->GetCurrentOutputDirectory(mf);


    const char * GENERATED_NEDC_FILES_value=
      info->CAPI->GetDefinition(mf,  cdata->m_GenSourcesVar);
    std::string nedsrcs("");
    if (GENERATED_NEDC_FILES_value!=0)
    {
      nedsrcs+=GENERATED_NEDC_FILES_value;
#ifdef DEBUG
      std::cout<<__FUNCTION__<< cdata->m_GenSourcesVar<<nedsrcs<<std::endl;
      std::cout<<__FUNCTION__<<"m_Target="<<cdata->m_Target<<std::endl;
#endif //DEBUG
    }

    for ( int i = 2; i < newArgc; i++)
    {
      //curr is sf in cmCPluginAPI notation
      void * curr = info->CAPI->GetSource(mf, newArgv[i]);
      // if we should use the source NED to generate .cc file
      if (!curr)
      {
        void* source_file = info->CAPI->CreateSourceFile();
        std::string srcName = newArgv[i];
        int k = 0;

/*
        //Preserve the relative path of .msg file required to tell cmake where
        //opp_msgc generates output
        if ((k = srcName.find_last_of(cdata->m_DirSep)) < srcName.size())
        {
          srcName = srcName.substr(0, k);
          srcName += cdata->m_DirSep;
          srcName += info->CAPI->GetFilenameWithoutExtension(newArgv[i]);
        }
        else
*/
        srcName = info->CAPI->GetFilenameWithoutExtension(newArgv[i]);

#ifdef DEBUG
        std::cout<<__FUNCTION__<<"srcName="<<srcName <<std::endl;
#endif //DEBUG

        const bool headerFileOnly = true;

        info->CAPI->SourceFileSetName2(source_file,
                                       (srcName + cdata->m_NewNameSuffix).c_str(),
                                       outputDirectory.c_str(),
                                       cdata->m_SourceExtn, !headerFileOnly);
        std::string origname = cdir + cdata->m_DirSep + newArgv[i];
        info->CAPI->SourceFileAddDepend(source_file, origname.c_str());
//        std::string headerName = cdir + cdata->m_DirSep +
        std::string headerName = outputDirectory + cdata->m_DirSep +
           std::string(info->CAPI->SourceFileGetSourceName(source_file)) +
          "." + std::string(cdata->m_HeaderExtn);
        info->CAPI->SourceFileAddDepend(source_file, headerName.c_str());
        cdata->m_GeneratedSourcesClasses.push_back(source_file);
        //Cannot set GENERATED source file property :()
        //std::cout<<info->CAPI->SourceFileGetProperty(source_file, "GENERATED")<<std::endl;
//        void  (*SourceFileSetProperty) (void *sf, const char *prop,
//                                  const char *value);

        nedsrcs=nedsrcs + ";" +
          std::string(info->CAPI->SourceFileGetSourceName(source_file)) + "." + std::string(cdata->m_SourceExtn);
//   + ";" +
//           std::string(info->CAPI->SourceFileGetSourceName(source_file)) + "." + std::string(cdata->m_HeaderExtn);

/*
        //Prevent double inclusion of _m.o if this was added to NEDC_GEN_SOURCES
        //which is included in ADD_LIBRARY
        nedsrcs=nedsrcs + ";" + cdir + cdata->m_DirSep +
           std::string(info->CAPI->SourceFileGetSourceName(source_file)) + "." + std::string(cdata->m_SourceExtn)
        //Prevent triple inclusion of _m.o
           + ";" + cdir + cdata->m_DirSep +
           std::string(info->CAPI->SourceFileGetSourceName(source_file)) + "." + std::string(cdata->m_HeaderExtn);
*/


#ifdef DEBUG
        std::cout<<__FUNCTION__<<"msgcgensrc="<<cdir + cdata->m_DirSep + std::string(info->CAPI->SourceFileGetSourceName(source_file)) + "." + std::string(cdata->m_SourceExtn)<<std::endl;
#endif //DEBUG

        cdata->m_WrapNedModule.push_back(origname);


      }
    }

    info->CAPI->AddDefinition(mf,  cdata->m_GenSourcesVar, nedsrcs.c_str());

#ifdef DEBUG
      std::cout<<__FUNCTION__<< cdata->m_GenSourcesVar<<nedsrcs<<std::endl;
#endif //DEBUG
    return true;




  }

  static void Fin(void *inf, void *mf)
  {
    cmLoadedCommandInfo *info = (cmLoadedCommandInfo *)inf;
    cmOPPWrapData* cdata = reinterpret_cast<cmOPPWrapData*> (info->CAPI->GetClientData(info));

    // first we add the rules for all the .msg to .h and .cc files
    size_t lastHeadersClass = cdata->m_GeneratedSourcesClasses.size();


    //std::string outputDirectory = info->CAPI->GetCurrentDirectory(mf);
    std::string outputDirectory = info->CAPI->GetCurrentOutputDirectory(mf);

    // Generate code for all the .ned files
    for(size_t classNum = 0; classNum < lastHeadersClass; classNum++)
    {
      //Give the explicit output path to be the source path as opp_msgc produces file there
      std::string cxxres = outputDirectory;
      cxxres += cdata->m_DirSep;
      cxxres += (
        std::string(info->CAPI->SourceFileGetSourceName(cdata->m_GeneratedSourcesClasses[classNum])) +
        ".");// + std::string(cdata->m_SourceExtn));

//    "." + cdata->m_GeneratedSourcesClasses[classNum].GetSourceExtension();
#ifdef DEBUG
      std::cout<<info->CAPI->SourceFileGetFullPath(cdata->m_GeneratedSourcesClasses[classNum])<<std::endl;
     std::cout<<__FUNCTION__<<":cxxres="<<cxxres<<std::endl;
#endif //DEBUG

      std::vector<std::string> cxxargs;

//No -I for msgc
/*
      //Need to add -Inedcincludes
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

*/

      cxxargs.push_back("-h"); //Generate output cc and h file in current dir

      cxxargs.push_back(cdata->m_WrapNedModule[classNum]);// name of the msg class file

      std::vector<std::string> depends;
      depends.push_back(cdata->m_ProgramName);

      std::vector<std::string> outputs;
      outputs.push_back( cxxres + cdata->m_SourceExtn);
      outputs.push_back( cxxres + cdata->m_HeaderExtn);

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
                                   cdata->m_ProgramName.c_str(), cxxargs.size(),
                                   const_cast<const char**>(cxxargsv), depends.size(),
                                   const_cast<const char**>(dependsv), outputs.size(),
                                   const_cast<const char**>(outputsv), cdata->m_Target.c_str());

      for (unsigned int i = 0; i < cxxargs.size(); i++)
        delete [] cxxargsv[i];
      delete [] cxxargsv;
      for (unsigned int i = 0; i < depends.size(); i++)
        delete [] dependsv[i];
      delete  [] dependsv;
      for (unsigned int i = 0; i < outputs.size(); i++)
        delete [] outputsv[i];
      delete [] outputsv;

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

  void CM_PLUGIN_EXPORT OPP_WRAP_MSGCInit(cmLoadedCommandInfo* info)
  {
    info->InitialPass = Init;
    info->FinalPass = Fin;
    info->Destructor = Dtor;
    info->GetTerseDocumentation = GetTerseDocumentation;
    info->GetFullDocumentation = GetFullDocumentation;
    info->m_Inherited = 1;
    info->Name = "OPP_WRAP_MSGC";

  }

}
