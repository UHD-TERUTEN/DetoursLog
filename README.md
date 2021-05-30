# DetoursLog
detours 라이브러리를 사용한 응용프로그램 로깅 DLL

## GoogleTest
![210530](https://user-images.githubusercontent.com/31408641/120103265-c84b7200-c189-11eb-9811-78c0fb99e21c.png)

## 사용법 (Detours sample [Setdll](https://github.com/microsoft/Detours/wiki/SampleSetdll) 사용 시)
1. 'x64 Native Tools Command Prompt for VS 2019'를 관리자 권한으로 실행
2. `setdll.exe /d:<디렉토리 경로>\DetoursLog.dll "C:\Program Files\Microsoft Office\root\Office16\WINWORD.EXE"` 실행 (MS Word)
3. 이후 워드 프로세스 사용하면 'D:\log.txt'에 로그를, 'D:\report.txt'에 프로세스가 사용하는 module(.exe 또는 .dll)의 버전 정보 로그를 남김
> 경로 변경이 필요한 경우, dllmain.cpp의 `LOG_PATH`를 직접 수정해서 사용   
> x64 Release 모드, 컴파일러 옵션에 UNICODE를 주어 빌드할 것   
> 작성된 로그 파일은 UTF-8 형식으로 기록됨
4. `setdll.exe /r "C:\Program Files\Microsoft Office\root\Office16\WINWORD.EXE"` 실행하면 순정 프로그램 상태로 되돌릴 수 있음

## 기타 마이크로소프트 오피스 프로그램 정보
* 플랫폼:		64-bit
* 오피스 프로그램 기본 경로:	C:\Program Files\Microsoft Office\root\Office16\

|프로그램|이름|
|--|--|
|워드프로세서|WINWORD.EXE|
|엑셀|EXCEL.EXE|
|액세스(?)|MSACCESS.EXE|
|퍼블리시(?)|MSPUB.EXE|
|아웃룩|OUTLOOK.EXE|
|파워포인트|POWERPNT.EXE|
