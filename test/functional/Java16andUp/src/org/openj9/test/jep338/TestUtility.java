package org.openj9.test.jep338;

import org.testng.Assert;

public class TestUtility {

    public static void compareResults(Object interpretedResults, Object jittedResults, Class<?> type) {
        if (type.equals(byte.class)) {
            Assert.assertEquals((byte[]) interpretedResults, (byte[]) jittedResults);
        } else if (type.equals(short.class)) {
            Assert.assertEquals((short[]) interpretedResults, (short[]) jittedResults);
        } else if (type.equals(int.class)) {
            Assert.assertEquals((int[]) interpretedResults, (int[]) jittedResults);
        } else if (type.equals(long.class)) {
            Assert.assertEquals((long[]) interpretedResults, (long[]) jittedResults);
        } else if (type.equals(float.class)) {
            Assert.assertEquals((float[]) interpretedResults, (float[]) jittedResults);
        } else if (type.equals(double.class)) {
            Assert.assertEquals((double[]) interpretedResults, (double[]) jittedResults);
        }
    }

    public static Object getArrayForType(Class<?> type, boolean init) {
        if (!init) {
            if (type.equals(byte.class)) {
                return new byte[16];
            } else if (type.equals(short.class)) {
                return new short[8];
            } else if (type.equals(int.class)) {
                return new int[4];
            } else if (type.equals(long.class)) {
                return new long[2];
            } else if (type.equals(float.class)) {
                return new float[4];
            } else if (type.equals(double.class)) {
                return new double[2];
            }
        } else {
            if (type.equals(byte.class)) {
                return new byte[] {1, 2, 3, 5, 10, 20, 40, 60, 80, 10, 100, 9, 12, 55, 12, 10};
            } else if (type.equals(short.class)) {
                return new short[] {20, 10, 20, 30, 100, 10, 22, 60};
            } else if (type.equals(int.class)) {
                return new int[] { 90, 10, 5, 9};
            } else if (type.equals(long.class)) {
                return new long[] {50, 111};
            } else if (type.equals(float.class)) {
                return new float[] {10.0f, 25.0f, 100.0f, 79.0f};
            } else if (type.equals(double.class)) {
                return new double[] {3.14159, 15.551};
            }
        }
        return null;
    }
}
