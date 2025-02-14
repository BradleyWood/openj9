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

#ifndef J9_X86INTRINSICS_INCL
#define J9_X86INTRINSICS_INCL

#include "codegen/Register.hpp"
#include "codegen/CodeGenerator.hpp"
#include "il/Node.hpp"
#include "il/Node_inlines.hpp"
#include "jitprotos.h"

namespace J9
{
namespace X86
{

float J9FASTCALL floatFMAImpl(J9VMThread* currentThread, float a, float b, float c);

double J9FASTCALL doubleFMAImpl(J9VMThread* currentThread, double a, double b, double c);

TR::Register *inlineMathFMA(TR::Node *node, TR::CodeGenerator *cg);

/**
 * TODO; Tests where a java method has an intrinsic implementation, and it is supported on the target.
 *
 * @param method The java method to check.
 * @return True if method has a supported intrinsic implementation.
 */
bool isSupportedIntrinsic(TR::RecognizedMethod method, TR::Node *node, TR::CodeGenerator *cg);

/**
 * TODO; Tests whether inlining the intrinsic is always worth it.
 *
 * @param method
 * @return
 */
bool alwaysWorthInlining(TR::RecognizedMethod method);

/**
 * TODO, test some flag?; Tests whether inliner should avoid inlining the call to the java method.
 *
 * @param method The recognized java method
 * @return True to suppress inlining, otherwise false;
 */
bool suppressIntrinsicInlining(TR::RecognizedMethod method, TR::Node *node, TR::CodeGenerator *cg);

/**
 * Generates call to intrinsic with system linkage.
 *
 * @param node The node making the call
 * @param cg The code generator
 * @param dt Return type
 * @param func Ptr to function
 * @return
 */
TR::Register *callCIntrinsic(TR::Node *node, TR::CodeGenerator *cg, TR::DataType dt, void *func);

/**
 * Inlines an intrinsic call.
 *
 * @param method The java method
 * @param node The node making the call
 * @param cg The code generator
 * @return TR::Register* if an intrinsic was found and called, otherwise NULL
 */
TR::Register *dispatchIntrinsic(TR::RecognizedMethod method, TR::Node *node, TR::CodeGenerator *cg);

}
}

#endif
