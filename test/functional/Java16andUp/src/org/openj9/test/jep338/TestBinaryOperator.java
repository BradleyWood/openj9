package org.openj9.test.jep338;

import jdk.incubator.vector.VectorOperators;

import org.testng.annotations.DataProvider;
import org.testng.annotations.Test;

import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.Iterator;

import static org.openj9.test.jep338.TestUtility.compareResults;
import static org.openj9.test.jep338.TestUtility.getArrayForType;

@Test(groups = { "level.sanity" })
public class TestBinaryOperator {

    private static final List<VectorOperators.Operator> BINARY_OPERATORS = Arrays.asList(
            VectorOperators.ADD,
            VectorOperators.ADD,
            VectorOperators.SUB,
            VectorOperators.DIV,
            VectorOperators.MUL,
            VectorOperators.AND,
            VectorOperators.MIN,
            VectorOperators.MAX,
            VectorOperators.NEG
    );

    private static final List<Class<?>> TYPES = Arrays.asList(
            byte.class,
            short.class,
            int.class,
            long.class,
            float.class,
            double.class
    );

    private static final List<Integer> VECTOR_LENGTHS = Arrays.asList(
            128
    );
    
    @Test(dataProvider = "ops")
    public void test(final VectorOperators.Operator operator, final Class<?> elementType, final int vectorLength) throws Throwable {
        final TestGenerator gen = new TestGenerator(operator, elementType, vectorLength);
        final Method jitMethod = gen.getMethodForJit();
        final Method interpretedMethod = gen.getMethodForInterpreter();

        Object jitResult = getArrayForType(elementType, false);
        Object interpretedResult = getArrayForType(elementType, false);

        if (operator instanceof VectorOperators.Unary) {
            Object jitOperand = getArrayForType(elementType, true);
            jitMethod.invoke(null, jitOperand, jitResult);

            Object interpretedOperand = getArrayForType(elementType, true);
            interpretedMethod.invoke(null, interpretedOperand, interpretedResult);
        } else {
            Object jitLhs = getArrayForType(elementType, true);
            Object jitRhs = getArrayForType(elementType, true);
            jitMethod.invoke(null, jitLhs, jitRhs, jitResult);

            Object interpretedLhs = getArrayForType(elementType, true);
            Object interpretedRhs = getArrayForType(elementType, true);
            interpretedMethod.invoke(null, interpretedLhs, interpretedRhs, interpretedResult);
        }

        compareResults(interpretedResult, jitResult, elementType);
    }

    @DataProvider(name = "ops")
    public static Iterator<Object[]> data() {
        final List<Object[]> parameters = new ArrayList<>();

        for (Integer vectorLength : VECTOR_LENGTHS) {
            for (VectorOperators.Operator binaryOperator : BINARY_OPERATORS) {
                for (Class<?> type : TYPES) {
                    if (binaryOperator == VectorOperators.AND && (type.equals(float.class) || type.equals(double.class)))
                        continue;
                    parameters.add(new Object[] {binaryOperator, type, vectorLength});
                }
            }
        }

        return parameters.iterator();
    }
}
