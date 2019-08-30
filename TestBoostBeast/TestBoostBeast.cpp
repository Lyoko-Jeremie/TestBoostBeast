// TestBoostBeast.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <sstream>
#include <string>
#include <cstdlib>
#include <functional>
#include <memory>
#include <chrono>
#include <thread>
#include <optional>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>

#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>

namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;

namespace pt = boost::property_tree;


const std::string content_type_json = "application/json";
const std::string content_type_text = "text/plain";

// https://www.boost.org/doc/libs/master/libs/beast/example/http/client/async/http_client_async.cpp
class session : public std::enable_shared_from_this<session>
{
	tcp::resolver resolver_;
	beast::tcp_stream stream_;
	beast::flat_buffer buffer_; // (Must persist between reads)
	http::request<http::string_body> req_;
	http::response<http::string_body> res_;

private:
	void fail(beast::error_code ec, char const* what)
	{
		std::cerr << what << ": " << ec.message() << "\n";
		if (responseCallback.has_value())
			responseCallback.value()(session::ResponseCallbackErrorTypeStruct{ec, std::string{what}}, res_);
	}

public:

	explicit session(asio::io_context& ioc)
		: resolver_(asio::make_strand(ioc))
		  , stream_(asio::make_strand(ioc))
	{
	}

	std::string host;
	std::string port;
	std::string target;
	int version;

	void configRequest(std::string host,
	                   std::string port,
	                   std::string target,
	                   http::verb method = http::verb::get,
	                   int version = 10)
	{
		this->host = host;
		this->port = port;
		this->target = target;
		this->version = version;

		// Set up an request message
		req_.version(version);
		req_.method(method);
		req_.target(target);
		req_.set(http::field::host, host);
		req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);
	}

	void setHeader(std::string key, std::string value)
	{
		req_.set(key, value);
	}

	void setBody(
		std::string content_type = content_type_text,
		std::string body = "")
	{
		req_.set(http::field::content_type, content_type);
		req_.body() = body;
		req_.prepare_payload();
	}

	void setBodyJson(std::string body = "")
	{
		req_.set(http::field::content_type, content_type_json);
		req_.body() = body;
		req_.prepare_payload();
	}

	void setBodyText(std::string body = "")
	{
		req_.set(http::field::content_type, content_type_text);
		req_.body() = body;
		req_.prepare_payload();
	}

	// void run(std::string host,
	//          std::string port,
	//          std::string target,
	//          int version)
	void run()
	{
		// // Set up an HTTP GET request message
		// req_.version(version);
		// req_.method(http::verb::get);
		// req_.target(target);
		// req_.set(http::field::host, host);
		// req_.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		resolver_.async_resolve(
			host,
			port,
			beast::bind_front_handler(
				&session::on_resolve,
				shared_from_this()
			)
		);
	}

private:
	void on_resolve(beast::error_code ec, tcp::resolver::results_type results)
	{
		if (ec)
		{
			return fail(ec, "resolve");
		}

		stream_.expires_after(std::chrono::seconds{30});

		stream_.async_connect(
			results,
			beast::bind_front_handler(
				&session::on_connect,
				shared_from_this()
			)
		);
	}

	void on_connect(beast::error_code ec, tcp::resolver::results_type::endpoint_type)
	{
		if (ec)
		{
			return fail(ec, "connect");
		}

		stream_.expires_after(std::chrono::seconds{30});

		http::async_write(
			stream_,
			req_,
			beast::bind_front_handler(
				&session::on_write,
				shared_from_this()
			)
		);
	}

	void on_write(beast::error_code ec, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
		{
			return fail(ec, "write");
		}

		http::async_read(
			stream_,
			buffer_,
			res_,
			beast::bind_front_handler(
				&session::on_read,
				shared_from_this()
			)
		);
	}

	void on_read(beast::error_code ec, std::size_t bytes_transferred)
	{
		boost::ignore_unused(bytes_transferred);

		if (ec)
		{
			return fail(ec, "read");
		}

		std::cout << res_ << std::endl;
		std::cout << "===================================" << std::endl;
		std::cout << res_.body() << std::endl;
		std::cout << "===================================" << std::endl;

		stream_.socket().shutdown(tcp::socket::shutdown_both, ec);

		if (ec && ec != beast::errc::not_connected)
		{
			return fail(ec, "shutdown");
		}

		// If we get here then the connection is closed gracefully

		if (responseCallback.has_value())
			responseCallback.value()({}, res_);
	}

public:
	// void noop_callback(std::optional<std::tuple<beast::error_code, std::string>> error, decltype(res_) response)
	// {
	// 	/* do nothing */
	// }

	struct ResponseCallbackErrorTypeStruct
	{
		beast::error_code ec;
		std::string what;

		ResponseCallbackErrorTypeStruct(beast::error_code ec, std::string what)
			: ec(ec), what(what)
		{
		}
	};

	using responseCallbackErrorType = std::optional<ResponseCallbackErrorTypeStruct>;
	using responseCallbackResponseType = decltype(res_)&;
	using responseCallbackType = std::function<void(responseCallbackErrorType, responseCallbackResponseType)>;
	std::optional<responseCallbackType> responseCallback;
};


void io_thread_main()
{
	asio::io_context ioc;

	// debug 
	bool isAdded = false;
	bool isEnd = false;

	while (true)
	{
		// TODO check exit flag
		if (false || isEnd /*TODO condition*/)
		{
			break;
		}

		// TODO check to add next httpSession to ioc
		if (true && !isAdded /*TODO condition*/)
		{
			isAdded = true;

			try
			{
				std::string host = "localhost";
				std::string port = "3000";
				// std::string target = "/test";
				std::string target = "/json";
				int version = 10;

				auto httpSession = std::make_shared<session>(ioc);
				// se->configRequest(host, port, target);
				httpSession->configRequest(host, port, target, http::verb::post);
				httpSession->setHeader("X-Secret-Key", "EncryptedHandshakeKey");
				httpSession->setHeader("X-Secret-Token", "EncryptedHandshakeToken");
				// se->setBody(content_type_json, R"({"test":123,"foobar":987654321})");
				// httpSession->setBodyJson(R"({"test":123,"foobar":987654321})");

				{
					pt::ptree json;
					json.put("test", 123);
					json.put("foobar", 987654321);

					json.add("listFoo", 1);
					json.add("listFoo", 2);
					json.add("listFoo", 3);

					std::stringstream jsonString;
					pt::write_json(jsonString, json);
					httpSession->setBodyJson(jsonString.str());
				}

				httpSession->responseCallback = [&isEnd](session::responseCallbackErrorType E,
				                                         session::responseCallbackResponseType res)
				{
					isEnd = true;

					if (E.has_value())
					{
						std::cerr << "responseCallback error: " << E.value().what
							<< " : " << E.value().ec.message() << std::endl;
						return;
					}

					std::stringstream jsonString;
					jsonString.str(res.body());

					try
					{
						pt::ptree json;
						pt::read_json(jsonString, json);

						auto test = json.get<int>("test");
						auto foo = json.get<int>("foo");
						auto bar = json.get_optional<int>("bar");

						std::cout << "responseCallback the data : "
							<< "\n\t" << "test:" << test
							<< "\n\t" << "foo:" << foo
							<< "\n\t" << "bar:" << bar.value_or(0)
							<< (bar.has_value() ? " (real value)" : " (default value)")
							<< std::endl;
					}
					catch (std::exception const& e)
					{
						std::cerr << "responseCallback json error: " << e.what() << std::endl;
						return;
					}
				};

				httpSession->run();
			}
			catch (std::exception const& e)
			{
				std::cerr << e.what() << std::endl;
				// TODO
				throw e;
			}
		}

		ioc.run_for(std::chrono::microseconds{500});

		std::this_thread::yield();
	}
}

int main()
{
	std::cout << "Hello World!\n";

	std::thread io_th{io_thread_main};

	io_th.join();

	return 0;
}


// async version
int main02()
{
	std::cout << "Hello World!\n";

	asio::io_context ioc;

	std::string host = "localhost";
	std::string port = "3000";
	// std::string target = "/test";
	std::string target = "/json";
	int version = 10;

	auto httpSession = std::make_shared<session>(ioc);
	// se->configRequest(host, port, target);
	httpSession->configRequest(host, port, target, http::verb::post);
	httpSession->setHeader("X-Secret-Key", "EncryptedHandshakeKey");
	httpSession->setHeader("X-Secret-Token", "EncryptedHandshakeToken");
	// se->setBody(content_type_json, R"({"test":123,"foobar":987654321})");
	httpSession->setBodyJson(R"({"test":123,"foobar":987654321})");
	httpSession->run();

	ioc.run();

	return 0;
}

// sync version
// https://www.boost.org/doc/libs/master/libs/beast/doc/html/beast/quick_start/http_client.html
int main01()
{
	std::cout << "Hello World!\n";


	asio::io_context ioc;

	std::string host = "localhost";
	std::string target = "/test";

	try
	{
		tcp::resolver resolver(ioc);
		beast::tcp_stream stream(ioc);

		auto const results = resolver.resolve(host, "3000");

		stream.connect(results);


		http::request<http::string_body> req{http::verb::get, target, 11};
		req.set(http::field::host, host);
		req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

		req.set("X-Secret-Key", "EncryptedHandshakeKey");
		req.set("X-Secret-Token", "EncryptedHandshakeToken");

		http::write(stream, req);


		beast::flat_buffer buffer;

		http::response<http::dynamic_body> res;

		http::read(stream, buffer, res);

		std::cout << res << std::endl;

		beast::error_code errc;
		stream.socket().shutdown(tcp::socket::shutdown_both, errc);

		if (errc && errc != beast::errc::not_connected)
		{
			std::cerr << errc.message() << std::endl;
			throw beast::system_error(errc);
		}
	}
	catch (std::exception const& e)
	{
		std::cerr << e.what() << std::endl;
	}


	return 0;
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门提示: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
