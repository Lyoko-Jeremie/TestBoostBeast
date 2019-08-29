// TestBoostBeast.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include <string>
#include <cstdlib>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>


namespace beast = boost::beast;
namespace http = beast::http;
namespace asio = boost::asio;
using tcp = asio::ip::tcp;


int main()
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
