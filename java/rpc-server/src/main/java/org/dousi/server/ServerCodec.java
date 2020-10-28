package org.dousi.server;

import org.msgpack.core.MessageBufferPacker;
import org.msgpack.core.MessagePack;
import org.msgpack.core.MessagePacker;
import org.msgpack.core.MessageUnpacker;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

import java.io.IOException;
import java.lang.reflect.InvocationTargetException;
import java.lang.reflect.Method;

public class ServerCodec {

    private final static Logger LOG = LoggerFactory.getLogger(ServerCodec.class);

    // Decode the bytes to the method name and arguments to perform.
    public byte[] decode(DousiRpcServer rpcServer, byte[] bytes) throws IOException, InvocationTargetException, IllegalAccessException {
        MessageUnpacker messageUnpacker = MessagePack.newDefaultUnpacker(bytes);

        final int argsNum = messageUnpacker.unpackArrayHeader();
        // The argsNum can be used to find the overloading methods.

        final String methodName = messageUnpacker.unpackString();

        ServiceInfo serviceInfo = rpcServer.getService();
        Method method = serviceInfo.getMethod(methodName);

        Class<?>[] paramTypes = method.getParameterTypes();
        if (paramTypes.length == 0) {
            // no params.
            LOG.debug("no params.");
//            return new Object[0];
        }

        // Check tuple size from bytes and assert equals to paramTypes.
        Object[] ret = new Object[paramTypes.length];
        for (int i = 0; i  < paramTypes.length; ++i) {
            final Class<?> paramType = paramTypes[i];
            Object arg = null;
            if (paramType.equals(Integer.class) || paramType.equals(int.class)) {
                arg = messageUnpacker.unpackInt();
            } else if (paramType.equals(Double.class) || paramType.equals(double.class)) {
                arg = messageUnpacker.unpackDouble();
            } else if (paramType.equals(String.class)) {
                arg = messageUnpacker.unpackString();
            } else {
                // user-defined class.
                // TODO
            }
            ret[i] = arg;
        }

        Object result = method.invoke(serviceInfo.getServiceObject(), ret);
        return encode(result);
    }

    public byte[] encode(Object o) throws IOException {
        MessageBufferPacker messagePacker = MessagePack.newDefaultBufferPacker();
        if (o.getClass().equals(Integer.class) || o.getClass().equals(int.class)) {
            messagePacker.packInt((Integer) o);
        }
        return messagePacker.toByteArray();
    }

}
