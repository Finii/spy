// spy.cc
//
// Small tool to observe the raw output of two sockets simultaneously
// and print the data received in hexdump and ascii format.
// With timestamps.
//
// My use: I have one Moxa terminal server to spy on a serial communication.
// With a double-Y adaptor the RXD of the main connection in sent to one
// Moxa port, while the TXD is sent to the other moxa port.
//
// Resulting is a tcpdump style log of the actual traffic on the rs232 lines.
//
// This very basic approach has several advantages over more sophisticated
// techniques, the main being that it is always correct ;)
//
// Before this I used several different programs like netcat, socat, wireshark.
// Well, none of these gives such a clear representation and is able to detect
// cable issues and so on...
//
// Remark: Not really any error handling, do not use for production
//
// Compile hint: g++ -std=c++11 spy.cc -lpthread -lboost_system -o spy
//
// Fini 12/2017

#include <mutex>
#include <thread>
#include <string>
#include <iostream>

#include <boost/asio.hpp>

#define BOOST_CHRONO_HEADER_ONLY
#include <boost/chrono/chrono.hpp>
#include <boost/chrono/time_point.hpp>
#define BOOST_ALLOW_DEPRECATED_HEADERS
#include <boost/chrono/io/time_point_io.hpp>

std::mutex cerr_mutex;

void hexdump(const void* ptr, const int buflen, const std::string& prompt, int color = 0)
{
	std::lock_guard<std::mutex> lock(cerr_mutex);
	std::string indent(prompt.length(), ' ');
	if (color)
		fprintf(stderr, "\033[%dm", color);

	// By epatel @ stack overflow
	// https://stackoverflow.com/a/29865
	auto buf = reinterpret_cast<const unsigned char*>(ptr);
	int i, j;
	for (i = 0; i < buflen; i += 16) {
		fprintf(stderr, "%s%06x: ", i ? indent.c_str() : prompt.c_str(), i);
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				fprintf(stderr, "%02x ", buf[i + j]);
			else
				fprintf(stderr, "   ");
		fprintf(stderr, " ");
		for (j = 0; j < 16; j++)
			if (i + j < buflen)
				fprintf(stderr, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
		fprintf(stderr, "\n");
	}

	if (color)
		fprintf(stderr, "\033[0m");
}

using boost::asio::ip::tcp;
boost::asio::io_service io_service;

struct dump_id {
	std::string prompt;
	int color;
};

struct hostport {
	const char* host;
	const char* port;
	hostport(const char* h, const char* p)
	: host{ h }
	, port{ p }
	{
	}
};

void dump_all_data(hostport dest, dump_id id, bool ping = false)
{
	// Get a list of endpoints corresponding to the server name.
	tcp::resolver resolver(io_service);
	tcp::resolver::query query(dest.host, dest.port);
	tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

	// Try each endpoint until we successfully establish a connection.
	tcp::socket socket(io_service);
	boost::asio::connect(socket, endpoint_iterator);

	const std::string dir = id.prompt;
	fprintf(stderr ,"Started %s ...\n", dir.c_str());

	char b[10000];
	auto response = boost::asio::buffer(b, 10000);
	std::stringstream s;

	do {
		if (ping) {
			// @@TODO@@: in fact we would need a read_some() with
			// timeout after this, which is to be implemented later
			socket.write_some(boost::asio::buffer("*", 1));
		}
		s.str("");
		int x = socket.read_some(response);
		s << boost::chrono::time_fmt(boost::chrono::timezone::local) << boost::chrono::system_clock::now() << dir;
		hexdump(b, x, s.str(), id.color);
		if (ping)
			sleep(1); // gosh, how cheap can code be
	} while (1);
}

int main(int argc, char* argv[])
{
	if (argc != 3 && argc != 5 && argc != 7) {
		std::cout << "Usage: spy <host> <port> [<host> <port>] [<host> <port>]\n\n";
		std::cout << "       The first two addresses are pure listening, the\n"
		             "       third address is used to ping the interface destination\n"
		             "       and needs a loopback adaptor to work.\n";
		return 1;
	}

	std::thread t, t2;

	if (argc >= 5) {
		dump_id id{ " <<< ", 32 };
		hostport dest{ argv[3], argv[4] };
		t = std::thread{ dump_all_data, dest, id , false };
	}

	if (argc >= 7) {
		dump_id id{ " --- ", 36 };
		hostport dest{ argv[5], argv[6] };
		t2 = std::thread{ dump_all_data, dest, id , true };
	}

	dump_id id{ " >>> ", 31 };
	hostport dest{ argv[1], argv[2] };
	dump_all_data(dest, id);
}
