# monolith
![image](/web/assets/logo/wide.jpg)

<div align="center">
<strong>
<samp>

[English](README.md) · [한국어](README_KO.md)

</samp>
</strong>
</div>

모노리스는 대학생 자작자동차대회 출전 차량을 위한 오픈소스 DIY 데이터로깅 플랫폼입니다.

가이드에 따라 데이터로거만 직접 제작하면 실시간 원격 모니터링, 데이터 분석 도구 등을 쉽게 이용할 수 있습니다.

### 주요 기능
* CAN, 3축 가속도, GPS, 디지털/아날로그 신호, 휠 스피드 등 다양한 데이터 기록
* Micro SD 카드에 로그 저장
* 실시간 텔레메트리(원격 데이터 모니터링)
* 자체 시계(RTC)를 통한 실제 시간 추적
* 웹 기반 데이터 시각화 및 분석 도구

### [가이드 문서](https://github.com/luftaquila/monolith/wiki)

`TMA-1` 데이터로거, `TMA-2` 텔레메트리 모니터, `TMA-3` 데이터 분석 도구로 구성되어 있습니다.

<br>

## TMA-1 데이터로거
차량에 장착되어 데이터를 수집하는 장비입니다. 기록할 수 있는 데이터의 종류는 다음과 같습니다.

* CAN 2.0(A/B) 버스 트래픽
* 8채널 디지털 입력 신호
* 4채널 아날로그 입력 신호
* 4채널 휠 스피드 센서(디지털 파형 주기 측정)
* 3축 ±4g 가속도 (ADXL345)
* GPS 위치 정보 (NEO-6/7M)
* 전원(LV) 전압 및 자체 CPU 온도
* RTC 실제 시간 정보

각 기능에 대한 상세 설명은 [TMA‐1 기능 및 사용 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%901-%EA%B8%B0%EB%8A%A5-%EB%B0%8F-%EC%82%AC%EC%9A%A9-%EA%B0%80%EC%9D%B4%EB%93%9C)를 참고하세요.

### 1-1. TMA-1 Do It Yourself!
TMA-1은 직접 부품을 구매해 제작하여 사용하는 장비입니다.

쉽게 제작할 수 있도록 최대한 시중에 판매하는 모듈과 스루홀 부품을 사용하여 설계되었습니다.

제작에 필요한 부품 목록과 제작 방법은 [TMA-1 DIY 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%901-DIY-%EA%B0%80%EC%9D%B4%EB%93%9C)를 참고하세요.

<img src='https://github.com/luftaquila/monolith/assets/17094868/062e846b-619e-4b5e-a784-63690c4cd73f' style='width: 700px'>

### 1-2. TMA-1 설정 도구
TMA-1의 펌웨어를 빌드하고 업로드하는 GUI 설정 도구입니다.

데이터로거에서 활성화할 기능을 설정할 수 있으며, TMA-1의 RTC를 컴퓨터의 시계에 동기화하는 기능을 제공합니다.

텔레메트리를 위한 Wi-Fi 핫스팟 네트워크, 차량 ID 및 서버 주소 등 또한 이 도구를 이용하여 설정할 수 있습니다.

자세한 사용법은 [TMA-1 설정 도구 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%901-%EC%84%A4%EC%A0%95-%EB%8F%84%EA%B5%AC-%EA%B0%80%EC%9D%B4%EB%93%9C)을 참고하세요.

<img src='https://github.com/luftaquila/monolith/assets/17094868/2350a23e-c4a7-4766-830a-d4313ee9fbee' width='700px'>

<br><br>

## TMA-2 텔레메트리 모니터
TMA-1 데이터로거가 통신하는 텔레메트리 서버와, 실시간 데이터 모니터링을 위한 웹 클라이언트입니다.

### 서버
따로 설정하지 않는 한 TMA-1은 기본적으로 모노리스 서버(monolith.luftaquila.io)를 사용하여 데이터를 실시간으로 전송합니다.

> 기본 제공되는 TMA-2 서버는 확인된 사용자만 이용할 수 있습니다.<br>
> 별도의 비용은 없으며, mail@luftaquila.io 로 학교, 팀 이름, 사용할 차량 ID와 key를 보내 주시면 사용자 등록이 완료됩니다.

서버를 개별적으로 구축하고 싶다면 [TMA-2 서버 구축 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%902-%EC%84%9C%EB%B2%84-%EA%B5%AC%EC%B6%95-%EA%B0%80%EC%9D%B4%EB%93%9C)를 참고하여 직접 self-hosting할 수 있습니다.

### 클라이언트
실시간 모니터링 웹 클라이언트는 https://monolith.luftaquila.io/live 에서 제공됩니다.

각 사용자가 개별적으로 UI를 설정할 수 있으며, UI 설정 파일을 내보내고 불러오는 기능을 지원합니다.

자세한 사용법은 [TMA-2 클라이언트 사용 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%902-%ED%81%B4%EB%9D%BC%EC%9D%B4%EC%96%B8%ED%8A%B8-%EC%82%AC%EC%9A%A9-%EA%B0%80%EC%9D%B4%EB%93%9C)를 참고하세요.

<img src='https://github.com/luftaquila/monolith/assets/17094868/5ba95b27-f435-4d70-a965-757269b4843e'>

<br><br>

## TMA-3 로그 해석기 및 데이터 분석 도구
TMA-1 데이터로거는 데이터를 [정해진 프로토콜](https://github.com/luftaquila/monolith/wiki/%EB%A1%9C%EA%B7%B8-%ED%94%84%EB%A1%9C%ED%86%A0%EC%BD%9C)에 따라 바이너리 파일로 SD 카드에 기록합니다.

이러한 로그를 사람이 읽을 수 있도록 json 및 csv 파일로 변환하는 해석기와, 데이터를 그래프로 시각화하는 분석 도구 모음입니다.

모두 웹 어플리케이션으로 구현되어 https://monolith.luftaquila.io 에서 바로 사용할 수 있습니다.

자세한 사용법은 [TMA-3 데이터 분석 도구 가이드](https://github.com/luftaquila/monolith/wiki/TMA%E2%80%903-%EB%8D%B0%EC%9D%B4%ED%84%B0-%EB%B6%84%EC%84%9D-%EB%8F%84%EA%B5%AC-%EA%B0%80%EC%9D%B4%EB%93%9C)를 사용하세요.

<img src='https://github.com/luftaquila/monolith/assets/17094868/7734849b-b7c6-4fe3-9ab6-27e2969be806' width='700px'>

<br><br>

## 기타
프로젝트의 이름은 아서 C. 클라크의 소설 2001: A Space Odyssey에서 차용하였습니다.

*2023 KSAE 대학생 자작자동차대회 기술아이디어 부문* 금상, *2022-2 아주대학교 아주 훌륭한 SW 융합인의 도전* 최우수 수상작입니다.

<img src='https://github.com/luftaquila/monolith/assets/17094868/53384153-dbec-466c-b6d7-5401e73fa48c' style='width: 400px'>

<br><br>

## Sponsors
<img src='https://github.com/luftaquila/monolith/assets/17094868/2a893442-4d76-4828-a763-64865f9e481f' width='200px'>

Prototype PCB of the TMA-1 was sponsored by [PCBWay](https://www.pcbway.com/).

<br>

## LICENSE
```
"THE BEERWARE LICENSE" (Revision 42):
LUFT-AQUILA wrote this project. As long as you retain this notice,
you can do whatever you want with this stuff. If we meet someday,
and you think this stuff is worth it, you can buy me a beer in return.
```
이 프로젝트의 내용물은 얼마든지 자유롭게 사용할 수 있습니다.

이 프로젝트가 마음에 든다면, 언젠가 우리가 만나게 되었을 때 맥주 한 잔 사 주세요.

<br>

<hr>

<picture>
  <source media="(prefers-color-scheme: dark)" srcset="https://api.star-history.com/svg?repos=luftaquila/monolith&type=Date&theme=dark" />
  <source media="(prefers-color-scheme: light)" srcset="https://api.star-history.com/svg?repos=luftaquila/monolith&type=Date" />
  <img alt="Star History Chart" src="https://api.star-history.com/svg?repos=star-history/star-history&type=Date" />
</picture>
