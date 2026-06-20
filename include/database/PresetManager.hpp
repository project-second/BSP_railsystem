#ifndef PRESET_MANAGER_HPP
#define PRESET_MANAGER_HPP

#include <string>
#include <vector>
#include <sqlite3.h>

// DB에서 꺼내올 리니어(Preset) 데이터를 담을 C++ 구조체
struct Preset {
    int id;
    std::string name;
    double position_cm;
};

class PresetManager {
public:
    explicit PresetManager(const std::string& dbPath = "storage/vms_local.db");
    ~PresetManager();

    // DB 연결 초기화
    bool init();
    
    // DB에 저장된 모든 프리셋 목록 불러오기
    std::vector<Preset> getAllPresets();

private:
    std::string m_dbPath;
    sqlite3* m_db;
};

#endif // PRESET_MANAGER_HPP