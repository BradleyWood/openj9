/*******************************************************************************
 * Copyright IBM Corp. and others 2025
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

#include "codegen/Register.hpp"
#include "codegen/RecognizedMethods.hpp"
#include "codegen/CodeGenerator.hpp"
#include "codegen/X86Instruction.hpp"
#include "il/Node.hpp"
#include "il/DataTypes.hpp"
#include "J9X86Intrinsics.hpp"
#include "jitprotos.h"
#include "immintrin.h"

/**
 * Proposed intrinsics model; Simply write a function and declare it as an intrinsic in J9X86Intrinsics.inc.
 * The goal is to centralize CG intrinsic implementations in a single place and simplify workflow.
 */

/** Outline intrinsic using intel intrinsics **/
/** No need to declare helper OMR **/
double J9FASTCALL J9::X86::doubleFMAImpl(J9VMThread* currentThread, double a, double b, double c)
   {
   return _mm_cvtsd_f64(_mm_fmadd_sd(_mm_set_sd(a), _mm_set_sd(b), _mm_set_sd(c)));
   }

float J9FASTCALL J9::X86::floatFMAImpl(J9VMThread* currentThread, float a, float b, float c)
   {
   return _mm_cvtss_f32(_mm_fmadd_ss(_mm_set_ss(a), _mm_set_ss(b), _mm_set_ss(c)));
   }

/** Codegen intrinsic example **/
TR::Register *J9::X86::inlineMathFMA(TR::Node *node, TR::CodeGenerator *cg)
   {
   printf("Inline math fma double=%d\n", node->getDataType().isDouble());
   TR::Register *resultReg = cg->allocateRegister(TR_FPR);
   resultReg->setIsSinglePrecision(!node->getDataType().isDouble());

   TR::Node *lhsNode = node->getFirstChild();
   TR::Node *midNode = node->getSecondChild();
   TR::Node *rhsNode = node->getThirdChild();

   TR::Register *lhsReg = cg->evaluate(lhsNode);
   TR::Register *midReg = cg->evaluate(midNode);
   TR::Register *rhsReg = cg->evaluate(rhsNode);

   TR::InstOpCode::Mnemonic fmaOpcode = node->getDataType().isDouble() ? TR::InstOpCode::VFMADD213SDRegRegReg : TR::InstOpCode::VFMADD213SSRegRegReg;
   TR::InstOpCode::Mnemonic fpMovRegRegOpcode = node->getDataType().isDouble() ? TR::InstOpCode::MOVSDRegReg : TR::InstOpCode::MOVSSRegReg;

   generateRegRegInstruction(fpMovRegRegOpcode, node, resultReg, lhsReg, cg);
   generateRegRegRegInstruction(fmaOpcode, node, resultReg, midReg, rhsReg, cg);

   cg->decReferenceCount(lhsNode);
   cg->decReferenceCount(midNode);
   cg->decReferenceCount(rhsNode);

   node->setRegister(resultReg);

   return resultReg;
   }

TR::Register *J9::X86::callCIntrinsic(TR::Node *node, TR::CodeGenerator *cg, TR::DataType dt, void *func)
   {
   printf("Inline callCIntrinsic double=%d\n", node->getDataType().isDouble());
   TR::MethodSymbol *methodSymbol = TR::MethodSymbol::create(cg->trHeapMemory(), TR_System);
   methodSymbol->setHelper();
   methodSymbol->setMethodAddress(func);

   TR::SymbolReference *helperSymRef = new (cg->trHeapMemory()) TR::SymbolReference(cg->comp()->getSymRefTab(), methodSymbol);
   helperSymRef->getSymbol()->getMethodSymbol()->setLinkage(TR_System);

   TR::Node::recreate(node, TR::ILOpCode::getDirectCall(dt));
   node->setSymbolReference(helperSymRef);

   return TR::TreeEvaluator::performCall(node, false, true, cg);
   }

bool J9::X86::isSupportedIntrinsic(TR::RecognizedMethod method, TR::Node *node, TR::CodeGenerator *cg)
   {
   switch (method)
      {
#define INTRINSIC(name, alias, ...) case alias: return true;       /* TODO; Test for platform support */
#define C_INTRINSIC(name, alias, dt, ...) case alias: return true; /* TODO; Test for platform support */
#include "J9X86Intrinsics.inc"
#undef INTRINSIC
#undef C_INTRINSIC
      default:
         return NULL;
      }
   }

TR::Register *J9::X86::dispatchIntrinsic(TR::RecognizedMethod method, TR::Node *node, TR::CodeGenerator *cg)
   {
   switch (method)
      {
#define INTRINSIC(name, alias, ...) case alias: return name(node, cg);
#define C_INTRINSIC(name, alias, dt, ...) case alias: return callCIntrinsic(node, cg, dt, reinterpret_cast<void *>(name));
#include "J9X86Intrinsics.inc"
#undef INTRINSIC
#undef C_INTRINSIC
      default:
         return NULL;
      }
   }

bool J9::X86::suppressIntrinsicInlining(TR::RecognizedMethod method, TR::Node *node, TR::CodeGenerator *cg)
   {
   // TODO; test some flag
   return isSupportedIntrinsic(method, node, cg);
   }
