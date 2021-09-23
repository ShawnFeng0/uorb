//
// Copyright (c) 2021 shawnfeng. All rights reserved.
//

#include <netinet/in.h>
#include <unistd.h>

#include <cstdlib>
#include <sstream>
#include <thread>

#include "slog.h"
#include "uorb/abs_time.h"
#include "uorb/publication.h"
#include "uorb/publication_multi.h"
#include "uorb/subscription.h"
#include "uorb/subscription_interval.h"
#include "uorb/topics/example_string.h"
#include "uorb/topics/msg_template.h"
#include "uorb/topics/sensor_accel.h"
#include "uorb/topics/sensor_gyro.h"

void TcpSocketSendThread(int socket_fd) {
  std::string str_buf;
  while (true) {
    char buf[1024];
    auto n = read(socket_fd, buf, sizeof(buf));
    if (n <= 0) break;
    write(socket_fd, buf, n);

    //    str_buf.append(buf, buf + n);
    //    std::string line;
    //    std::getline(std::istringstream{str_buf}, line);
  }
  close(socket_fd);
}

void TcpServerThread(uint16_t port) {
  int server_fd;
  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  // Forcefully attaching socket to the port 8080
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt,
                 sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in address {};
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  // Forcefully attaching socket to the port 8080
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 30) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }

  int new_socket;
  int addrlen = sizeof(address);
  while ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                              (socklen_t *)&addrlen)) >= 0) {
    std::thread{TcpSocketSendThread, new_socket}.detach();
  }
  LOGGER_ERROR("TCP server finished");
}

template <const orb_metadata &T>
[[noreturn]] static void thread_publisher() {
  uorb::PublicationData<T> publication_data;

  while (true) {
    auto &data = publication_data.get();

    data.timestamp = orb_absolute_time_us();

    if (!publication_data.Publish()) {
      LOGGER_ERROR("Publish error");
    }

    usleep(1 * 1000 * 1000);
  }
  LOGGER_WARN("Publication over.");
}

template <const orb_metadata &T>
[[noreturn]] static void thread_subscriber() {
  uorb::SubscriptionData<T> subscription_data;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

  int timeout_ms = 2000;

  struct orb_pollfd poll_fds[] = {
      {.fd = subscription_data.handle(), .events = POLLIN, .revents = 0}};

  while (true) {
    if (0 < orb_poll(poll_fds, ARRAY_SIZE(poll_fds), timeout_ms)) {
      if (subscription_data.Update()) {
        //        auto data = sub_example_string.get();
        //        LOGGER_INFO("timestamp: %" PRIu64 "[us]", data.timestamp);
      }
    }
  }
}

int main(int, char *[]) {
  LOGGER_INFO("uORB version: %s", orb_version());

  for (int i = 0; i < 3; ++i)
    std::thread{thread_publisher<uorb::msg::example_string>}.detach();

  for (int i = 0; i < 3; ++i)
    std::thread{thread_publisher<uorb::msg::sensor_accel>}.detach();

  for (int i = 0; i < 3; ++i)
    std::thread{thread_subscriber<uorb::msg::example_string>}.detach();

  for (int i = 0; i < 3; ++i)
    std::thread{thread_subscriber<uorb::msg::sensor_accel>}.detach();

  for (int i = 0; i < 3; ++i)
    std::thread{thread_subscriber<uorb::msg::sensor_gyro>}.detach();

  std::thread{TcpServerThread, 10924}.detach();

  // Wait for all threads to finish
  pthread_exit(nullptr);
}
