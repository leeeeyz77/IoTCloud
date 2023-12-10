# IoTCloud

## 크리스마스 트리로 관리하는 반려식물

![IoT클라우드플랫폼_기말프로젝트보고서_이연지_유란](https://github.com/leeeeyz77/IoTCloud/assets/102798337/062d4dce-b84e-42c7-94d1-c02a1748515b)


1. Arduino : 디바이스
   - 온습도 센서: 현재 온도 측정
   - 3색 LED : 현재 온도를 시각적으로 보여주는 용도
   - LCD(열선패드 대체)
       : 화분마다 다른 적정온도 3가지 버전으로 적용 - ["COLD", "WARM", "HOT"]
       : ON, OFF 표시
3. Lambda
   - 디바이스의 현재 온도, 3색 LED 색깔, 현재 설정한 온도, 현재 열선패드 On/off 상태 DynamoDB에 로그 기록
   - DynamoDB에 저장된 로그 기록 조회
   - Web을 이용해 디바이스 상태 조회/변경
4. Web
   - 3개의 버튼["COLD", "WARM", "HOT"]을 통해서 열선패드 적정온도 변경
   - 지속적으로 열선패드 상태를 살펴보기 위한 로그 조회(TimeStamp, LED3, temperature, WarmPad, WarmPadState)
