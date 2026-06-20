#include "hardware/MotorController.hpp"
#include <iostream>
#include <cmath>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>

// 커널 드라이버와 약속된 이진(Binary) 데이터 프로토콜 구조체
struct RailCommand {
    int direction; // 1: 전진, -1: 후진, 0: 정지
    int speed;     // PWM 속도 (0 ~ 255)
};

MotorController::MotorController(const std::string& devicePath)
    : m_devicePath(devicePath), m_fd(-1), m_currentPosition(0.0) {}

MotorController::~MotorController() {
    stop(); // 안전을 위해 어플리케이션 종료 시 모터 무조건 정지
    if (m_fd >= 0) {
        close(m_fd);
        std::cout << "[MotorController] Device file closed safely." << std::endl;
    }
}

bool MotorController::init() {
    // 리눅스 파일 시스템을 통해 드라이버 오픈 (읽기/쓰기 모드)
    m_fd = open(m_devicePath.c_str(), O_RDWR);
    if (m_fd < 0) {
        std::cerr << "[MotorController] CRITICAL: Cannot open device file " << m_devicePath 
                  << " (Reason: " << std::strerror(errno) << ")" << std::endl;
        std::cerr << "[MotorController] HINT: 커널 드라이버(.ko)가 insmod로 정상 적재되었는지 확인하세요." << std::endl;
        return false;
    }
    std::cout << "[MotorController] Hardware interface initialized. Node: " << m_devicePath << std::endl;
    return true;
}

bool MotorController::move(int direction, int speed) {
    if (m_fd < 0) return false;

    // 안전장치: 속도 범위를 하드웨어 한계치(0~255) 내로 한정 (클램핑)
    if (speed < 0) speed = 0;
    if (speed > 255) speed = 255;

    // 구조체에 데이터 세팅
    RailCommand cmd{direction, speed};
    
    // 커널의 write 함수 호출 -> 드라이버 내부의 .write 함수가 실행됨
    ssize_t bytesWritten = write(m_fd, &cmd, sizeof(RailCommand));
    if (bytesWritten < 0) {
        std::cerr << "[MotorController] Failed to pass command into kernel space." << std::endl;
        return false;
    }
    return true;
}

bool MotorController::moveTo(double targetCm, int speed) {
    if (m_fd < 0) return false;

    // 현재 하드웨어 실시간 위치 파악
    double current = getCurrentPosition();
    
    // 목표 도달 판정 임계값 (예: 오차 범위 0.5cm 이내면 제동)
    if (std::abs(targetCm - current) < 0.5) {
        std::cout << "[MotorController] Target reached. Braking at: " << current << "cm" << std::endl;
        return stop();
    }

    // 방향성 자동 계산 (현재 위치보다 목표치가 크면 정방향, 작으면 역방향)
    int direction = (targetCm > current) ? 1 : -1;
    return move(direction, speed);
}

bool MotorController::stop() {
    return move(0, 0);
}

double MotorController::getCurrentPosition() {
    if (m_fd < 0) return m_currentPosition;

    int encoderCount = 0;
    // 커널 드라이버로부터 실시간 엔코더 카운트(바이너리 int) 읽기
    ssize_t bytesRead = read(m_fd, &encoderCount, sizeof(int));
    if (bytesRead < 0) {
        std::cerr << "[MotorController] Failed to read hardware encoder tick." << std::endl;
        return m_currentPosition;
    }

    // [기구학 환산 공식 수식]
    // 모터 엔코더 펄스 수(Tick)를 실제 물리적 이동 거리(cm)로 변환하는 비례상수
    // (예시: 감속비와 우레탄 바퀴 지름 계산 결과 100개 펄스당 1cm 전진한다고 가정)
    const double TICK_PER_CM = 100.0; 
    m_currentPosition = static_cast<double>(encoderCount) / TICK_PER_CM;

    return m_currentPosition;
}