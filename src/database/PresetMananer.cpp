#include "database/PresetManager.hpp"
#include <iostream>

PresetManager::PresetManager(const std::string& dbPath) 
    : m_dbPath(dbPath), m_db(nullptr) {}

PresetManager::~PresetManager() {
    if (m_db) {
        sqlite3_close(m_db); // 메모리 누수 방지
    }
}

bool PresetManager::init() {
    // SQLite DB 파일 열기
    int rc = sqlite3_open(m_dbPath.c_str(), &m_db);
    if (rc) {
        std::cerr << "[PresetManager] DB 연결 실패: " << sqlite3_errmsg(m_db) << std::endl;
        return false;
    }
    std::cout << "[PresetManager] 로컬 SQLite DB 연결 성공 (" << m_dbPath << ")" << std::endl;
    return true;
}

std::vector<Preset> PresetManager::getAllPresets() {
    std::vector<Preset> presets;
    const char* sql = "SELECT id, name, position_cm FROM presets ORDER BY position_cm ASC;";
    sqlite3_stmt* stmt;

    // SQL 쿼리 준비
    if (sqlite3_prepare_v2(m_db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << "[PresetManager] 쿼리 준비 실패: " << sqlite3_errmsg(m_db) << std::endl;
        return presets;
    }

    // 한 줄씩 데이터 읽어오기
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        Preset p;
        p.id = sqlite3_column_int(stmt, 0);
        p.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        p.position_cm = sqlite3_column_double(stmt, 2);
        presets.push_back(p);
    }

    sqlite3_finalize(stmt);
    return presets;
}