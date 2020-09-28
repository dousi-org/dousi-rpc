#ifndef _DOUSI_MASTER_MASTER_CLIENT_H_
#define _DOUSI_MASTER_MASTER_CLIENT_H_

#include "core/submitter/service_handle.h"
#include "core/submitter/submitter.h"
#include "common/endpoint.h"
#include "common/logging.h"

#include <msgpack.hpp>

#include <iostream>

namespace dousi {
namespace master {

class MasterClient {
public:

  explicit MasterClient(const std::string &master_server_address) {
      dousi::DousiLog::StartDousiLog("/tmp/dousi/MasterClient.log",
                                     dousi::LOG_LEVEL::DEBUG, 10, 3);
      submitter_ = std::make_shared<Submitter>();
      submitter_->Init(master_server_address);
      const auto master_service_handle = submitter_->GetService("MasterService");
      master_service_handle_ = std::make_shared<ServiceHandle>(submitter_, master_service_handle.GetServiceName());
  }

  virtual ~MasterClient() = default;

  /**
   * Register a Dousi RPC service to the master server.
   *
   * @param service_name The service name to be registered.
   * @param service_address The address of this service.
   */
  void RegisterService(const std::string &service_name,
                       const std::string &service_address);

  /// Note that thsi is a sync call.
  std::unordered_map<std::string, std::string> GetAllEndpoints();

  int32_t SyncRequestProcessId();

private:


private:
    std::shared_ptr<Submitter> submitter_ = nullptr;

    std::shared_ptr<ServiceHandle> master_service_handle_ = nullptr;
};

} // namespace master
} // namespace dousi

#endif
