# Monolith
![image](/web/assets/logo/wide.jpg)

모노리스는 대학생 자작자동차대회 출전 차량을 위한 오픈소스 DIY 데이터로깅 플랫폼입니다.

* CAN 통신, 디지털 및 아날로그 신호, 3축 가속도, GPS 등 다양한 데이터 기록
* Micro SD 카드에 로그 저장
* 실시간 텔레메트리(원격 데이터 모니터링) 기능
* 자체 시계(RTC)를 통한 실제 시간 추적
* 웹 기반 데이터 시각화 및 분석 도구 제공

`TMA-1` 데이터로거, `TMA-2` 실시간 중계 서버, `TMA-3` 데이터 분석 도구로 구성되어 있습니다.

## [가이드 문서](https://github.com/luftaquila/monolith/wiki)

## TMA-1 데이터로거
차량에 장착되어 데이터를 수집하는 장비입니다. 기록할 수 있는 데이터는 다음과 같습니다.

* CAN 버스 트래픽
* 8채널 디지털 입력 신호
* 4채널 아날로그 입력 신호
* 4채널 디지털 파형 주기
* 3축 ±4g 가속도 (ADXL345)
* GPS 위치 정보 (NEO-7M)
* 전원 전압(LV)
* 자체 CPU 온도
* RTC를 통한 실제 시간 정보 추적 및 동기화

각 기능에 대한 상세 설명은 [TMA‐1 기능 및 사용 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%901-%EA%B8%B0%EB%8A%A5-%EB%B0%8F-%EC%82%AC%EC%9A%A9-%EA%B0%80%EC%9D%B4%EB%93%9C)를 참고하세요.

### 1-1. TMA-1 Do It Yourself!
TMA-1은 쉽게 제작할 수 있도록 SMD 부품 대신 시중에 판매하는 모듈을 최대한 사용하여 설계되었습니다.

![image](https://github.com/luftaquila/monolith/assets/17094868/7883bb92-a58b-4ab8-9087-d2fc6ee720ea)

* [TMA-1 DIY 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%901-DIY-%EA%B0%80%EC%9D%B4%EB%93%9C)
* [PCB 거버 파일](https://github.com/luftaquila/monolith/blob/main/device/hardware/gerber/gerber.zip)
* [PCB 설계도](https://github.com/luftaquila/monolith/tree/main/device/hardware) (KiCAD 7.0 회로도/PCB 아트웍)

### 1-2. TMA-1 설정 도구
TMA-1에서 활성화할 기능을 선택하여 펌웨어를 직접 빌드하고 보드에 업로드하는 GUI 설정 도구입니다.

또한, TMA-1의 RTC를 컴퓨터의 시계에 동기화하는 기능을 제공합니다.

* [TMA-1 설정 도구 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%901-%EC%84%A4%EC%A0%95-%EB%8F%84%EA%B5%AC-%EA%B0%80%EC%9D%B4%EB%93%9C)

<img src='https://github.com/luftaquila/monolith/assets/17094868/aee092ce-9f0a-4a0e-9fb4-b2b1cbcccda3' style='width: 700px'>

## TMA-2 무선 통신 중계 서버
TMA-1 데이터로거가 전송한 로그를 실시간으로 해석하여 웹 클라이언트에 중계하는 서버입니다. 텔레메트리 기능을 사용할 때만 필요합니다.

TMA-2 서버와 웹 클라이언트는 기본적으로 모노리스에서 제공하여 별도로 구축할 필요가 없습니다.

중계 서버는 기본값으로 monolith.luftaquila.io:80 을 사용합니다. 서버를 개별적으로 구축하고 싶다면 [TMA-2 서버 구축 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%902-%EC%84%9C%EB%B2%84-%EA%B5%AC%EC%B6%95-%EA%B0%80%EC%9D%B4%EB%93%9C)를 참고하여 직접 self-hosting 하여 사용할 수 있습니다.

> 기본 제공되는 TMA-2 서버를 사용하려면 먼저 사용자 등록을 해야 합니다.<br>
> 등록을 원하시면 mail@luftaquila.io 로 메일 주세요.

클라이언트는 https://monolith.luftaquila.io/live 에서 각자 UI를 구성해 사용할 수 있습니다. 별도로 구현을 원하는 경우 소켓 통신 라이브러리를 이용해 직접 구현할 수 있습니다.

<img src='https://github.com/luftaquila/monolith/assets/17094868/5ba95b27-f435-4d70-a965-757269b4843e'><br>

## TMA-3 로그 해석기 및 데이터 분석 도구
TMA-1 데이터로거는 용량 최적화를 위해 데이터를 바이너리 형식으로 SD 카드에 저장합니다.

TMA-3는 저장된 로그를 사람이 읽을 수 있도록 json 또는 csv 파일로 변환하는 해석기와, 변환한 데이터를 그래프로 시각화하는 데이터 분석 도구 모음입니다.

모두 웹 어플리케이션으로 구현되어 https://monolith.luftaquila.io 에서 바로 사용할 수 있습니다.

<img src='https://github.com/luftaquila/monolith/assets/17094868/12c3801c-3507-4647-bfbc-e4012fde11ea'>

## 기타
프로젝트의 이름인 모노리스는 아서 C. 클라크의 소설 2001: A Space Odyssey에서 차용하였습니다.

2023 KSAE 대학생 자작자동차대회 기술아이디어 금상, 2022-2 아주대학교 아주 훌륭한 SW 융합인의 도전 최우수 수상작입니다.

<img src='https://github.com/luftaquila/monolith/assets/17094868/53384153-dbec-466c-b6d7-5401e73fa48c' style='width: 400px'>

## LICENSE
```
"THE BEERWARE LICENSE" (Revision 42):
LUFT-AQUILA wrote this project. As long as you retain this notice,
you can do whatever you want with this stuff. If we meet someday,
and you think this stuff is worth it, you can buy me a beer in return.
```
이 프로젝트의 내용물은 얼마든지 자유롭게 사용할 수 있습니다.

이 프로젝트가 마음에 든다면, 언젠가 우리가 만나게 되었을 때 맥주 한 잔 사 주세요.
