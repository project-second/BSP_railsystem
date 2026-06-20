#include <iostream>
#include <thread>
#include <chrono>
#include "hardware/MotorController.hpp"

int main() {
    std::cout << "[System] VMS Edge Node Application Start..." << std::endl;
    
    // MotorController 객체 생성 (기본값으로 /dev/vms_rail_hal 지정됨)
    MotorController motor;
    
    // 드라이버 파일 연결 시도
    // (현재 도커 환경엔 .ko가 실제 하드웨어 노드로 생성되어 있지 않으므로 가짜 성공 분기 생성)
    bool isConnected = motor.init();
    
    if (!isConnected) {
        std::cout << "[Simulation Mode] 실제 하드웨어 노드가 감지되지 않아 가상 시뮬레이션 모드로 전환합니다." << std::endl;
    }

    std::cout << "\n--- [Test Scenario 1: 수동 모터 구동 명령 송출] ---" << std::endl;
    if (isConnected) {
        motor.move(1, 150); // 정방향, 150의 속도로 전진
    } else {
        std::cout << "[Virtual] Write command to /dev/vms_rail_hal -> Dir: 1, Speed: 150" << std::endl;
    }
    
    std::this_thread::sleep_for(std::chrono::seconds(2)); // 2초 주행 시뮬레이션

    std::cout << "\n--- [Test Scenario 2: 실시간 위치 확인 및 프리셋 도달 테스트] ---" << std::endl;
    if (isConnected) {
        double currentPos = motor.getCurrentPosition();
        std::cout << "[Hardware] 현재 CCTV 물리적 위치: " << currentPos << "cm" << std::endl;
        motor.moveTo(50.0, 200); // 50cm 지점에 지정 속도 200으로 이동 요청
    } else {
        std::cout << "[Virtual] 현재 가상 위치: 0.0cm -> 목표 50.0cm 지점으로 이동 명령 계산" << std::endl;
        std::cout << "[Virtual] 이동 방향 계산 완료: 정방향(1)으로 구동 가동" << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(1));
    motor.stop();
    std::cout << "\n[System] 테스트 제어 시나리오 무사히 종료." << std::endl;
    
    return 0;
}