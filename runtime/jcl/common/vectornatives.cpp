/*******************************************************************************
 * Copyright IBM Corp. and others 2020
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
#include "j9.h"
#include "jclprots.h"
#include "j9protos.h"

#define DEFAULT_VECTOR_REGISTER_SIZE 128

extern "C" {

jint JNICALL
Java_jdk_internal_vm_vector_VectorSupport_registerNatives(JNIEnv *env, jclass clazz)
{
	return 0;
}

jint JNICALL
Java_jdk_internal_vm_vector_VectorSupport_getMaxLaneCount(JNIEnv *env, jclass clazz, jclass elementType)
{
	J9VMThread *currentThread = (J9VMThread *) env;
	J9JavaVM *vm = currentThread->javaVM;
	J9InternalVMFunctions *vmFuncs = vm->internalVMFunctions;
	jint laneCount = 0;

	vmFuncs->internalEnterVMFromJNI(currentThread);
	j9object_t classObj = J9_JNI_UNWRAP_REFERENCE(elementType);
	if (NULL == classObj) {
		vmFuncs->setCurrentException(currentThread, J9VMCONSTANTPOOL_JAVALANGNULLPOINTEREXCEPTION, NULL);
	} else {
		J9Class *elementType = J9VM_J9CLASS_FROM_HEAPCLASS(currentThread, classObj);
		jint vectorRegSize = DEFAULT_VECTOR_REGISTER_SIZE;

#if defined(J9X86) || defined(J9HAMMER)
		// In CRIU mode, we limit species-preferred to 128-bits for portability. Species preferred is stable and cannot
		// be reevaluated post-restore.
		if (!vm->internalVMFunctions->isCheckpointAllowed(vm)) {
			PORT_ACCESS_FROM_JAVAVM(vm);
			OMRPORT_ACCESS_FROM_J9PORT(PORTLIB);

			OMRProcessorDesc desc;
			omrsysinfo_get_processor_description(&desc);

			// The portlibrary does not support OS feature checks for YMM/ZMM registers
			// Therefore, species preferred will be overestimated on modern hardware running
			// old operating systems such as Windows Server 2012.
			if (omrsysinfo_processor_has_feature(&desc, OMR_FEATURE_X86_AVX512F)) {
				vectorRegSize = 512;
			} else if (omrsysinfo_processor_has_feature(&desc, OMR_FEATURE_X86_AVX2)) {
				vectorRegSize = 256;
			}

			// TODO; Floating-point operations can be supported with 256-bit vectors on AVX hardware (without AVX-2). Some
			// JIT evaluators (like mask operations) use integer opcodes which aren't supported on 256-bit vectors by AVX.
			// Until this fixed or improved, we limit floating-point operations to 128-bits on hardware without AVX-2.
			// This will prevent the JIT from unnecessarily rejecting vectorization of these operations.
      }
#endif

		if (elementType == vm->byteReflectClass) {
			laneCount = vectorRegSize / 8;
		} else if (elementType == vm->shortReflectClass) {
			laneCount = vectorRegSize / 16;
		} else if ((elementType == vm->intReflectClass) || (elementType == vm->floatReflectClass)) {
			laneCount = vectorRegSize / 32;
		} else if ((elementType == vm->longReflectClass) || (elementType == vm->doubleReflectClass)) {
			laneCount = vectorRegSize / 64;
		} else {
			vmFuncs->setCurrentException(currentThread, J9VMCONSTANTPOOL_JAVALANGILLEGALARGUMENTEXCEPTION, NULL);
		}
	}

	vmFuncs->internalExitVMToJNI(currentThread);


	return laneCount;
}

}
