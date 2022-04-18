package org.openj9.test.jep338;

import jdk.incubator.vector.VectorOperators;
import jdk.incubator.vector.VectorSpecies;
import org.objectweb.asm.ClassWriter;
import org.objectweb.asm.Label;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.Type;

import java.lang.reflect.Method;

public class TestGenerator implements Opcodes {

    private final VectorOperators.Operator operator;
    private final Class<?> elementType;
    private final int vectorLength;
    private Class<?> compiledClazz;
    private Method jitMethod;
    private Method interpretedMethod;

    public TestGenerator(final VectorOperators.Operator operator, final Class<?> elementType, final int vectorLength) {
        this.operator = operator;
        this.elementType = elementType;
        this.vectorLength = vectorLength;
    }

    public byte[] dump() {
        final String testName = getTestName();
        final ClassWriter classWriter = new ClassWriter(ClassWriter.COMPUTE_FRAMES + ClassWriter.COMPUTE_FRAMES);
        MethodVisitor methodVisitor;

        classWriter.visit(V16, ACC_PUBLIC | ACC_SUPER, getTestClassName(), null, "java/lang/Object", null);

        final String vectorClass = getVectorType(elementType);

        final Type argDesc = Type.getType(elementType.arrayType());
        final Type vectorClassType = Type.getType("L" + vectorClass + ";");
        final String species = "SPECIES_" + vectorLength;

        {
            methodVisitor = classWriter.visitMethod(ACC_PUBLIC, "<init>", "()V", null, null);
            methodVisitor.visitCode();
            Label label0 = new Label();
            methodVisitor.visitLabel(label0);
            methodVisitor.visitLineNumber(8, label0);
            methodVisitor.visitVarInsn(ALOAD, 0);
            methodVisitor.visitMethodInsn(INVOKESPECIAL, "java/lang/Object", "<init>", "()V", false);
            methodVisitor.visitInsn(RETURN);
            Label label1 = new Label();
            methodVisitor.visitLabel(label1);
            methodVisitor.visitLocalVariable("this", "Lcom/ibm/vectorapi/" + testName + ";", null, label0, label1, 0);
            methodVisitor.visitMaxs(1, 1);
            methodVisitor.visitEnd();
        }
        {
            final String fromArrayDescriptor = Type.getMethodType(vectorClassType, Type.getType(VectorSpecies.class), Type.getType(elementType.arrayType()), Type.getType(int.class)).getDescriptor();
            Type testType = Type.getMethodType(Type.VOID_TYPE, argDesc, argDesc, argDesc);

            if (operator instanceof VectorOperators.Unary) {
                testType = Type.getMethodType(Type.VOID_TYPE, argDesc, argDesc);
            }

            methodVisitor = classWriter.visitMethod(ACC_PUBLIC | ACC_STATIC, "jit_test", testType.getDescriptor(), null, null);
            methodVisitor.visitCode();

            methodVisitor.visitFieldInsn(GETSTATIC, vectorClass, species, "Ljdk/incubator/vector/VectorSpecies;");
            methodVisitor.visitVarInsn(ALOAD, 0);
            methodVisitor.visitInsn(ICONST_0);
            methodVisitor.visitMethodInsn(INVOKESTATIC, vectorClass, "fromArray", fromArrayDescriptor, false);

            if (operator instanceof VectorOperators.Unary) {
                methodVisitor.visitMethodInsn(INVOKEVIRTUAL, vectorClass, operator.name().toLowerCase(), "()L" + vectorClass + ";", false);
                methodVisitor.visitVarInsn(ALOAD, 1);
            } else {
                methodVisitor.visitFieldInsn(GETSTATIC, vectorClass, species, "Ljdk/incubator/vector/VectorSpecies;");
                methodVisitor.visitVarInsn(ALOAD, 1);
                methodVisitor.visitInsn(ICONST_0);
                methodVisitor.visitMethodInsn(INVOKESTATIC, vectorClass, "fromArray", fromArrayDescriptor, false);
                methodVisitor.visitMethodInsn(INVOKEVIRTUAL, vectorClass, operator.name().toLowerCase(), "(Ljdk/incubator/vector/Vector;)L" + vectorClass + ";", false);
                methodVisitor.visitVarInsn(ALOAD, 2);
            }

            methodVisitor.visitInsn(ICONST_0);

            final Type intoArrayType = Type.getMethodType(Type.VOID_TYPE, argDesc, Type.INT_TYPE);
            methodVisitor.visitMethodInsn(INVOKEVIRTUAL, vectorClass, "intoArray", intoArrayType.getDescriptor(), false);

            methodVisitor.visitInsn(RETURN);
            methodVisitor.visitMaxs(4, 3);
            methodVisitor.visitEnd();
        }

        {
            final String fromArrayDescriptor = Type.getMethodType(vectorClassType, Type.getType(VectorSpecies.class), Type.getType(elementType.arrayType()), Type.getType(int.class)).getDescriptor();
            Type testType = Type.getMethodType(Type.VOID_TYPE, argDesc, argDesc, argDesc);

            if (operator instanceof VectorOperators.Unary) {
                testType = Type.getMethodType(Type.VOID_TYPE, argDesc, argDesc);
            }

            methodVisitor = classWriter.visitMethod(ACC_PUBLIC | ACC_STATIC, "jit_off_test", testType.getDescriptor(), null, null);
            methodVisitor.visitCode();

            methodVisitor.visitFieldInsn(GETSTATIC, vectorClass, species, "Ljdk/incubator/vector/VectorSpecies;");
            methodVisitor.visitVarInsn(ALOAD, 0);
            methodVisitor.visitInsn(ICONST_0);
            methodVisitor.visitMethodInsn(INVOKESTATIC, vectorClass, "fromArray", fromArrayDescriptor, false);

            if (operator instanceof VectorOperators.Unary) {
                methodVisitor.visitMethodInsn(INVOKEVIRTUAL, vectorClass, operator.name().toLowerCase(), "()L" + vectorClass + ";", false);
                methodVisitor.visitVarInsn(ALOAD, 1);
            } else {
                methodVisitor.visitFieldInsn(GETSTATIC, vectorClass, species, "Ljdk/incubator/vector/VectorSpecies;");
                methodVisitor.visitVarInsn(ALOAD, 1);
                methodVisitor.visitInsn(ICONST_0);
                methodVisitor.visitMethodInsn(INVOKESTATIC, vectorClass, "fromArray", fromArrayDescriptor, false);
                methodVisitor.visitMethodInsn(INVOKEVIRTUAL, vectorClass, operator.name().toLowerCase(), "(Ljdk/incubator/vector/Vector;)L" + vectorClass + ";", false);
                methodVisitor.visitVarInsn(ALOAD, 2);
            }

            methodVisitor.visitInsn(ICONST_0);

            final Type intoArrayType = Type.getMethodType(Type.VOID_TYPE, argDesc, Type.INT_TYPE);
            methodVisitor.visitMethodInsn(INVOKEVIRTUAL, vectorClass, "intoArray", intoArrayType.getDescriptor(), false);

            methodVisitor.visitInsn(RETURN);

            methodVisitor.visitMaxs(4, 3);
            methodVisitor.visitEnd();
        }

        classWriter.visitEnd();

        return classWriter.toByteArray();
    }

    private Class<?> getClazz() throws Exception {
        if (compiledClazz != null)
            return compiledClazz;

        final byte[] bytes = dump();
        final ClassLoader cl = new ClassLoader() {
            @Override
            protected Class<?> findClass(String className) throws ClassNotFoundException {
                if (className.equals(getTestClassName().replaceAll("/", "."))) {
                    return defineClass(className, bytes, 0, bytes.length);
                }
                return super.findClass(className);
            }
        };

        compiledClazz = cl.loadClass(getTestClassName().replaceAll("/", "."));
        return compiledClazz;
    }

    public Method getMethodForJit() throws Exception {
        if (jitMethod != null)
            return jitMethod;

        final Class<?> aClass = getClazz();

        for (Method declaredMethod : aClass.getDeclaredMethods()) {
            if (declaredMethod.getName().startsWith("jit_test")) {
                declaredMethod.setAccessible(true);
                jitMethod = declaredMethod;
                return declaredMethod;
            }
        }

        return null;
    }

    public Method getMethodForInterpreter() throws Exception {
        if (interpretedMethod != null)
            return interpretedMethod;

        final Class<?> aClass = getClazz();

        for (Method declaredMethod : aClass.getDeclaredMethods()) {
            if (declaredMethod.getName().startsWith("jit_off_test")) {
                declaredMethod.setAccessible(true);
                interpretedMethod = declaredMethod;
                return declaredMethod;
            }
        }

        return null;
    }

    private String getTestName() {
        return "Vector" + vectorLength +
                elementType.getName().replaceFirst(elementType.getName().charAt(0) + "", (elementType.getName().charAt(0) + "").toUpperCase()) +
                operator.name().charAt(0) + operator.name().substring(1).toLowerCase();
    }

    private String getTestClassName() {
        return "com/ibm/vectorapi/" + getTestName();
    }

    private String getVectorType(Class<?> elementType) {
        if (elementType.equals(byte.class)) {
            return "jdk/incubator/vector/ByteVector";
        } else if (elementType.equals(short.class)) {
            return "jdk/incubator/vector/ShortVector";
        } else if (elementType.equals(int.class)) {
            return "jdk/incubator/vector/IntVector";
        } else if (elementType.equals(float.class)) {
            return "jdk/incubator/vector/FloatVector";
        } else if (elementType.equals(double.class)) {
            return "jdk/incubator/vector/DoubleVector";
        } else if (elementType.equals(long.class)) {
            return "jdk/incubator/vector/LongVector";
        } else {
            throw new IllegalArgumentException();
        }
    }
}
