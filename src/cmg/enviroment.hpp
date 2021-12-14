/**
 * Author: Cyan
 * Date: Fri Dec  3 11:59:11 CST 2021
 */
#pragma once
#include <memory>
#include <string>
#include <map>

#include "sender.hpp"
#include "receiver.hpp"
#include "socket.hpp"

namespace cmg {

	class Environment {
	public:
		static constexpr auto MULTI_PROC_NAME = "~";

	private:
		Environment(const std::string &proc_name);

	private:
		std::string proc_name_;

		unsigned port_;

		std::shared_ptr<Socket> server_;

		std::map<std::string, std::shared_ptr<Socket>> clients_;

	private:
		static std::map<std::string, Environment> insts;

	public:
		static auto Inst(const std::string &proc_name = "") -> Environment&;

		static void Spin();

	private:
		static auto PortFromKey(const std::string &key) -> unsigned;

	public:
		~Environment();

		template <typename Msg>
		auto sender(const std::string &topic, unsigned wait) -> std::shared_ptr<Sender> {

			if (!this->server_) {

				this->server_ = Socket::Server(this->port_);
			}

			auto sender = std::make_shared<SocketSender>(this->server_, topic, wait);

			return sender;
		}

		template <typename Msg>
		auto receiver(const std::string &topic, unsigned wait, const Receiver::Callback<Msg> &callback) -> std::shared_ptr<Receiver> {

			auto client = Socket::Client(this->port_);
			this->clients_.emplace(topic, client);

			auto receiver = std::make_shared<SocketReceiver<Msg>>(client, topic, callback);
			return receiver;
		}
	};
}
