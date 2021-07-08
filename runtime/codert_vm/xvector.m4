dnl Copyright IBM Corp. and others 2023
dnl
dnl This program and the accompanying materials are made available under
dnl the terms of the Eclipse Public License 2.0 which accompanies this
dnl distribution and is available at https://www.eclipse.org/legal/epl-2.0/
dnl or the Apache License, Version 2.0 which accompanies this distribution and
dnl is available at https://www.apache.org/licenses/LICENSE-2.0.
dnl
dnl This Source Code may also be made available under the following
dnl Secondary Licenses when the conditions for such availability set
dnl forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
dnl General Public License, version 2 with the GNU Classpath
dnl Exception [1] and GNU General Public License, version 2 with the
dnl OpenJDK Assembly Exception [2].
dnl
dnl [1] https://www.gnu.org/software/classpath/license.html
dnl [2] https://openjdk.org/legal/assembly-exception.html
dnl
dnl SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0-only WITH Classpath-exception-2.0 OR GPL-2.0-only WITH OpenJDK-assembly-exception-1.0

include(xhelpers.m4)

	FILE_START

dnl For both of these functions, on entry:
dnl
dnl 1) Return address on the stack
dnl 2) EAX contains javaVM->extendedRuntimeFlags
dnl 3) _rbp contains the current J9VMThread
dnl
dnl On exit, _rcx is overwritten

START_PROC(jitSaveVectorRegisters)
	pop uword ptr J9TR_VMThread_floatTemp3[_rbp]
	lfence

	dnl save ZMM registers
	forloop({REG_CTR}, 0, 31, {SAVE_ZMM_REG(REG_CTR, J9TR_cframe_jitFPRs+(REG_CTR*64))})

ifdef({ASM_J9VM_ENV_DATA64},{
	forloop({REG_CTR}, 0, 31, {SAVE_ZMM_REG(REG_CTR, J9TR_cframe_jitFPRs+(REG_CTR*64))})
}, { dnl ASM_J9VM_ENV_DATA64
	forloop({REG_CTR}, 0, 7, {SAVE_ZMM_REG(REG_CTR, J9TR_cframe_jitFPRs+(REG_CTR*64))})
})

	vzeroupper

    dnl save Opmask registers
	forloop({REG_CTR}, 0, 7, {SAVE_MASK_64(REG_CTR, J9TR_cframe_maskRegisters+(REG_CTR*8))})

	push uword ptr J9TR_VMThread_floatTemp3[_rbp]
	ret
END_PROC(jitSaveVectorRegisters)

START_PROC(jitRestoreVectorRegisters)
	pop uword ptr J9TR_VMThread_floatTemp3[_rbp]
	lfence

	dnl restore ZMM registers
ifdef({ASM_J9VM_ENV_DATA64},{
	forloop({REG_CTR}, 0, 31, {RESTORE_ZMM_REG(REG_CTR, J9TR_cframe_jitFPRs+(REG_CTR*64))})
}, { dnl ASM_J9VM_ENV_DATA64
	forloop({REG_CTR}, 0, 7, {RESTORE_ZMM_REG(REG_CTR, J9TR_cframe_jitFPRs+(REG_CTR*64))})
})

	dnl restore Opmask registers
	forloop({REG_CTR}, 0, 7, {RESTORE_MASK_64(REG_CTR, J9TR_cframe_maskRegisters+(REG_CTR*8))})

	push uword ptr J9TR_VMThread_floatTemp3[_rbp]
	ret
END_PROC(jitRestoreVectorRegisters)

	FILE_END
