#ifndef _DOUSI_RUNTIME_H_
#define _DOUSI_RUNTIME_H_

#include "abstract_service.h"

#include <common/logging.h>
#include "common/endpoint.h"
#include "core/submitter/service_handle.h"

#include <nameof/nameof.hpp>

#include "common/noncopyable.h"

#include "core/common/tuple_traits.h"
#include "core/stream/stream.h"

#include <unordered_map>
#include <string>
#include <functional>

#include <boost/asio.hpp>

namespace dousi {


template <typename ServiceType>
class DousiService;

template<typename ReturnType>
struct InvokeHelper {
    // This could be refined as a lambda.
    template<typename MethodType, typename ServiceOriginalType>
    static void Invoke(
            const MethodType &method,
            DousiService<ServiceOriginalType> *service_instance,
            const char *data,
            size_t size,
            std::string &result) {
        msgpack::unpacked unpacked;
        msgpack::unpack(unpacked, data, size);
        using MethodNameWithArgsTupleTypes = typename FunctionTraits<MethodType>::MethodNameWithArgsTuple;
        auto method_name_and_args_tuple = unpacked.get().as<MethodNameWithArgsTupleTypes>();

        auto ret = TraitAndCall(method, service_instance->GetServiceObjectRef(), method_name_and_args_tuple);
//            using ReturnType = typename FunctionTraits<MethodType>::ReturnType;
// TODO(qwang):
//        DOUSI_LOG(INFO) << "The type of the result is " << NAMEOF_TYPE(ReturnType) << ", and the result = " << ret;

        // Serialize result.
        msgpack::sbuffer buffer(1024);
        msgpack::pack(buffer, ret);
        result = {buffer.data(), buffer.size()};

        DOUSI_LOG(INFO) << "Invoke the user method: " << std::string(data, size);
    }
};

template<>
struct InvokeHelper<void> {
    // This could be refined as a lambda.
    template<typename MethodType, typename ServiceOriginalType>
    static void Invoke(
            const MethodType &method,
            DousiService<ServiceOriginalType> *service_instance,
            const char *data,
            size_t size,
            std::string &result) {
        msgpack::unpacked unpacked;
        msgpack::unpack(unpacked, data, size);
        using MethodNameWithArgsTupleTypes = typename FunctionTraits<MethodType>::MethodNameWithArgsTuple;
        auto method_name_and_args_tuple = unpacked.get().as<MethodNameWithArgsTupleTypes>();

        TraitAndCallVoidReturn(method, service_instance->GetServiceObjectRef(), method_name_and_args_tuple);
//            using ReturnType = typename FunctionTraits<MethodType>::ReturnType;
        DOUSI_LOG(INFO) << "The type of the result is void.";
        DOUSI_LOG(INFO) << "Invoke the user method: " << std::string(data, size);
    }
};

/**
 * A singleton executor runtime of dousi.
 */
class Executor : public std::enable_shared_from_this<Executor> {
public:
    Executor() : io_service_(std::thread::hardware_concurrency()), work_(io_service_) {}

    void Init(const std::string &listening_address) {
        this->listening_address_ = listening_address;
        acceptor_ = std::make_unique<boost::asio::ip::tcp::acceptor>(
                io_service_, Endpoint(listening_address).GetTcpEndpoint());
    }

    void Shutdown() {
        acceptor_->close();
    }

    void Loop() {
        DoAccept();
        io_service_.run();
    }


    template<typename ServiceType>
    DousiService<ServiceType> CreateService(const std::string &service_name) {
//        constexpr auto service_name  = std::string(NAMEOF_TYPE(ServiceType));
        DOUSI_LOG(INFO) << "Registering service: " << service_name;
        auto service_ptr = std::make_shared<ServiceType>();
        created_services_[service_name] = service_ptr;
        return DousiService<ServiceType>(shared_from_this(), service_ptr);
    }

    template<typename ServiceOriginalType, typename MethodType>
    void RegisterMethod(DousiService<ServiceOriginalType> *service_instance, RemoteMethod<MethodType> remote_method) {
        registered_methods_[remote_method.GetName()] =  std::bind(
                &InvokeHelper<typename FunctionTraits<MethodType>::ReturnType>::template Invoke<MethodType, ServiceOriginalType>,
                remote_method.GetMethod(),
                service_instance,
                std::placeholders::_1,
                std::placeholders::_2,
                std::placeholders::_3
                );
    }

    void InvokeMethod(uint64_t stream_id, uint32_t object_id, const std::string &data, std::string &result) {
        msgpack::unpacked unpacked;
        msgpack::unpack(unpacked, data.data(), data.size());
        auto tuple = unpacked.get().as<std::tuple<std::string>>();
        const auto method_name = std::get<0>(tuple);
        registered_methods_[method_name](data.data(), data.size(), result);

        auto it = streams_.find(stream_id);
        assert(it != streams_.end());
        std::shared_ptr<AsioStream> stream = it->second;

        // Note that this result is already serialized since we should know the ReturnType of it.
        stream->Write(object_id, result);
        DOUSI_LOG(INFO) << "Method invoked, method name is " << method_name << ", result is \"" << result << "\".";
    }

    uint64_t RequestStreamID() {
        return curr_stream_id_.fetch_add(1, std::memory_order_relaxed);
    }

private:
    void DoAccept();

private:
    std::atomic<uint64_t> curr_stream_id_ =  0;

    // Whether this is unused? If so, remove AbstractService.
    std::unordered_map<std::string, std::shared_ptr<AbstractService>> created_services_;

    std::unordered_map<std::string, std::function<void(const char *, size_t, std::string &)>> registered_methods_;

    boost::asio::io_service io_service_;

    boost::asio::io_service::work work_;

    // The listening address that this RPC server listening on.
    std::string listening_address_;

    // acceptor
    std::unique_ptr<boost::asio::ip::tcp::acceptor> acceptor_ = nullptr;

    // The stream id to the stream pointer.
    std::unordered_map<uint64_t , std::shared_ptr<AsioStream>> streams_;
};

}

#endif
