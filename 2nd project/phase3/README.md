myshell - Phase 3

myshell은 사용자가 입력한 명령어를 파싱하고 실행하는 간단한 사용자 정의 쉘 프로그램입니다.
Phase 3은 명령어를 background로 실행하고 signal을 통해 이를 관리할 수 있습니다.


구성 파일

csapp.h, csapp,c : 프로그래밍을 위한 유틸리티 함수들이 정의되어 있습니다.
myshell.c : 사용자 정의 쉘의 메인 로직이 구현되어 있습니다.


주요 기능

1. background 실행
	명령어에 '&'이 포함되어 있으면 해당 명령어를 background process로 실행합니다.
	Fork()를 통해 parent process에서 곧바로 다음 명령어를 입력할 수 있습니다.
2. process(job) 관리 구조
	process의 pid, 상태, 명령어를 저장하는 Job 구조체를 정의하고, 배열을 만들어 저장했습니다.
	각 process의 pid를 통해 process를 관리하며, -1을 초기 상태로 설정했습니다.
	배열에 존재하는 Job의 상태와 pid 값을 출력할 수 있습니다.
3. siganl handling
	background process를 관리하기 위해 siganl handler 함수들을 정의했습니다.
	사용자 지정 명령어를 통해 signal을 다뤄 여러 process를 관리할 수 있습니다.
4. 상태와 signal의 역할
	명령어 뒤에 '&'을 붙이면 해당 명령어가 background에서 실행합니다.
	ctrl+z를 누르면 SIGTSTP에 의해 background process가 stop 상태가 됩니다.
	fg 명령어를 통해 background process를 foreground로 실행할 수 있습니다.
	bg 명령어를 통해 foreground process를 background로 실행할 수 있습니다.
	kill 명령어를 통해 background process를 terminate 할 수 있습니다.
	ctrl+c를 누르면 SIGINT에 의해 foreground process를 terminate 할 수 있습니다.
