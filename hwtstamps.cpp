#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <linux/net_tstamp.h>
#include <linux/sockios.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <system_error>
#include <unistd.h>
#include <utility>

namespace
{
	class unique_fd
	{
	  public:
		unique_fd() noexcept : fd_{-1} {}

		explicit unique_fd(int fd) noexcept : fd_{fd} {}

		unique_fd(const unique_fd&) = delete;
		unique_fd& operator=(const unique_fd&) = delete;

		unique_fd(unique_fd&& other) noexcept :
		    fd_{std::exchange(other.fd_, -1)}
		{
		}

		unique_fd& operator=(unique_fd&& other) noexcept
		{
			close();
			std::swap(fd_, other.fd_);
			return *this;
		}

		~unique_fd() { close(); }

		explicit operator bool() const noexcept { return fd_ > -1; }

		void close() noexcept
		{
			if (fd_ > -1) {
				::close(fd_); /* Ignoring any errors here. */
				fd_ = -1;
			}
		}

		int get() const noexcept { return fd_; }

	  private:
		int fd_;
	};
} // namespace

int main(int argc, const char* argv[])
{
	if (argc != 2) {
		std::cerr << "missing device name\n";
		return EXIT_FAILURE;
	}

	const char* dev = argv[1];

	auto fd = unique_fd{socket(AF_UNIX, SOCK_DGRAM, 0)};
	if (!fd) {
		const auto err_cond =
		    std::error_condition{errno, std::generic_category()};
		std::cerr << "socket(): " << err_cond.message() << '\n';
		return EXIT_FAILURE;
	}

	hwtstamp_config hwts_conf = {};
	hwts_conf.rx_filter = HWTSTAMP_FILTER_ALL;

	ifreq ifr = {};
	ifr.ifr_data = reinterpret_cast<__caddr_t>(&hwts_conf);
	std::strncpy(ifr.ifr_name, dev, sizeof(ifr.ifr_name) - 1);
	if (std::strlen(dev) != std::strlen(ifr.ifr_name)) {
		std::cerr << "device name too long\n";
		return EXIT_FAILURE;
	}

	if (ioctl(fd.get(), SIOCSHWTSTAMP, &ifr)) {
		const auto err_cond =
		    std::error_condition{errno, std::generic_category()};
		std::cerr << "ioctl(SIOCSHWTSTAMP): " << err_cond.message() << '\n';
		return EXIT_FAILURE;
	}

	std::cout << "driver returned " << hwts_conf.rx_filter
	          << " (one of HWTSTAMP_FILTER_*) in rx_filter\n";
}
