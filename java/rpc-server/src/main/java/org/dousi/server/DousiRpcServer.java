package org.dousi.server;

import io.netty.bootstrap.ServerBootstrap;
import io.netty.buffer.PooledByteBufAllocator;
import io.netty.channel.ChannelFuture;
import io.netty.channel.ChannelInitializer;
import io.netty.channel.ChannelOption;
import io.netty.channel.ChannelPipeline;
import io.netty.channel.nio.NioEventLoopGroup;
import io.netty.channel.socket.SocketChannel;
import io.netty.channel.socket.nio.NioServerSocketChannel;

import java.util.concurrent.ConcurrentHashMap;

public class DousiRpcServer {

    private io.netty.channel.Channel serverChannel;

    private NioEventLoopGroup bossGroup;

    private NioEventLoopGroup workerGroup;

    private ConcurrentHashMap<String, ServiceInfo> serviceInfo = new ConcurrentHashMap<>();

    public DousiRpcServer(String listeningAddr) {
        bossGroup = new NioEventLoopGroup(1);
        workerGroup = new NioEventLoopGroup();
    }

    public ServiceInfo getService() {
        return serviceInfo.get("org.dousi.examples.adder.AdderService");
    }

    public void registerService(Class<?> service, Object instance) {
        serviceInfo.put(service.getName(), new ServiceInfo(service.getName(), instance));
    }

    public void loop() throws InterruptedException {
        ServerBootstrap serverBootstrap = new ServerBootstrap();
        serverBootstrap.group(bossGroup, workerGroup)
                .channel(NioServerSocketChannel.class)
                .childHandler(new ChannelInitializer<SocketChannel>() {
                    @Override
                    protected void initChannel(SocketChannel ch) {
                        ChannelPipeline pipeline = ch.pipeline();
//                        pipeline.addLast("decoder", new ProtobufVarint32FrameDecoder());
//                        pipeline.addLast("encoder", new ProtobufVarint32LengthFieldPrepender());
                        pipeline.addLast("handler", new DousiRpcServerChannelHandler(DousiRpcServer.this));
                    }
                });
        serverBootstrap.childOption(ChannelOption.TCP_NODELAY, true);
        serverBootstrap.childOption(ChannelOption.SO_KEEPALIVE, true);
        serverBootstrap.childOption(ChannelOption.ALLOCATOR, PooledByteBufAllocator.DEFAULT);

        ChannelFuture f = serverBootstrap.bind(10001).syncUninterruptibly();
        serverChannel = f.channel();
        serverChannel.closeFuture().sync();
    }

    public void close() {
        if (serverChannel != null) {
            serverChannel.close();
        }
        if (bossGroup != null) {
            bossGroup.shutdownGracefully();
            bossGroup = null;
        }
        if (workerGroup != null) {
            workerGroup.shutdownGracefully();
            workerGroup = null;
        }
    }
}
