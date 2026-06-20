# 1. 리눅스 커널 빌드 시스템용 오브젝트 파일 지정
# 빌드 성공 시 vms_rail_hal.ko 와 vms_hw_ctrl.ko 가 생성됩니다.
obj-m += vms_rail_hal.o
obj-m += vms_hw_ctrl.o

# 2. 라즈베리파이 커널 헤더 경로 및 현재 디렉토리 설정
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)

# 3. 기본 빌드 타겟 (명령어: make)
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# 4. 빌드 부산물 청소 타겟 (명령어: make clean)
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

# 5. [편의 기능] 드라이버 커널 적재 및 권한 부여 (명령어: make load)
# C++ 앱이나 Qt 앱이 sudo 권한 없이 파일에 접근할 수 있도록 chmod 666을 수행합니다.
load:
	@echo "Loading VMS Kernel Modules..."
	sudo insmod vms_rail_hal.ko
	sudo insmod vms_hw_ctrl.ko
	sudo chmod 666 /dev/vms_rail_hal
	sudo chmod 666 /dev/vms_hw_ctrl
	@echo "Drivers loaded successfully with read/write permissions."

# 6. [편의 기능] 드라이버 커널 해제 (명령어: make unload)
unload:
	@echo "Unloading VMS Kernel Modules..."
	sudo rmmod vms_rail_hal
	sudo rmmod vms_hw_ctrl
	@echo "Drivers unloaded successfully."