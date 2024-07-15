/*******************************************************************************
 * Copyright IBM Corp. and others 2000
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] https://openjdk.org/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0
 *******************************************************************************/

#include "compile/Compilation.hpp"
#include "env/CompilerEnv.hpp"
#include "env/CPU.hpp"
#include "env/VMJ9.h"
#include "x/runtime/X86Runtime.hpp"
#include "env/JitConfig.hpp"
#include "codegen/CodeGenerator.hpp"
#if defined(J9VM_OPT_JITSERVER)
#include "control/CompilationRuntime.hpp"
#include "control/CompilationThread.hpp"
#include "runtime/JITClientSession.hpp"
#endif /* defined(J9VM_OPT_JITSERVER) */

// This is a workaround to avoid J9_PROJECT_SPECIFIC macros in x/env/OMRCPU.cpp
// Without this definition, we get an undefined symbol of JITConfig::instance() at runtime
TR::JitConfig * TR::JitConfig::instance() { return NULL; }

TR::CPU
J9::X86::CPU::detectRelocatable(OMRPortLibrary * const omrPortLib)
   {
   // For our portable processor description we allow only features present in the Sandybridge Architecture
   const uint32_t customFeatures [] = {OMR_FEATURE_X86_FPU, OMR_FEATURE_X86_CX8, OMR_FEATURE_X86_CMOV,
                                       OMR_FEATURE_X86_MMX, OMR_FEATURE_X86_SSE, OMR_FEATURE_X86_SSE2,
                                       OMR_FEATURE_X86_SSSE3, OMR_FEATURE_X86_SSE4_1, OMR_FEATURE_X86_POPCNT,
                                       OMR_FEATURE_X86_SSE3, OMR_FEATURE_X86_AESNI, OMR_FEATURE_X86_AVX
                                      };

   OMRPORT_ACCESS_FROM_OMRPORT(omrPortLib);
   OMRProcessorDesc customProcessorDescription;
   memset(customProcessorDescription.features, 0, OMRPORT_SYSINFO_FEATURES_SIZE*sizeof(uint32_t));
   for (size_t i = 0; i < sizeof(customFeatures)/sizeof(uint32_t); i++)
      {
      omrsysinfo_processor_set_feature(&customProcessorDescription, customFeatures[i], TRUE);
      }

   OMRProcessorDesc hostProcessorDescription;
   omrsysinfo_get_processor_description(&hostProcessorDescription);

   // Pick the older processor between our hand-picked processor and host processor to be the actual portable processor
   OMRProcessorDesc portableProcessorDescription;
   portableProcessorDescription.processor = OMR_PROCESSOR_X86_FIRST;
   portableProcessorDescription.physicalProcessor = portableProcessorDescription.processor;
   memset(portableProcessorDescription.features, 0, OMRPORT_SYSINFO_FEATURES_SIZE*sizeof(uint32_t));

   for (size_t i = 0; i < OMRPORT_SYSINFO_FEATURES_SIZE; i++)
      {
      portableProcessorDescription.features[i] = hostProcessorDescription.features[i] & customProcessorDescription.features[i];
      }

   return TR::CPU::customize(portableProcessorDescription);
   }

void
J9::X86::CPU::enableFeatureMasks()
   {
   // Only enable the features that compiler currently uses
   const uint32_t utilizedFeatures [] = {OMR_FEATURE_X86_FPU, OMR_FEATURE_X86_CX8, OMR_FEATURE_X86_CMOV,
                                        OMR_FEATURE_X86_MMX, OMR_FEATURE_X86_SSE, OMR_FEATURE_X86_SSE2,
                                        OMR_FEATURE_X86_SSSE3, OMR_FEATURE_X86_SSE4_1, OMR_FEATURE_X86_POPCNT,
                                        OMR_FEATURE_X86_AESNI, OMR_FEATURE_X86_OSXSAVE, OMR_FEATURE_X86_AVX,
                                        OMR_FEATURE_X86_FMA, OMR_FEATURE_X86_HLE, OMR_FEATURE_X86_RTM,
                                        OMR_FEATURE_X86_SSE3, OMR_FEATURE_X86_AVX2, OMR_FEATURE_X86_AVX512F,
                                        OMR_FEATURE_X86_AVX512VL, OMR_FEATURE_X86_AVX512BW, OMR_FEATURE_X86_AVX512DQ,
                                        OMR_FEATURE_X86_AVX512CD, OMR_FEATURE_X86_SSE4_2};

   memset(_supportedFeatureMasks.features, 0, OMRPORT_SYSINFO_FEATURES_SIZE*sizeof(uint32_t));
   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);
   for (size_t i = 0; i < sizeof(utilizedFeatures)/sizeof(uint32_t); i++)
      {
      omrsysinfo_processor_set_feature(&_supportedFeatureMasks, utilizedFeatures[i], TRUE);
      }
   _isSupportedFeatureMasksEnabled = true;
   }


TR_X86CPUIDBuffer *
J9::X86::CPU::queryX86TargetCPUID()
   {
   static TR_X86CPUIDBuffer buf = { {'U','n','k','n','o','w','n','B','r','a','n','d'} };
   jitGetCPUID(&buf);
   return &buf;
   }

const char *
J9::X86::CPU::getProcessorVendorId()
   {
   return self()->getX86ProcessorVendorId();
   }

uint32_t
J9::X86::CPU::getProcessorSignature()
   {
   return self()->getX86ProcessorSignature();
   }

bool
J9::X86::CPU::hasPopulationCountInstruction()
   {
   if ((self()->getX86ProcessorFeatureFlags2() & TR_POPCNT) != 0x00000000)
      return true;
   else
      return false;
   }

bool
J9::X86::CPU::isCompatible(const OMRProcessorDesc& processorDescription)
   {
   for (int i = 0; i < OMRPORT_SYSINFO_FEATURES_SIZE; i++)
      {
      // Check to see if the current processor contains all the features that code cache's processor has
      if ((processorDescription.features[i] & _processorDescription.features[i]) != processorDescription.features[i])
         return false;
      }
   return true;
   }

bool
J9::X86::CPU::is(OMRProcessorArchitecture p)
   {
   return _processorDescription.processor == p;
   }

bool
J9::X86::CPU::supportsFeature(uint32_t feature)
   {
   OMRPORT_ACCESS_FROM_OMRPORT(TR::Compiler->omrPortLib);

   return TRUE == omrsysinfo_processor_has_feature(&_processorDescription, feature);
   }

uint32_t
J9::X86::CPU::getX86ProcessorFeatureFlags()
   {
#if defined(J9VM_OPT_JITSERVER)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      auto *vmInfo = TR::compInfoPT->getClientData()->getOrCacheVMInfo(stream);
      return vmInfo->_processorDescription.features[0];
      }
#endif /* defined(J9VM_OPT_JITSERVER) */
   return self()->queryX86TargetCPUID()->_featureFlags;
   }

uint32_t
J9::X86::CPU::getX86ProcessorFeatureFlags2()
   {
#if defined(J9VM_OPT_JITSERVER)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      auto *vmInfo = TR::compInfoPT->getClientData()->getOrCacheVMInfo(stream);
      return vmInfo->_processorDescription.features[1];
      }
#endif /* defined(J9VM_OPT_JITSERVER) */
   return self()->queryX86TargetCPUID()->_featureFlags2;
   }

uint32_t
J9::X86::CPU::getX86ProcessorFeatureFlags8()
   {
#if defined(J9VM_OPT_JITSERVER)
   if (auto stream = TR::CompilationInfo::getStream())
      {
      auto *vmInfo = TR::compInfoPT->getClientData()->getOrCacheVMInfo(stream);
      return vmInfo->_processorDescription.features[3];
      }
#endif /* defined(J9VM_OPT_JITSERVER) */
   return self()->queryX86TargetCPUID()->_featureFlags8;
   }

bool
J9::X86::CPU::is_test(OMRProcessorArchitecture p)
   {
#if defined(J9VM_OPT_JITSERVER)
   if (TR::CompilationInfo::getStream())
      return true;
#endif /* defined(J9VM_OPT_JITSERVER) */
   if (TR::comp()->compileRelocatableCode() || TR::comp()->compilePortableCode())
      return true;

   return _processorDescription.processor == p;
   }
