#ifndef MOTOR_CONTROLLER_HPP
#define MOTOR_CONTROLLER_HPP

#include <string>

class MotorController {
public:
    // 기본적으로 우리가 만든 드라이버 파일(/dev/vms_rail_hal)을 타겟으로 설정합니다.
    explicit MotorController(const std::string& devicePath = "/dev/vms_rail_hal");
    ~MotorController();

    // 디바이스 드라이버 연결 및 초기화
    bool init();
    
    // 모터 수동 제어 (방향: 1(전진), -1(후진), 0(정지) / 속도: 0 ~ 255 PWM)
    bool move(int direction, int speed);
    
    // 특정 목적지(cm 단위) 지정 이동 (현재 위치와 비교하여 전/후진 자동 결정)
    bool moveTo(double targetCm, int speed);
    
    // 모터 즉시 비상 정지
    bool stop();
    
    // 커널 엔코더 인터럽트 값을 읽어 현재 레일 위 위치(cm) 반환
    double getCurrentPosition();

private:
    std::string m_devicePath;
    int m_fd;                  // 디바이스 파일 핸들러 (File Descriptor)
    double m_currentPosition;  // 소프트웨어 측 스냅샷 위치
};

#endif // MOTOR_CONTROLLER_HPP