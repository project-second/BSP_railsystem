-- 1. 리니어(Preset) 테이블: 단일 위치 데이터
CREATE TABLE IF NOT EXISTS presets (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,      -- 프리셋 이름 (예: "출입문", "창문 A")
    position_cm REAL NOT NULL,      -- 물리적 위치 (cm 단위)
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 2. 투어(Tour) 테이블: 순찰 그룹 메타데이터
CREATE TABLE IF NOT EXISTS tours (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL UNIQUE,      -- 투어 이름 (예: "야간 순찰 A코스")
    created_at DATETIME DEFAULT CURRENT_TIMESTAMP
);

-- 3. 투어 스텝(Tour Steps) 테이블: 투어와 리니어를 연결하는 핵심 매핑 테이블
-- N:M (다대다) 관계를 풀어주며, 순서와 대기 시간을 저장합니다.
CREATE TABLE IF NOT EXISTS tour_steps (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    tour_id INTEGER NOT NULL,       -- 어떤 투어에 속해 있는지
    preset_id INTEGER NOT NULL,     -- 어떤 프리셋으로 갈 것인지
    step_order INTEGER NOT NULL,    -- 방문 순서 (1, 2, 3...)
    dwell_time_sec INTEGER DEFAULT 3, -- 해당 위치에서 몇 초간 머무를 것인지
    
    -- 외래 키(Foreign Key) 제약 조건: 부모 데이터가 지워지면 같이 지워지도록(CASCADE) 설정
    FOREIGN KEY (tour_id) REFERENCES tours(id) ON DELETE CASCADE,
    FOREIGN KEY (preset_id) REFERENCES presets(id) ON DELETE CASCADE
);

-- 테스트용 초기 더미 데이터 (Mock Data) 삽입
INSERT INTO presets (name, position_cm) VALUES ('Zone A (Left)', 10.5);
INSERT INTO presets (name, position_cm) VALUES ('Zone B (Center)', 100.0);
INSERT INTO presets (name, position_cm) VALUES ('Zone C (Right)', 180.0);

INSERT INTO tours (name) VALUES ('Night Patrol Basic');

-- Night Patrol Basic 투어의 시나리오: Zone A(3초) -> Zone C(5초) -> Zone B(3초)
INSERT INTO tour_steps (tour_id, preset_id, step_order, dwell_time_sec) VALUES (1, 1, 1, 3);
INSERT INTO tour_steps (tour_id, preset_id, step_order, dwell_time_sec) VALUES (1, 3, 2, 5);
INSERT INTO tour_steps (tour_id, preset_id, step_order, dwell_time_sec) VALUES (1, 2, 3, 3);