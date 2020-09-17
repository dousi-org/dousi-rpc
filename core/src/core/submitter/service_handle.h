#ifndef _SERVICE_HANDLE_H_
#define _SERVICE_HANDLE_H_


#include "dousi_future.h"

#include "core/submitter/submitter_runtime.h"
#include "core/common/function_traits.h"
#include "common/msgpack_utils.h"

#include <string>
#include <utility>

namespace dousi {


template <typename ReturnType>
class RemoteMethod;



class ServiceHandle {
public:
    explicit ServiceHandle(std::string service_name) :service_name_(std::move(service_name)) {}

    template<typename MethodType, typename... ArgTypes>
    auto Call(RemoteMethod<MethodType> method, ArgTypes... args);

    template<typename MethodType, typename... ArgTypes>
    auto Call2(RemoteMethod<MethodType> method, ArgTypes... args);


    struct InternalCaller {

        template<typename MethodType, typename ArgsTupleType>
        static auto MyCall(RemoteMethod<MethodType> method, ArgsTupleType &&args_tuple, NonVoidReturnTag unused)
                -> DousiFuture<typename FunctionTraits<MethodType>::ReturnType> {
            const auto object_id = SubmitterRuntime::GetInstance().RequestObjectID();

            msgpack::sbuffer sbuffer = common::PackArgsToBuffer(std::forward<ArgsTupleType>(args_tuple));
            using ReturnType = typename FunctionTraits<MethodType>::ReturnType;
            std::string buffer {sbuffer.data(), sbuffer.size()};

            {
                msgpack::unpacked unpacked;
                msgpack::unpack(unpacked, buffer.data(), buffer.size());
                auto tuple = unpacked.get().as<std::tuple<std::string>>();
                const auto method_name = std::get<0>(tuple);
                DOUSI_LOG(INFO) << "----------------------ok, oject_id=" << object_id;
            }

            SubmitterRuntime::GetInstance().Submit(object_id, method.GetName(), buffer);
            return DousiFuture<ReturnType> { object_id };
        }

        template<typename MethodType, typename ArgsTupleType>
        static auto MyCall(RemoteMethod<MethodType> method, ArgsTupleType &&args_tuple, VoidReturnTag unused)
                -> DousiFuture<bool> {
            const auto object_id = SubmitterRuntime::GetInstance().RequestObjectID();

            msgpack::sbuffer sbuffer = common::PackArgsToBuffer(args_tuple);

            std::string buffer {sbuffer.data(), sbuffer.size()};
            {
                msgpack::unpacked unpacked;
                msgpack::unpack(unpacked, buffer.data(), buffer.size());
                auto tuple = unpacked.get().as<std::tuple<std::string>>();
                const auto method_name = std::get<0>(tuple);
                DOUSI_LOG(INFO) << "----------------------ok, oject_id=" << object_id;
            }

            SubmitterRuntime::GetInstance().Submit(object_id, method.GetName(), buffer);
            return DousiFuture<bool> { object_id };
        }
    };

private:
    std::string service_name_;
};


//////////////////////////////////////////// Implementations

template<typename MethodType, typename... ArgTypes>
auto ServiceHandle::Call(RemoteMethod<MethodType> method, ArgTypes... args) {
    return Call2(method, method.GetName(), args...);
}

template<typename MethodType, typename... ArgTypes>
auto ServiceHandle::Call2(RemoteMethod<MethodType> method, ArgTypes... args) {
    using ReturnType = typename FunctionTraits<MethodType>::ReturnType;
    return InternalCaller::MyCall(method, std::forward_as_tuple(std::forward<ArgTypes>(args)...), typename VoidReturnTrait<ReturnType>::Tag {});
}

}
#endif
